// Tests for MicroMoth (JavaScript).
// Gate results, bit order and the measurement rule are checked by the shared cases in tests/conformance.
// This file checks what is specific to the JavaScript version: its API, its errors and how it loads.
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
    console.log('Gates and outputs');

    // crz is only in the JavaScript and Arduino versions, so the shared cases do not cover it.
    check('crz: controlled phase',
        sameStatevector(statevector(new QuantumCircuit(2).h(0).h(1).crz(Math.PI, 0, 1)), [[0.5, 0], [0, -0.5], [0.5, 0], [0, 0.5]]));
    check('Bell sampling: only 00 and 11', keys(counts(new QuantumCircuit(2, 2).h(0).cx(0, 1).measure_all(), 2000)).join() === '00,11');
    {
        const c = counts(new QuantumCircuit(1, 1).ry(1, 0).measure_all(), 20000);
        check('Sampling: frequency within 0.02 of sin^2(0.5)', approx(c['1'] / 20000, Math.sin(0.5) ** 2, 0.02));
    }
    {
        const p = simulate(new QuantumCircuit(2).x(1), 0, 'probabilities_dict');
        check('probabilities_dict: 4 keys, |10> certain', keys(p).join() === '00,01,10,11' && approx(p['10'], 1));
        check("probabilities_dict: 1.0 spelling 'probability_dictionary' still works",
            approx(simulate(new QuantumCircuit(2).x(1), 0, 'probability_dictionary')['10'], 1));
    }
    {
        const m = simulate(new QuantumCircuit(1, 1).h(0).measure_all(), 300, 'memory', [], 3);
        check('memory: one entry per shot, only 0/1', m.length === 300 && m.every((s) => s === '0' || s === '1'));
    }
    {
        const qc = new QuantumCircuit(2, 2).h(0).cx(0, 1).measure_all();
        check('Determinism: same seed reproduces counts and memory',
            JSON.stringify(counts(qc, 500, 7)) === JSON.stringify(counts(qc, 500, 7))
            && simulate(qc, 50, 'memory', [], 9).join() === simulate(qc, 50, 'memory', [], 9).join());
    }

    console.log('Noise model');
    {
        const p = simulate(new QuantumCircuit(2), 0, 'probabilities_dict', 0.1);
        check('Noise as one number: 0.81 / 0.09 / 0.09 / 0.01',
            approx(p['00'], 0.81) && approx(p['01'], 0.09) && approx(p['10'], 0.09) && approx(p['11'], 0.01));
    }
    check('Noise list of the wrong length throws',
        throws(() => simulate(new QuantumCircuit(2), 0, 'probabilities_dict', [0.1]), RangeError));

    console.log('Initialisation');
    check('initialize: real amplitudes', sameStatevector(statevector(new QuantumCircuit(1).initialize([R2, R2])), [[R2, 0], [R2, 0]]));
    {
        const qc = new QuantumCircuit(1).x(0).initialize([[1, 0], [0, 0]]);
        check('initialize: clears earlier gates', qc.data.length === 1 && approx(statevector(qc)[0][0], 1));
    }
    check('initialize: all zeros throws', throws(() => new QuantumCircuit(1).initialize([0, 0]), RangeError));
    check("initialize: unnormalised |1> samples only '1'",
        keys(counts(new QuantumCircuit(1, 1).initialize([[0, 0], [0.6, 0]]).measure(0, 0), 1000)).join() === '1');
    {
        const plus = [[R2, 0], [R2, 0]], zero = [[1, 0], [0, 0]];
        check('kron: |0> (x) |+> matches h(0)',
            sameStatevector(statevector(new QuantumCircuit(2).initialize(kron(zero, plus))), statevector(new QuantumCircuit(2).h(0))));
        const n = norm([[3, 0], [0, 4]]);
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
        const wide = new QuantumCircuit(1, 1).x(0).add(new QuantumCircuit(3, 2).x(2));
        check('add: takes the larger register sizes', wide.num_qubits === 3 && wide.num_clbits === 2);
        check('compose: smaller initialize into larger circuit throws',
            throws(() => new QuantumCircuit(2).compose(new QuantumCircuit(1).initialize([1, 0])), RangeError));
    }

    console.log('Validation');
    check('Constructor: 0 qubits throws', throws(() => new QuantumCircuit(0), RangeError));
    check('Constructor: 21 qubits throws', throws(() => new QuantumCircuit(21), RangeError));
    check('Gate: fractional qubit throws', throws(() => new QuantumCircuit(2).x(0.5), RangeError));
    check('Gate: NaN angle throws', throws(() => new QuantumCircuit(1).rx(NaN, 0), TypeError));
    check('measure: negative clbit throws', throws(() => new QuantumCircuit(2, 2).measure(0, -1), RangeError));
    {
        const qc = new QuantumCircuit(2);
        try { qc.ry(Infinity, 0); } catch { /* expected */ }
        try { qc.crz(1, 0, 0); } catch { /* expected */ }
        check('Composite gates: a bad call adds nothing', qc.data.length === 0);
    }
    check('Statevector ignores the measurement rule', !throws(() => statevector(new QuantumCircuit(1, 1).measure(0, 0).x(0))));
    check('Unknown get mode throws', throws(() => simulate(new QuantumCircuit(1), 1, 'bogus'), RangeError));

    console.log('Conformance (tests/conformance/cases.json)');
    {
        const file = path.join(__dirname, '..', '..', 'tests', 'conformance', 'cases.json');
        if (!fs.existsSync(file)) {
            console.log('  cases.json not found, skipped');
        } else {
            const cases = JSON.parse(fs.readFileSync(file, 'utf8'));
            const tol = cases.tolerance.double;
            const build = (c) => {
                const qc = new QuantumCircuit(c.qubits, c.clbits);
                for (const [name, ...args] of c.ops) qc[name](...args);
                return qc;
            };
            const bits = (j, n) => j.toString(2).padStart(n, '0');
            let agreed = 0, total = 0;
            const tally = (name, ok) => {
                total++;
                if (ok) agreed++; else check(`Conformance: ${name}`, false);
            };
            for (const c of cases.statevector) tally(c.name, sameStatevector(statevector(build(c)), c.expected, tol));
            for (const c of cases.probabilities) {
                const p = simulate(build(c), 0, 'probabilities_dict', c.noise);
                tally(c.name, c.expected.every((want, j) => approx(p[bits(j, c.qubits)], want, tol)));
            }
            for (const c of cases.counts) tally(c.name, JSON.stringify(counts(build(c), c.shots)) === JSON.stringify(c.expected));
            for (const c of cases.errors) tally(`rejects ${c.name}`, throws(() => simulate(build(c), 4, c.get, [], 1)));
            check(`Conformance: ${agreed} of ${total} shared cases agree`, agreed === total);
        }
    }

    console.log('Loading');
    {
        const esm = await import('./micromoth.js');
        check('Node ESM: named imports', typeof esm.QuantumCircuit === 'function' && typeof esm.simulate === 'function');
    }
    {
        const src = fs.readFileSync(path.join(__dirname, 'micromoth.js'), 'utf8');
        const ctx = vm.createContext({});
        vm.runInContext(src, ctx);
        check('Browser <script>: globals for p5.js', typeof ctx.QuantumCircuit === 'function' && typeof ctx.simulate === 'function');
        check('Browser <script>: no global norm or kron to clash with p5.js',
            !('norm' in ctx) && !('kron' in ctx) && typeof ctx.MicroMoth.norm === 'function');
        // The p5.js Editor cannot run files with private class members, which are written with a # prefix.
        check('p5.js Editor: no private class members', !/#[A-Za-z_$]/.test(src));
    }

    console.log(failures === 0 ? '\nAll smoke tests passed.' : `\n${failures} test(s) FAILED.`);
    process.exitCode = failures === 0 ? 0 : 1;
}

main().catch((err) => {
    console.error(err);
    process.exitCode = 1;
});
