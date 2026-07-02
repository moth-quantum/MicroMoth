// Lightweight smoke tests for MicroMoth (C#), in the spirit of the other ports' tests.js / tests.lua.
// Run with:  dotnet run --project versions/Csharp/Tests -c Release
// Exits non-zero if any check fails, so CI can use it as a publish gate.

using System.Numerics;
using Moth.MicroMoth;

const double R2 = 0.70710678118654752440; // 1/sqrt(2)
int failures = 0;

void Check(string name, bool ok)
{
    Console.WriteLine($"  [{(ok ? "PASS" : "FAIL")}] {name}");
    if (!ok) failures++;
}
bool Approx(double a, double b, double tol = 1e-9) => Math.Abs(a - b) < tol;

// X flips |0> -> |1>
{
    var c = Simulator.Counts(new QuantumCircuit(1, 1).X(0).MeasureAll(), 1000, seed: 1);
    Check("X: all ones", c.Count == 1 && c.GetValueOrDefault("1") == 1000);
}

// H gives a balanced superposition
{
    var c = Simulator.Counts(new QuantumCircuit(1, 1).H(0).MeasureAll(), 1000, seed: 1);
    Check("H: two balanced outcomes", c.Count == 2 && c.GetValueOrDefault("0") is > 400 and < 600);
}

// Bell statevector amplitudes (phase-sensitive: counts can't see this)
{
    var a = Simulator.Statevector(new QuantumCircuit(2).H(0).Cx(0, 1));
    Check("Bell statevector = [.707, 0, 0, .707]",
        Approx(a[0].Real, R2) && Approx(a[3].Real, R2) && Approx(a[1].Magnitude, 0) && Approx(a[2].Magnitude, 0));
}

// Bell counts: perfectly correlated
{
    var c = Simulator.Counts(new QuantumCircuit(2, 2).H(0).Cx(0, 1).MeasureAll(), 2000, seed: 1);
    Check("Bell: only 00 and 11", c.Keys.All(k => k is "00" or "11") && c.ContainsKey("00") && c.ContainsKey("11"));
}

// GHZ counts: three-way correlation
{
    var c = Simulator.Counts(new QuantumCircuit(3, 3).H(0).Cx(0, 1).Cx(1, 2).MeasureAll(), 2000, seed: 1);
    Check("GHZ: only 000 and 111", c.Keys.All(k => k is "000" or "111"));
}

// Endianness: setting qubit 0 must read as the rightmost bit
{
    var c = Simulator.Counts(new QuantumCircuit(2, 2).X(0).MeasureAll(), 100, seed: 1);
    Check("Endianness: X(0) -> '01'", c.Count == 1 && c.ContainsKey("01"));
}

// Swap exchanges qubit contents
{
    var c = Simulator.Counts(new QuantumCircuit(2, 2).X(0).Swap(0, 1).MeasureAll(), 100, seed: 1);
    Check("Swap: X(0) then Swap -> '10'", c.Count == 1 && c.ContainsKey("10"));
}

// Ry(pi/2) on |0> -> [.707, .707], real and positive (catches a wrong Rz sign inside Ry)
{
    var a = Simulator.Statevector(new QuantumCircuit(1).Ry(Math.PI / 2, 0));
    Check("Ry(pi/2): [.707, .707] real",
        Approx(a[0].Real, R2) && Approx(a[1].Real, R2) && Approx(a[0].Imaginary, 0) && Approx(a[1].Imaginary, 0));
}

// Rz is a pure phase: probabilities on |+> are unchanged
{
    var p = Simulator.Probabilities(new QuantumCircuit(1).H(0).Rz(Math.PI / 3, 0));
    Check("Rz: phase only (probs 0.5/0.5)", Approx(p["0"], 0.5) && Approx(p["1"], 0.5));
}

// Same seed -> identical sampling
{
    var qc = new QuantumCircuit(2, 2).H(0).Cx(0, 1).MeasureAll();
    Check("Determinism: same seed reproduces counts",
        Simulator.Counts(qc, 500, seed: 7).SequenceEqual(Simulator.Counts(qc, 500, seed: 7)));
}

// composition vs serialisation
{
    var a = new QuantumCircuit(2).H(0);
    var b = new QuantumCircuit(2).Cx(0, 1);
    var composed = a.Compose(b);

    var expected = Simulator.Statevector(new QuantumCircuit(2).H(0).Cx(0, 1));
    var outcome = Simulator.Statevector(composed);

    bool isSame = true;
    for (int j = 0; j < expected.Length; j++) isSame &= Approx((outcome[j] - expected[j]).Magnitude, 0);

    Check("Compose: matches sequential gates", isSame);
    Check("Compose: operands untouched", a.Data.Count == 1 && b.Data.Count == 1);
    Check("Compose: name comes from left operand", composed.Name == a.Name);
}

// operator + testing: Is it doing the composition well?
{
    var sum = new QuantumCircuit(1).X(0) + new QuantumCircuit(1).X(0);
    Check("operator +: X then X returns to |0>", Approx(Simulator.Statevector(sum)[0].Magnitude, 1));
}

Console.WriteLine(failures == 0 ? "\nAll smoke tests passed." : $"\n{failures} test(s) FAILED.");
return failures == 0 ? 0 : 1;
