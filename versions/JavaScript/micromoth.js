// (C) Copyright Moth Quantum 2026.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

// (C) Copyright @kaleb-hutchy 2020.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

// MicroMoth for JavaScript.
//
// Behaviour follows the commented Python template (micromoth.py at the repo root).
// Structure follows the C# port (versions/Csharp/MicroMoth/MicroMoth.cs):
// one immutable gate record, validated gate methods, a statevector held in typed
// arrays, a clbit -> qubit map, integer outcomes and a seedable random generator.
//
// The same file works as a plain browser <script> (including p5.js global mode),
// with Node's require(), and with Node's ESM `import { ... } from`.

(function (root) {
    'use strict';

    const SQRT1_2 = Math.SQRT1_2; // 1/sqrt(2), used by the h gate.
    const MAX_QUBITS = 20;        // 2^20 amplitudes = 16 MB of Float64 storage.
    const MAX_CLBITS = 30;        // Outcomes are packed into 31-bit integers.

    // Gate operations. The values are the tags used by the Python template.
    const Op = Object.freeze({
        INIT: 'init', X: 'x', H: 'h', RX: 'rx', RZ: 'rz',
        CX: 'cx', CRX: 'crx', SWAP: 'swap', M: 'm',
    });

    const GET_MODES = ['statevector', 'probabilities_dict', 'counts', 'memory'];

    // Every gate is stored in the same immutable record shape (the C# `Gate` record).
    //   single-qubit gates: a = target qubit
    //   two-qubit gates:    a = control (source) qubit, b = target qubit
    //   measure:            a = qubit, b = classical bit
    //   init:               state = { re, im } Float64Arrays
    function gate(op, a = 0, b = 0, theta = 0, state = null) {
        return Object.freeze({ op, theta, a, b, state });
    }

    function checkAngle(theta) {
        if (typeof theta !== 'number' || !Number.isFinite(theta)) {
            throw new TypeError(`Angle must be a finite number, got ${theta}.`);
        }
        return theta;
    }

    function isNumber(v) {
        return typeof v === 'number' && Number.isFinite(v);
    }

    class QuantumCircuit {
        #qubits;
        #clbits;
        #data = [];

        constructor(n, m = 0) {
            if (!Number.isInteger(n) || n < 1 || n > MAX_QUBITS) {
                throw new RangeError(`num_qubits must be an integer from 1 to ${MAX_QUBITS}, got ${n}.`);
            }
            if (!Number.isInteger(m) || m < 0 || m > MAX_CLBITS) {
                throw new RangeError(`num_clbits must be an integer from 0 to ${MAX_CLBITS}, got ${m}.`);
            }
            this.#qubits = n;
            this.#clbits = m;
            this.name = '';
        }

        get num_qubits() { return this.#qubits; }
        get num_clbits() { return this.#clbits; }

        // A read-only snapshot of the gate list, so callers cannot corrupt the circuit.
        get data() { return Object.freeze(this.#data.slice()); }
        getData() { return this.data; } // Kept for code written against MicroMoth JS 1.0.

        // ===== Validation =====

        #qubit(q, label = 'Qubit') {
            if (!Number.isInteger(q) || q < 0 || q >= this.#qubits) {
                throw new RangeError(`${label} index ${q} is out of range for ${this.#qubits} qubit(s).`);
            }
            return q;
        }

        #pair(s, t) {
            this.#qubit(s, 'Control qubit');
            this.#qubit(t, 'Target qubit');
            if (s === t) throw new RangeError(`Control and target qubits must differ, both are ${s}.`);
        }

        #push(g) {
            this.#data.push(g);
            return this;
        }

        // ===== Basic gates: the only ones the simulator knows =====

        x(q)               { return this.#push(gate(Op.X, this.#qubit(q))); }
        h(q)               { return this.#push(gate(Op.H, this.#qubit(q))); }
        rx(theta, q)       { return this.#push(gate(Op.RX, this.#qubit(q), 0, checkAngle(theta))); }
        rz(theta, q)       { return this.#push(gate(Op.RZ, this.#qubit(q), 0, checkAngle(theta))); }
        cx(s, t)           { this.#pair(s, t); return this.#push(gate(Op.CX, s, t)); }
        crx(theta, s, t)   { this.#pair(s, t); return this.#push(gate(Op.CRX, s, t, checkAngle(theta))); }
        swap(s, t)         { this.#pair(s, t); return this.#push(gate(Op.SWAP, s, t)); }

        // ===== Gates built from the basic ones =====
        // Arguments are checked before the first push, so a bad call never leaves half a gate behind.

        ry(theta, q) {
            checkAngle(theta);
            this.#qubit(q);
            return this.rx(Math.PI / 2, q).rz(theta, q).rx(-Math.PI / 2, q);
        }

        z(q) { return this.rz(Math.PI, q); }
        t(q) { return this.rz(Math.PI / 4, q); }
        y(q) { this.#qubit(q); return this.rz(Math.PI, q).x(q); }

        // Controlled rz, using h·rx·h = rz on the target. Not in the Python template; kept from MicroMoth JS 1.0.
        crz(theta, s, t) {
            checkAngle(theta);
            this.#pair(s, t);
            return this.h(t).crx(theta, s, t).h(t);
        }

        // ===== Measurement =====

        measure(q, b) {
            this.#qubit(q);
            if (!Number.isInteger(b) || b < 0 || b >= this.#clbits) {
                throw new RangeError(`Classical bit index ${b} is out of range for ${this.#clbits} classical bit(s).`);
            }
            return this.#push(gate(Op.M, q, b));
        }

        measure_all() {
            if (this.#clbits === 0) this.#clbits = this.#qubits; // Create the classical register if absent.
            if (this.#clbits < this.#qubits) {
                throw new RangeError(`measure_all needs at least ${this.#qubits} classical bits, the circuit has ${this.#clbits}.`);
            }
            for (let q = 0; q < this.#qubits; q++) this.measure(q, q);
            return this;
        }

        // ===== Initialisation =====

        // Takes 2^num_qubits amplitudes, each a real number or a [re, im] pair.
        // Like Python and C#, this discards any gates added before it.
        initialize(state) {
            const dim = 1 << this.#qubits;
            if (!Array.isArray(state) && !ArrayBuffer.isView(state)) {
                throw new TypeError('initialize expects an array of amplitudes.');
            }
            if (state.length !== dim) {
                throw new RangeError(`Statevector must have ${dim} amplitudes, got ${state.length}.`);
            }

            const re = new Float64Array(dim), im = new Float64Array(dim);
            let total = 0;
            for (let i = 0; i < dim; i++) {
                const e = state[i];
                if (isNumber(e)) {
                    re[i] = e;
                } else if (Array.isArray(e) && e.length === 2 && isNumber(e[0]) && isNumber(e[1])) {
                    re[i] = e[0];
                    im[i] = e[1];
                } else {
                    throw new TypeError(`Amplitude ${i} must be a finite number or a [re, im] pair.`);
                }
                total += re[i] * re[i] + im[i] * im[i];
            }
            if (total === 0) throw new RangeError('Statevector must not be all zeros.');

            this.#data.length = 0;
            return this.#push(gate(Op.INIT, 0, 0, 0, { re, im }));
        }

        // ===== Composition =====

        // Returns a new circuit: this circuit's gates followed by the other's. Neither operand changes.
        compose(other) {
            if (!(other instanceof QuantumCircuit)) throw new TypeError('compose expects a QuantumCircuit.');

            const out = new QuantumCircuit(
                Math.max(this.#qubits, other.#qubits),
                Math.max(this.#clbits, other.#clbits),
            );
            out.name = this.name;
            out.#data = this.#data.concat(other.#data);

            const dim = 1 << out.#qubits;
            for (const g of out.#data) {
                if (g.op === Op.INIT && g.state.re.length !== dim) {
                    throw new RangeError('Cannot compose: an initialize() on fewer qubits would not cover the combined circuit.');
                }
            }
            return out;
        }

        // The Python template's `+`.
        add(other) { return this.compose(other); }
    }

    // ===== Maths helpers. Each updates a pair of amplitudes, indices i and j, in place. =====

    // x: swap the two amplitudes.
    function exchange(re, im, i, j) {
        const r = re[i], m = im[i];
        re[i] = re[j]; im[i] = im[j];
        re[j] = r;     im[j] = m;
    }

    // h: (x + y)/sqrt(2) and (x - y)/sqrt(2).
    function superpose(re, im, i, j) {
        const xr = re[i], xi = im[i], yr = re[j], yi = im[j];
        re[i] = SQRT1_2 * (xr + yr); im[i] = SQRT1_2 * (xi + yi);
        re[j] = SQRT1_2 * (xr - yr); im[j] = SQRT1_2 * (xi - yi);
    }

    // rx: cos(θ/2)·x − i·sin(θ/2)·y and cos(θ/2)·y − i·sin(θ/2)·x, with c = cos(θ/2), s = sin(θ/2).
    function turn(re, im, i, j, c, s) {
        const xr = re[i], xi = im[i], yr = re[j], yi = im[j];
        re[i] = c * xr + s * yi; im[i] = c * xi - s * yr;
        re[j] = c * yr + s * xi; im[j] = c * yi - s * xr;
    }

    // rz: e^(−iθ/2)·x and e^(iθ/2)·y.
    function phaseturn(re, im, i, j, c, s) {
        const xr = re[i], xi = im[i], yr = re[j], yi = im[j];
        re[i] = c * xr + s * xi; im[i] = c * xi - s * xr;
        re[j] = c * yr - s * yi; im[j] = c * yi + s * yr;
    }

    // ===== Gate application =====

    // A single-qubit gate mixes amplitude pairs whose indices differ only at the target bit.
    function applySingle(re, im, n, g) {
        const target = g.a, step = 1 << target, blocks = 1 << (n - target - 1);
        const c = Math.cos(g.theta / 2), s = Math.sin(g.theta / 2); // Once per gate, not per pair.
        for (let high = 0; high < blocks; high++) {
            const start = high << (target + 1);
            for (let i0 = start; i0 < start + step; i0++) {
                const i1 = i0 + step; // Same index with the target bit set.
                switch (g.op) {
                    case Op.X:  exchange(re, im, i0, i1); break;
                    case Op.H:  superpose(re, im, i0, i1); break;
                    case Op.RX: turn(re, im, i0, i1, c, s); break;
                    default:    phaseturn(re, im, i0, i1, c, s); // Op.RZ
                }
            }
        }
    }

    // A two-qubit gate visits each group of four amplitudes that differ only at the control and target bits.
    function applyControlled(re, im, n, g) {
        const s = g.a, t = g.b;
        const lo = Math.min(s, t), hi = Math.max(s, t);
        const sBit = 1 << s, tBit = 1 << t;
        const c = Math.cos(g.theta / 2), sn = Math.sin(g.theta / 2);
        const lowCount = 1 << lo, midCount = 1 << (hi - lo - 1), highCount = 1 << (n - hi - 1);

        for (let high = 0; high < highCount; high++) {
            for (let mid = 0; mid < midCount; mid++) {
                const base = (mid << (lo + 1)) + (high << (hi + 1));
                for (let low = 0; low < lowCount; low++) {
                    const b00 = base + low;   // control 0, target 0
                    const b10 = b00 + sBit;   // control 1, target 0
                    const b11 = b10 + tBit;   // control 1, target 1
                    switch (g.op) {
                        case Op.CX:  exchange(re, im, b10, b11); break;
                        case Op.CRX: turn(re, im, b10, b11, c, sn); break;
                        default:     exchange(re, im, b00 + tBit, b10); // Op.SWAP
                    }
                }
            }
        }
    }

    // Runs every gate on |0...0> and returns the statevector as two typed arrays.
    function evolve(n, data) {
        const dim = 1 << n;
        const re = new Float64Array(dim), im = new Float64Array(dim);
        re[0] = 1;

        for (const g of data) {
            switch (g.op) {
                case Op.INIT:
                    re.set(g.state.re);
                    im.set(g.state.im);
                    break;
                case Op.M:
                    break; // Measurements are sampled at the end.
                case Op.X: case Op.H: case Op.RX: case Op.RZ:
                    applySingle(re, im, n, g);
                    break;
                default: // Op.CX, Op.CRX, Op.SWAP
                    applyControlled(re, im, n, g);
            }
        }
        return { re, im };
    }

    // ===== Probabilities and noise =====

    // Accepts a single probability or one per qubit, as in the Python template.
    function normaliseNoise(noise_model, n) {
        if (noise_model === undefined || noise_model === null) return null;
        const list = typeof noise_model === 'number' ? new Array(n).fill(noise_model) : noise_model;
        if (!Array.isArray(list)) {
            throw new TypeError('noise_model must be a probability or an array of per-qubit probabilities.');
        }
        if (list.length === 0) return null;
        if (list.length !== n) {
            throw new RangeError(`noise_model needs one probability per qubit (${n}), got ${list.length}.`);
        }
        for (const p of list) {
            if (typeof p !== 'number' || !(p >= 0 && p <= 1)) {
                throw new RangeError(`Noise probabilities must be within [0, 1], got ${p}.`);
            }
        }
        return list;
    }

    function probabilities(re, im, n, noise) {
        const dim = re.length;
        const probs = new Float64Array(dim);
        for (let i = 0; i < dim; i++) probs[i] = re[i] * re[i] + im[i] * im[i];

        // A measurement error on qubit j mixes the probabilities of outcomes that differ only at bit j.
        if (noise) {
            for (let j = 0; j < n; j++) {
                const p = noise[j];
                if (p === 0) continue;
                const step = 1 << j, blocks = 1 << (n - j - 1);
                for (let high = 0; high < blocks; high++) {
                    const start = high << (j + 1);
                    for (let b0 = start; b0 < start + step; b0++) {
                        const b1 = b0 + step, p0 = probs[b0], p1 = probs[b1];
                        probs[b0] = (1 - p) * p0 + p * p1;
                        probs[b1] = (1 - p) * p1 + p * p0;
                    }
                }
            }
        }
        return probs;
    }

    // ===== Sampling =====

    // Measurements are deferred to the end, so no gate may touch a qubit after it is measured.
    function checkMeasuredLast(data, n) {
        const measured = new Array(n).fill(false);
        let anyMeasured = false;
        for (const g of data) {
            switch (g.op) {
                case Op.M:
                    measured[g.a] = true;
                    anyMeasured = true;
                    break;
                case Op.INIT:
                    if (anyMeasured) throw new Error('initialize() cannot follow a measurement.');
                    break;
                case Op.X: case Op.H: case Op.RX: case Op.RZ:
                    if (measured[g.a]) {
                        throw new Error(`Gate '${g.op}' acts on qubit ${g.a} after it was measured. Measure each qubit after its last gate.`);
                    }
                    break;
                default:
                    if (measured[g.a] || measured[g.b]) {
                        const q = measured[g.a] ? g.a : g.b;
                        throw new Error(`Gate '${g.op}' acts on qubit ${q} after it was measured. Measure each qubit after its last gate.`);
                    }
            }
        }
    }

    // Seedable generator (mulberry32). The same seed always gives the same samples.
    function makeRng(seed) {
        if (seed === undefined || seed === null) return Math.random;
        if (!Number.isInteger(seed)) throw new TypeError(`seed must be an integer, got ${seed}.`);
        let a = seed >>> 0;
        return function () {
            a = (a + 0x6D2B79F5) >>> 0;
            let t = a;
            t = Math.imul(t ^ (t >>> 15), t | 1);
            t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
            return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
        };
    }

    // Returns one integer per shot. Bit b of the integer is the value read into classical bit b.
    function sampleOutcomes(data, m, probs, shots, rng) {
        const dim = probs.length;

        // Cumulative distribution, searched by bisection: O(2^n + shots·n) instead of O(shots·2^n).
        const cdf = new Float64Array(dim);
        let total = 0, lastNonZero = 0;
        for (let i = 0; i < dim; i++) {
            total += probs[i];
            cdf[i] = total;
            if (probs[i] > 0) lastNonZero = i;
        }

        const map = new Int32Array(m).fill(-1); // clbit -> qubit; unmeasured clbits read 0.
        for (const g of data) if (g.op === Op.M) map[g.b] = g.a;

        const outcomes = new Int32Array(shots);
        for (let k = 0; k < shots; k++) {
            const r = rng() * total; // Scaling by total tolerates an unnormalised state.
            let lo = 0, hi = dim - 1;
            while (lo < hi) {
                const mid = (lo + hi) >>> 1;
                if (cdf[mid] > r) hi = mid; else lo = mid + 1;
            }
            if (cdf[lo] <= r) lo = lastNonZero; // Floating-point round-off guard.

            let out = 0;
            for (let b = 0; b < m; b++) {
                if (map[b] >= 0) out |= ((lo >> map[b]) & 1) << b;
            }
            outcomes[k] = out;
        }
        return outcomes;
    }

    function toBits(value, width) {
        return width === 0 ? '' : value.toString(2).padStart(width, '0');
    }

    // ===== Public entry point =====

    // simulate(qc, shots, get, noise_model, seed)
    //   get = 'statevector'        -> array of [re, im] pairs, one per basis state
    //         'probabilities_dict' -> { bitstring: probability } for every basis state
    //         'counts'             -> { bitstring: count } over `shots` samples
    //         'memory'             -> array of `shots` bitstrings, in sampling order
    //   Bitstrings put qubit / classical bit 0 on the right, as in Qiskit.
    function simulate(qc, shots = 1024, get = 'counts', noise_model = [], seed = undefined) {
        if (!(qc instanceof QuantumCircuit)) throw new TypeError('simulate expects a QuantumCircuit.');
        if (get === 'probability_dictionary') get = 'probabilities_dict'; // MicroMoth JS 1.0 spelling.
        if (!GET_MODES.includes(get)) {
            throw new RangeError(`get must be one of ${GET_MODES.map((g) => `'${g}'`).join(', ')}, got '${get}'.`);
        }

        const n = qc.num_qubits, m = qc.num_clbits;
        const data = qc.data;
        const sampling = get === 'counts' || get === 'memory';
        const noise = get === 'statevector' ? null : normaliseNoise(noise_model, n);
        let rng = null;

        if (sampling) {
            if (!Number.isInteger(shots) || shots < 0) {
                throw new RangeError(`shots must be a non-negative integer, got ${shots}.`);
            }
            if (m === 0) {
                throw new Error(`'${get}' needs classical bits. Call measure() or measure_all() first.`);
            }
            checkMeasuredLast(data, n);
            rng = makeRng(seed);
        }

        const { re, im } = evolve(n, data);

        if (get === 'statevector') {
            return Array.from(re, (r, i) => [r, im[i]]);
        }

        const probs = probabilities(re, im, n, noise);

        if (get === 'probabilities_dict') {
            const out = {};
            for (let i = 0; i < probs.length; i++) out[toBits(i, n)] = probs[i];
            return out;
        }

        const outcomes = sampleOutcomes(data, m, probs, shots, rng);

        // Bitstrings are only built once per distinct outcome.
        const labels = new Map();
        const label = (o) => {
            let s = labels.get(o);
            if (s === undefined) labels.set(o, (s = toBits(o, m)));
            return s;
        };

        if (get === 'memory') return Array.from(outcomes, label);

        const tally = new Map();
        for (const o of outcomes) tally.set(o, (tally.get(o) || 0) + 1);
        const counts = {};
        for (const o of [...tally.keys()].sort((a, b) => a - b)) counts[label(o)] = tally.get(o);
        return counts;
    }

    // ===== Statevector helpers, handy for building the input to initialize() =====

    // Tensor product of two statevectors of [re, im] pairs. vec1 ends up on the lower qubits.
    function kron(vec0, vec1) {
        const out = [];
        for (const [ar, ai] of vec0) {
            for (const [br, bi] of vec1) out.push([ar * br - ai * bi, ar * bi + ai * br]);
        }
        return out;
    }

    // Rescales a statevector of [re, im] pairs to unit length.
    function norm(ket) {
        let total = 0;
        for (const [r, i] of ket) total += r * r + i * i;
        if (total === 0) throw new RangeError('Cannot normalise an all-zero statevector.');
        const scale = 1 / Math.sqrt(total);
        return ket.map(([r, i]) => [r * scale, i * scale]);
    }

    // ===== Exports =====
    // The object literal is spelled out so Node can detect named exports for `import { ... }`.
    if (typeof module === 'object' && module !== null && module.exports) {
        module.exports = { QuantumCircuit, simulate, kron, norm };
    } else {
        // Browser globals, which p5.js global mode relies on.
        root.QuantumCircuit = QuantumCircuit;
        root.simulate = simulate;
        root.kron = kron;
        root.norm = norm;
    }
})(typeof globalThis !== 'undefined' ? globalThis : this);
