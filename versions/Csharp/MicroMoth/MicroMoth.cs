// (C) Copyright Moth 2026.
// (C) Copyright IBM 2023. (Original MicroQiskit)
// Apache-2.0

using System.Numerics;
using System.Runtime.CompilerServices;

namespace Moth.MicroMoth;

public enum GateOp { Init, X, H, Rx, Rz, Cx, Crx, Swap, M }

public readonly record struct Gate(
    // single-qubit gates: A means the target.
    // two-qubit gates: A means the control qubit and B means the target qubit.
    // measurement gate: A stands for the qubit and B is for the classical bit.
    GateOp Op, double Theta = 0, int A = 0, int B = 0, Complex[]? State = null
);

public sealed class QuantumCircuit
{
    public int Qubits { get; }
    public int Clbits { get; private set; }
    public string Name { get; set; } = "";
    public List<Gate> Data { get; } = new();

    public QuantumCircuit(int n, int m = 0)
    {
        if (n < 1) throw new ArgumentOutOfRangeException(nameof(n), "Need at least 1 qubit.");
        if (m < 0) throw new ArgumentOutOfRangeException(nameof(m));
        Qubits = n;
        Clbits = m;
    }

    // ===== Basic of the basics =====
    public QuantumCircuit X(int q)            { Data.Add(new Gate(GateOp.X, A: q)); return this; }
    public QuantumCircuit H(int q)            { Data.Add(new Gate(GateOp.H, A: q)); return this; }
    public QuantumCircuit Rx(double th, int q){ Data.Add(new Gate(GateOp.Rx, Theta: th, A: q)); return this; }
    public QuantumCircuit Rz(double th, int q){ Data.Add(new Gate(GateOp.Rz, Theta: th, A: q)); return this; }
    public QuantumCircuit Cx(int s, int t)    { Data.Add(new Gate(GateOp.Cx, A: s, B: t)); return this; }
    public QuantumCircuit Crx(double th, int s, int t) { Data.Add(new Gate(GateOp.Crx, Theta: th, A: s, B: t)); return this; }
    public QuantumCircuit Swap(int s, int t)  { Data.Add(new Gate(GateOp.Swap, A: s, B: t)); return this; }

    // ===== (Slightly) advanced gates as a combo of basic gates =====
    public QuantumCircuit Ry(double th, int q)
    {
        Rx(Math.PI / 2, q);
        Rz(th, q);
        Rx(-Math.PI / 2, q);
        return this;
    }

    public QuantumCircuit Z(int q) => Rz(Math.PI, q);
    public QuantumCircuit T(int q) => Rz(Math.PI / 4, q);

    public QuantumCircuit Y(int q)
    {
        Rz(Math.PI, q);
        X(q);
        return this;
    }

    // Measurement gate: these can be wrapped around Node's GD.Print on Godot 4.x.
    public QuantumCircuit Measure(int q, int b)
    {
        if (b >= Clbits) throw new ArgumentOutOfRangeException(nameof(b), "Classical bit is out of range.");
        if (q >= Qubits) throw new ArgumentOutOfRangeException(nameof(q), "Qubit is out of range.");
        Data.Add(new Gate(GateOp.M, A: q, B: b));
        return this;
    }

    public QuantumCircuit MeasureAll()
    {
        if (Clbits == 0) Clbits = Qubits; // create the classical bit storage if absent
        for (int q = 0; q < Qubits; q++) Measure(q, q);
        return this;
    }

    // Initialisation: Dimension (length: 2^Qubits) statevector.
    public QuantumCircuit Init(Complex[] state)
    {
        int dim = 1 << Qubits;
        if (state.Length != dim)
            throw new ArgumentException($"statevector must have {dim} amplitudes.", nameof(state));

        Data.Clear(); // wipes prior gates.
        Data.Add(new Gate(GateOp.Init, State: (Complex[])state.Clone()));
        return this;
    }
}

public static class Simulator
{
    const double Sqrt1_2 = 0.70710678118654752440;

    // ===== Helper functions for Maths =====

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    static (Complex, Complex) Superposition(Complex a, Complex b)
        => ((a + b) * Sqrt1_2, (a - b) * Sqrt1_2);

