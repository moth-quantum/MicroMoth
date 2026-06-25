// (C) Copyright Moth 2026.
// Apache-2.0
using System.Numerics;
namespace Moth.MicroMoth;
public enum GateOp { init, x, h, rx, rz, cx, crx, swap, m }

public readonly record struct Gate(
    // single-qubit gates: A means the target.
    // two-qubit gates: A means the control qubit and B means the target qubit.
    // measurement gate: A stands for the qubits and B is for classical bits.
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
    public QuantumCircuit X(int q)
    {
        Data.Add(new Gate(GateOp.x, A: q));
        return this;
    }

    public QuantumCircuit H(int q)
    {
        Data.Add(new Gate(GateOp.h, A: q));
        return this;
    }

    public QuantumCircuit Rx(double th, int q)
    {
        Data.Add(new Gate(GateOp.rx, Theta:th, A: q));
        return this;
    }

    public QuantumCircuit Rz(double th, int q)
    {
        Data.Add(new Gate(GateOp.rz, Theta: th, A: q));
        return this;
    }

    public QuantumCircuit Cx(int s, int t)
    {
        Data.Add(new Gate(GateOp.rz, A: s, B: t));
        return this;
    }

    public QuantumCircuit Crx(double th, int s, int t)
    {
        Data.Add(new Gate(GateOp.crx, Theta: th, A: s, B: t));
        return this;
    }

    public QuantumCircuit Swap(int s , int t)
    {
        Data.Add(new Gate(GateOp.swap, A: s, B: t));
        return this;
    }

    // ===== (Slightly) advanced gates as a combo of basic gates =====
    public QuantumCircuit Ry(double th, int q)
    {
        Rx(Math.PI / 2, q);
        Rz(th, q);
        Rx(-Math.PI / 2, q);
        return this;
    }

    public QuantumCircuit Z(int q)
    {
        return Rz(Math.PI, q);
    }

    public QuantumCircuit T(int q)
    {
        return Rz(Math.PI / 4, q);
    }

    public QuantumCircuit Y(int q)
    {
        Rz(Math.PI, q);
        X(q);
        return this;
    }

    // Measurement gate: These can be wrapped around Node's GD.Print on Godot 4.x.
    public QuantumCircuit Measure(int q , int b)
    {
        if (b >= Clbits) throw new ArgumentOutOfRangeException(nameof(b), "Classical bit is out of range.");
        if (q >= Qubits) throw new ArgumentOutOfRangeException(nameof(q), "Qubit is out of range.");
        Data.Add(new Gate(GateOp.m, A: q, B: b));
        return this;
    }

    public QuantumCircuit MeasureAll()
    {
        if (Clbits == 0) Clbits = Qubits; // create the classical bit storage if absent
        for (int q = 0; q < Qubits; q++) Measure(q, q);
        return this;
    }

    // Initialisation
    public QuantumCircuit Init(Complex[] state)
    {
        int dim = 1 << Qubits;
        if (state.Length != dim) throw new ArgumentException($"statevector must have {dim}-dimension of amplitudes.", nameof(state));
        
        Data.Clear(); // wipes prior gates
        Data.Add(new Gate(GateOp.init, State: (Complex[])state.Clone()));
        return this;
    }
}

public class MicroMoth
{

}
