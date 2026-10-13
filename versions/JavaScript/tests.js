// Smoke tests for MicroMoth (JavaScript), mirroring versions/Csharp/Tests/Tests.cs
// plus the Python template's behaviour (noise model, probabilities_dict, memory, post-measure check).
// Run with:  npm test   (or: node tests.js)
// Exits non-zero if any check fails, so CI can use it as a publish gate.

'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');
const { QuantumCircuit, simulate, kron, norm } = require('./micromoth.js');

const R2 = Math.SQRT1_2;
let failures = 0;

function check(name, ok) {
    console.log(`  [${ok ? 'PASS' : 'FAIL'}] ${name}`);
    if (!ok) failures++;
}

function approx(a, b, tol = 1e-9) {
    return Math.abs(a - b) < tol;
}

function throws(fn, type = Error) {
    try { fn(); } catch (e) { return e instanceof type; }
    return false;
}

function sameStatevector(a, b, tol = 1e-9) {
    return a.length === b.length && a.every(([r, i], j) => approx(r, b[j][0], tol) && approx(i, b[j][1], tol));
}

const counts = (qc, shots, seed = 1, noise = []) => simulate(qc, shots, 'counts', noise, seed);
const statevector = (qc) => simulate(qc, 0, 'statevector');
const keys = (o) => Object.keys(o).sort();