    // RX(θ) = [[cos, −i·sin], [−i·sin, cos]] at θ/2.
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    static (Complex, Complex) Turn(Complex a, Complex b, double theta)
    {
        double cos = Math.Cos(theta * 0.5), sin = Math.Sin(theta * 0.5);
        Complex iSin = new(0, sin);
        return (cos * a - iSin * b,
                cos * b - iSin * a);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    static (Complex, Complex) Phaseturn(Complex a, Complex b, double theta)
    {
        Complex phase = Complex.FromPolarCoordinates(1.0, theta * 0.5);
        return (a * Complex.Conjugate(phase),                          
                b * phase);
    }

    // ===== Getting information as a statevector =====

    public static Complex[] Statevector(QuantumCircuit qc)
    {
        int n = qc.Qubits, dim = 1 << n;
        var amp = new Complex[dim];
        amp[0] = Complex.One; 

        foreach (var g in qc.Data)
            switch (g.Op)
            {
                case GateOp.Init: g.State!.CopyTo(amp, 0); break; 
                case GateOp.M: break;                              
                case GateOp.X or GateOp.H or GateOp.Rx or GateOp.Rz: ApplySingle(amp, n, g); break;
                case GateOp.Cx or GateOp.Crx or GateOp.Swap:        ApplyControlled(amp, n, g); break;
            }

        return amp;
    }

    // A single-qubit gate mixes amplitude pairs whose indices differ only at 'target' bit.
    static void ApplySingle(Complex[] amp, int n, in Gate g)
    {
        int target = g.A;
        int step = 1 << target; 
        for (int high = 0; high < (1 << (n - target - 1)); high++)
            for (int low = 0; low < step; low++)
            {
                int i0 = low + (high << (target + 1)); // 'target' = 0
                int i1 = i0 + step;                     // 'target' = 1
                (amp[i0], amp[i1]) = g.Op switch
                {
                    GateOp.X  => (amp[i1], amp[i0]),
                    GateOp.H  => Superposition(amp[i0], amp[i1]),
                    GateOp.Rx => Turn(amp[i0], amp[i1], g.Theta),
                    _         => Phaseturn(amp[i0], amp[i1], g.Theta), // Rz gate
                };
            }
    }

    // A controlled gate only modifies states where the control bit is 1.
    static void ApplyControlled(Complex[] amp, int n, in Gate g)
    {
        int s = g.A, t = g.B;
        int lo = Math.Min(s, t), hi = Math.Max(s, t);
        int sBit = 1 << s, tBit = 1 << t;
        for (int a = 0; a < (1 << lo); a++)
            for (int b = 0; b < (1 << (hi - lo - 1)); b++)
                for (int c = 0; c < (1 << (n - hi - 1)); c++)
                {
                    int b00 = a + (b << (lo + 1)) + (c << (hi + 1));
                    int b10 = b00 + sBit; // control 1, target 0
                    int b11 = b10 + tBit; // control 1, target 1
                    switch (g.Op)
                    {
                        case GateOp.Cx:   (amp[b10], amp[b11]) = (amp[b11], amp[b10]); break;
                        case GateOp.Crx:  (amp[b10], amp[b11]) = Turn(amp[b10], amp[b11], g.Theta); break;
                        case GateOp.Swap: (amp[b00 + tBit], amp[b10]) = (amp[b10], amp[b00 + tBit]); break;
                    }
                }
    }

    // This can be called from the end user
    public static Dictionary<string, double> Probabilities(QuantumCircuit qc)
    {
        var amp = Statevector(qc);
        int n = qc.Qubits;
        var result = new Dictionary<string, double>(amp.Length);
        for (int j = 0; j < amp.Length; j++)
            result[ToBits(j, n)] = Norm2(amp[j]);
        return result;
    }

    public static Dictionary<string, int> Counts(QuantumCircuit qc, int shots = 1024, int? seed = null)
    {
        int m = qc.Clbits;
        var tally = new int[1 << m]; // integer tally -> string allocations only for distinct results
        foreach (int outcome in SampleOutcomes(qc, shots, seed)) tally[outcome]++;

        var result = new Dictionary<string, int>();
        for (int i = 0; i < tally.Length; i++)
            if (tally[i] > 0) result[ToBits(i, m)] = tally[i];
        return result;
    }

    public static List<string> Memory(QuantumCircuit qc, int shots = 1024, int? seed = null)
    {
        int m = qc.Clbits;
        var list = new List<string>(shots);
        foreach (int outcome in SampleOutcomes(qc, shots, seed)) list.Add(ToBits(outcome, m));
        return list;
    }

    // Result as integers
    static int[] SampleOutcomes(QuantumCircuit qc, int shots, int? seed)
    {
        var amp = Statevector(qc);
        int dim = amp.Length, m = qc.Clbits;

        var probs = new double[dim];
        double total = 0;
        for (int i = 0; i < dim; i++) { probs[i] = Norm2(amp[i]); total += probs[i]; }

        Span<int> map = m > 0 ? stackalloc int[m] : Span<int>.Empty; // clbit -> qubit, no heap alloc
        map.Fill(-1);
        foreach (var g in qc.Data) if (g.Op == GateOp.M) map[g.B] = g.A;

        var rng = seed is int sd ? new Random(sd) : Random.Shared; // **seed** ability for game development
        var outcomes = new int[shots];
        for (int k = 0; k < shots; k++)
        {
            double r = rng.NextDouble() * total, cumu = 0; // total tolerates an unnormalized state
            int chosen = dim - 1;                          // fallback guards floating-point round-off
            for (int j = 0; j < dim; j++) { cumu += probs[j]; if (r < cumu) { chosen = j; break; } }

            int outv = 0;
            for (int clbit = 0; clbit < m; clbit++)
                if (map[clbit] >= 0) outv |= ((chosen >> map[clbit]) & 1) << clbit;
            outcomes[k] = outv;
        }
        return outcomes;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    static double Norm2(Complex z) => z.Real * z.Real + z.Imaginary * z.Imaginary;

    static string ToBits(int value, int width) => Convert.ToString(value, 2).PadLeft(width, '0'); // print out as a bitstring
}