async function main() {
    console.log('Gates');

    // X flips |0> -> |1>
    {
        const c = counts(new QuantumCircuit(1, 1).x(0).measure_all(), 1000);
        check('X: all ones', keys(c).join() === '1' && c['1'] === 1000);
    }

    // H gives a balanced superposition
    {
        const c = counts(new QuantumCircuit(1, 1).h(0).measure_all(), 1000);
        check('H: two balanced outcomes', keys(c).length === 2 && c['0'] > 400 && c['0'] < 600);
    }

    // Bell statevector amplitudes (phase-sensitive: counts can't see this)
    {
        const a = statevector(new QuantumCircuit(2).h(0).cx(0, 1));
        check('Bell statevector = [.707, 0, 0, .707]', sameStatevector(a, [[R2, 0], [0, 0], [0, 0], [R2, 0]]));
    }

    // Bell counts: perfectly correlated
    {
        const c = counts(new QuantumCircuit(2, 2).h(0).cx(0, 1).measure_all(), 2000);
        check('Bell: only 00 and 11', keys(c).join() === '00,11');
    }

    // GHZ counts: three-way correlation
    {
        const c = counts(new QuantumCircuit(3, 3).h(0).cx(0, 1).cx(1, 2).measure_all(), 2000);
        check('GHZ: only 000 and 111', keys(c).join() === '000,111');
    }

    // Endianness: setting qubit 0 must read as the rightmost bit
    {
        const c = counts(new QuantumCircuit(2, 2).x(0).measure_all(), 100);
        check("Endianness: x(0) -> '01'", keys(c).join() === '01');
    }

    // Swap exchanges qubit contents
    {
        const c = counts(new QuantumCircuit(2, 2).x(0).swap(0, 1).measure_all(), 100);
        check("Swap: x(0) then swap -> '10'", keys(c).join() === '10');
    }

    // Ry(pi/2) on |0> -> [.707, .707], real and positive (catches a wrong rz sign inside ry)
    {
        const a = statevector(new QuantumCircuit(1).ry(Math.PI / 2, 0));
        check('Ry(pi/2): [.707, .707] real', sameStatevector(a, [[R2, 0], [R2, 0]]));
    }

    // Rz is a pure phase: probabilities on |+> are unchanged
    {
        const p = simulate(new QuantumCircuit(1).h(0).rz(Math.PI / 3, 0), 0, 'probabilities_dict');
        check('Rz: phase only (probs 0.5/0.5)', approx(p['0'], 0.5) && approx(p['1'], 0.5));
    }

    // crz only phases the control-1 half: |++> -> [.5, -.5i, .5, .5i] for crz(pi, 0, 1)
    {
        const a = statevector(new QuantumCircuit(2).h(0).h(1).crz(Math.PI, 0, 1));
        check('crz: controlled phase', sameStatevector(a, [[0.5, 0], [0, -0.5], [0.5, 0], [0, 0.5]]));
    }

    // Every gate, with controls above and below targets, against the Python template's statevector
    {
        const qc = new QuantumCircuit(3)
            .h(0).rx(0.7, 1).rz(1.1, 0).ry(0.3, 2).y(1).t(0).z(2)
            .cx(2, 0).crx(0.9, 2, 1).swap(0, 2).crx(0.4, 0, 2).cx(1, 2).h(2);
        const expected = [
            [0, -0.19922637949540753],
            [-0.0017416246978755984, -0.008591704369767583],
            [-0.5457830890379092, 0],
            [0.08563046244323613, -0.017358154082492475],
            [-0.27433922922096493, 0],
            [0.011830971181960227, 0.0023982565883972308],
            [0, -0.7515556541645066],
            [0.023902570652293995, -0.11791508295242457],
        ];
        check('All gates: matches Python template statevector', sameStatevector(statevector(qc), expected));
    }

    console.log('Measurement and outputs');

    // measure(q, b) with q != b routes each qubit to the requested bit
    {
        const qc = new QuantumCircuit(2, 2).x(0).measure(0, 1).measure(1, 0);
        check("measure(0,1): qubit 0 lands on bit 1 -> '10'", keys(counts(qc, 50)).join() === '10');
    }

    // A classical bit that is never written reads 0
    {
        const qc = new QuantumCircuit(2, 2).x(0).x(1).measure(0, 0);
        check("Unmeasured clbit reads 0 -> '01'", keys(counts(qc, 50)).join() === '01');
    }

    // Partial measurement into a smaller register
    {
        const qc = new QuantumCircuit(3, 1).x(2).measure(2, 0);
        check("Partial: measure(2,0) on 3 qubits -> '1'", keys(counts(qc, 50)).join() === '1');
    }

    // measure_all creates the classical register when none exists
    {
        const qc = new QuantumCircuit(2).h(0).cx(0, 1).measure_all();
        check('measure_all: creates 2 clbits', qc.num_clbits === 2 && keys(counts(qc, 500)).join() === '00,11');
    }

    // Sampling frequencies follow the probabilities: ry(1) gives p(1) = sin^2(0.5)
    {
        const c = counts(new QuantumCircuit(1, 1).ry(1, 0).measure_all(), 20000);
        check('Sampling: frequency within 0.02 of sin^2(0.5)', approx(c['1'] / 20000, Math.sin(0.5) ** 2, 0.02));
    }

    // probabilities_dict lists every basis state with n-bit keys
    {
        const p = simulate(new QuantumCircuit(2).x(1), 0, 'probabilities_dict');
        check('probabilities_dict: 4 keys, |10> certain',
            keys(p).join() === '00,01,10,11' && approx(p['10'], 1) && approx(p['00'], 0));
        const legacy = simulate(new QuantumCircuit(2).x(1), 0, 'probability_dictionary');
        check("probabilities_dict: 1.0 spelling 'probability_dictionary' still works", approx(legacy['10'], 1));
    }

    // memory returns one bitstring per shot
    {
        const m = simulate(new QuantumCircuit(1, 1).h(0).measure_all(), 300, 'memory', [], 3);
        check('memory: one entry per shot, only 0/1', m.length === 300 && m.every((s) => s === '0' || s === '1'));
    }

    // Same seed -> identical sampling
    {
        const qc = new QuantumCircuit(2, 2).h(0).cx(0, 1).measure_all();
        const a = JSON.stringify(counts(qc, 500, 7)), b = JSON.stringify(counts(qc, 500, 7));
        const ma = simulate(qc, 50, 'memory', [], 9).join(), mb = simulate(qc, 50, 'memory', [], 9).join();
        check('Determinism: same seed reproduces counts and memory', a === b && ma === mb);
    }

    console.log('Noise model');

    // A single probability applies to every qubit
    {
        const p = simulate(new QuantumCircuit(2), 0, 'probabilities_dict', 0.1);
        check('Noise float: 0.81 / 0.09 / 0.09 / 0.01',
            approx(p['00'], 0.81) && approx(p['01'], 0.09) && approx(p['10'], 0.09) && approx(p['11'], 0.01));
    }

    // A list gives each qubit its own error probability
    {
        const p = simulate(new QuantumCircuit(2).x(0), 0, 'probabilities_dict', [0.2, 0]);
        check('Noise list: only qubit 0 flips', approx(p['01'], 0.8) && approx(p['00'], 0.2) && approx(p['10'], 0));
    }

    check('Noise list: wrong length throws',
        throws(() => simulate(new QuantumCircuit(2), 0, 'probabilities_dict', [0.1]), RangeError));

    console.log('Initialisation');

    // Real-only amplitudes are accepted, like the Python template
    {
        const a = statevector(new QuantumCircuit(1).initialize([R2, R2]));
        check('initialize: real amplitudes', sameStatevector(a, [[R2, 0], [R2, 0]]));
    }

    // initialize discards earlier gates
    {
        const qc = new QuantumCircuit(1).x(0).initialize([[1, 0], [0, 0]]);
        check('initialize: clears earlier gates', qc.data.length === 1 && approx(statevector(qc)[0][0], 1));
    }

    check('initialize: wrong length throws', throws(() => new QuantumCircuit(2).initialize([1, 0]), RangeError));
    check('initialize: all zeros throws', throws(() => new QuantumCircuit(1).initialize([0, 0]), RangeError));

    // An unnormalised state is sampled by relative weight, never as a spurious |0>
    {
        const qc = new QuantumCircuit(1, 1).initialize([[0, 0], [0.6, 0]]).measure(0, 0);
        check("initialize: unnormalised |1> samples only '1'", keys(counts(qc, 1000)).join() === '1');
    }

    // kron and norm build inputs for initialize
    {
        const plus = [[R2, 0], [R2, 0]], zero = [[1, 0], [0, 0]];
        const a = statevector(new QuantumCircuit(2).initialize(kron(zero, plus)));
        const b = statevector(new QuantumCircuit(2).h(0));
        const n = norm([[3, 0], [0, 4]]);
        check('kron: |0> (x) |+> matches h(0)', sameStatevector(a, b));
        check('norm: [3, 4i] -> [.6, .8i]', approx(n[0][0], 0.6) && approx(n[1][1], 0.8));
    }

    console.log('Composition');

    {
        const a = new QuantumCircuit(2).h(0);
        const b = new QuantumCircuit(2).cx(0, 1);
        a.name = 'left';
        const composed = a.compose(b);
        check('compose: matches sequential gates',
            sameStatevector(statevector(composed), statevector(new QuantumCircuit(2).h(0).cx(0, 1))));
        check('compose: operands untouched', a.data.length === 1 && b.data.length === 1);
        check('compose: name comes from left operand', composed.name === 'left');
    }

    {
        const sum = new QuantumCircuit(1).x(0).add(new QuantumCircuit(1).x(0));
        check('add: x then x returns to |0>', approx(statevector(sum)[0][0], 1));
        const wide = new QuantumCircuit(1, 1).x(0).add(new QuantumCircuit(3, 2).x(2));
        check('add: takes the larger register sizes', wide.num_qubits === 3 && wide.num_clbits === 2);
    }

    check('compose: smaller initialize into larger circuit throws',
        throws(() => new QuantumCircuit(2).compose(new QuantumCircuit(1).initialize([1, 0])), RangeError));

    console.log('Validation');

    check('Constructor: 0 qubits throws', throws(() => new QuantumCircuit(0), RangeError));
    check('Constructor: 21 qubits throws', throws(() => new QuantumCircuit(21), RangeError));
    check('Gate: qubit out of range throws', throws(() => new QuantumCircuit(2).x(2), RangeError));
    check('Gate: negative qubit throws', throws(() => new QuantumCircuit(2).h(-1), RangeError));
    check('Gate: fractional qubit throws', throws(() => new QuantumCircuit(2).x(0.5), RangeError));
    check('Gate: control equal to target throws', throws(() => new QuantumCircuit(2).cx(1, 1), RangeError));
    check('Gate: NaN angle throws', throws(() => new QuantumCircuit(1).rx(NaN, 0), TypeError));
    check('measure: negative clbit throws', throws(() => new QuantumCircuit(2, 2).measure(0, -1), RangeError));

    {
        const qc = new QuantumCircuit(2);
        try { qc.ry(Infinity, 0); } catch { /* expected */ }
        try { qc.crz(1, 0, 0); } catch { /* expected */ }
        check('Composite gates: a bad call adds nothing', qc.data.length === 0);
    }

    {
        const qc = new QuantumCircuit(1).h(0);
        let mutated = false;
        try { qc.data.push('junk'); mutated = true; } catch { /* frozen */ }
        check('data: read-only snapshot', !mutated && qc.data.length === 1);
    }

    console.log('Measurement order (Python template rule)');

    check('Gate after measure throws',
        throws(() => counts(new QuantumCircuit(1, 1).measure(0, 0).x(0), 10)));
    check('cx after measuring its control throws',
        throws(() => counts(new QuantumCircuit(2, 2).h(0).measure(0, 0).cx(0, 1).measure(1, 1), 10)));
    check('Gate after measure(q, b) with q != b throws',
        throws(() => counts(new QuantumCircuit(2, 2).measure(0, 1).measure(1, 0).x(0), 10)));
    check('Statevector ignores the rule',
        !throws(() => statevector(new QuantumCircuit(1, 1).measure(0, 0).x(0))));
    check('counts without classical bits throws', throws(() => counts(new QuantumCircuit(1).h(0), 10)));
    check('Unknown get mode throws', throws(() => simulate(new QuantumCircuit(1), 1, 'bogus'), RangeError));

    console.log('Loading');

    {
        const esm = await import('./micromoth.js');
        check('Node ESM: named imports', typeof esm.QuantumCircuit === 'function' && typeof esm.simulate === 'function');
    }

    {
        const ctx = vm.createContext({});
        vm.runInContext(fs.readFileSync(path.join(__dirname, 'micromoth.js'), 'utf8'), ctx);
        check('Browser <script>: globals for p5.js', typeof ctx.QuantumCircuit === 'function' && typeof ctx.simulate === 'function');
    }

    console.log(failures === 0 ? '\nAll smoke tests passed.' : `\n${failures} test(s) FAILED.`);
    process.exitCode = failures === 0 ? 0 : 1;
}

main().catch((err) => {
    console.error(err);
    process.exitCode = 1;
});
