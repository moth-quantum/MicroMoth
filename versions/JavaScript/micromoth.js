// (C) Copyright Moth Quantum 2025.
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

const r2 = 0.70710678118;

export default class QuantumCircuit {
    #data;

    constructor(n, m=0) {
        this.num_qubits = n;
        this.num_clbits = m;
        this.name = '';

        this.#data = [];
    };

    getData() {
        return this.#data;
    }

    initialize(state) {
        if (!Array.isArray(state) || !Array.isArray(state[0])) {
            throw new Error("State must be an array of [real, imag] pairs.");
        }
        this.#data.push({ gate: "init", state });
        return this;
    }

    // Pauli-X gate
    x(q) {
        this.#data.push(['x', q]);
        return this;
    };

    // Hadamard gate
    h(q) {
        this.#data.push(['h', q]);
        return this;
    };

    // CX gate (Control-X gate)
    cx(s, t) { // source, target
        this.#data.push(['cx', s, t]);
        return this;
    };

    // Rotation-X gate
    rx(theta, q) {
        this.#data.push(['rx', theta, q]);
        return this;
    };

    // Rotation-Y gate
    ry(theta, q) {
        this.rx(Math.PI / 2, q);
        this.rz(theta, q);
        this.rx(-Math.PI / 2, q);
        return this;
    };

    // Rotation-Z gate
    rz(theta, q) {
        this.h(q);
        this.rx(theta, q);
        this.h(q);
        return this;
    };

    // Y gate
    y(q) {
        this.rz(Math.PI, q);
        this.x(q);
        return this;
    };

    // Pauli-Z gate
    z(q) {
        this.rz(Math.PI, q);
        return this;
    };

    // T gate
    t(q) {
        this.rz(Math.PI / 4, q);
    };

    crx(theta, s, t) {
        this.#data.push(['crx', theta, s, t]);
    };

    crz(theta, s, t) {
        this.#data.push(['crz', theta, s, t]);
    }; 
    
    swap(s, t) {
        this.#data.push(['swap', s, t]);
    };

    measure(q, b) {
        if (q >= this.num_qubits) {
            throw 'Select the right index of qubits.';
        }

        if (b >= this.num_clbits) {
            throw 'Select the right index of classical bits.';
        }

        this.#data.push(['m', q, b]);
        return this;
    };

    measure_all() {
        if (this.num_clbits === 0) this.num_clbits = this.num_qubits;
        for (let q = 0; q < this.num_qubits; q++) this.measure(q, q);

        return this;
    }
};

function simulate(qc, shots = 1024, get = 'counts', noise_model = []) {
    const nq = qc.num_qubits;
    const nc = qc.num_clbits;
    const d = qc.getData();

    let state = Array(2 ** nq).fill([0, 0]);
    state[0] = [1, 0]; // a |000...000> statevector

    if (!Array.isArray(noise_model)) {
        noise_model = Array(nq).fill(noise_model);
    }

    const outmap = {};

    for (let gate of d) {
        let op, args;

        if (Array.isArray(gate)) {
            [op, ...args] = gate;
        }
        else if (typeof gate == 'object' && gate.gate == 'init') {
            op = 'init';
            args = [gate.state];
        }
        else {
            throw new Error('Unknown gate format: ' + JSON.stringify(gate));
        }

        if (op == 'm') {
            outmap[args[1]] = args[0];
        }
        else if (op == 'init') {
            const s = args[0];

            for (let i = 0; i < s.length && i < state.length; i++) {
                state[i] = s[i];
            }
        }
        else if (['x', 'h', 'rx', 'rz'].includes(op)) {
            const j = args[args.length - 1];

            for (let i0 = 0; i0 < 2 ** j; i0++) {
                for (let i1 = 0; i1 < 2 ** (nq - j - 1); i1++) {
                    const b0 = i0 + 2 ** (j + 1) * i1;
                    const b1 = b0 + 2 ** j;

                    if (op == 'x') {
                        [state[b0], state[b1]] = [state[b1], state[b0]];
                    }
                    else if (op == 'h') {
                        [state[b0], state[b1]] = superposition(state[b0], state[b1]);
                    }
                    else if (op == 'rx') {
                        [state[b0], state[b1]] = rotate(state[b0], state[b1], args[0]);
                    }
                    else if (op == 'rz') {
                        [state[b0], state[b1]] = phaseturn(state[b0], state[b1], args[0]);
                    }
                }
            }
        }
        else if (['cx', 'crx', 'swap'].includes(op)) {
            const theta = (op == 'crx') ? args[0] : null;
            const [s, t] = (op == 'crx') ? args.slice(1) : args;
            const [l, h] = [Math.min(s, t), Math.max(s, t)];

            for (let i0 = 0; i0 < 2 ** l; i0++) {
                for (let i1 = 0; i1 < 2 ** (h - l - 1); i1++) {
                    for (let i2 = 0; i2 < 2 ** (nq - h - 1); i2++) {
                        const b00 = i0 + 2 ** (l + 1) * i1 + 2 ** (h + 1) * i2;
                        const b01 = b00 + 2 ** t;
                        const b10 = b00 + 2 ** s;
                        const b11 = b10 + 2 ** t;

                        if (op == 'cx') {
                        [state[b10], state[b11]] = [state[b11], state[b10]];
                        }
                        else if (op == 'crx') {
                        [state[b10], state[b11]] = rotate(state[b10], state[b11], theta);
                        }
                        else if (op == 'swap') {
                            [state[b01], state[b10]] = [state[b10], state[b01]];
                        }
                    }
                }
            }
        }
    }

    if (get == 'statevector') return state;
    // ====================================

    let probs = state.map(([a, b]) => a * a + b * b);

    if (noise_model.length > 0) {
        for (let j = 0; j < nq; j++) {
            const p = noise_model[j];
            for (let i0 = 0; i0 < 2 ** j; i0++) {
                for (let i1 = 0; i1 < 2 ** (nq - j - 1); i1++) {
                    const b0 = i0 + 2 ** (j + 1) * i1;
                    const b1 = b0 + 2 ** j;
                    const p0 = probs[b0], p1 = probs[b1];
                    probs[b0] = (1 - p) * p0 + p * p1;
                    probs[b1] = (1 - p) * p1 + p * p0;
                }
            }
         }
    }

    if (get == 'probability_dictionary') {
        return Object.fromEntries(
            probs.map((p, i) => [i.toString(2).padStart(nq, '0'), p])
        );
    }

    let results = [];
    for (let _ = 0; _ < shots; _++) {
        let r = Math.random();
        let cumu = 0;
        let chosen = null;

        for (let j = 0; j < probs.length; j++) {
            cumu += probs[j];
            if (r < cumu) {
                chosen = j;
                break;
            }
        }

        const raw = chosen.toString(2).padStart(nq, '0');
        const out_arr = Array(nc).fill('0');
        for (let bit in outmap) {
            out_arr[nc - 1 - parseInt(bit)] = raw[nq - 1 - outmap[bit]];
        }
        results.push(out_arr.join(''));
    }

    if (get == 'memory') return results; // accumulated step-by-step changes

    const counts = {};
    for (let res of results) {
        counts[res] = (counts[res] || 0) + 1;
    }

    return counts;
};

function rotate(x, y, theta) {
    const COS = Math.cos(theta / 2);
    const SIN = Math.sin(theta / 2);

    return [
        [x[0] * COS + y[1] * SIN, x[1] * COS - y[0] * SIN],
        [y[0] * COS + x[1] * SIN, y[1] * COS - x[0] * SIN]
    ];
};

function superposition(x, y) {
    return [
        [r2 * (x[0] + y[0]), r2 * (x[1] + y[1])],
        [r2 * (x[0] - y[0]), r2 * (x[1] - y[1])]
    ]
};

function phaseturn(x, y, tt) {
    const COS = Math.cos(tt / 2);
    const SIN = Math.sin(tt / 2);

    return [
        [x[0] * COS + x[1] * SIN, x[1] * COS - x[0] * SIN],
        [y[0] * COS - y[1] * SIN, y[1] * COS + y[0] * SIN]
    ]
};

function kron(vec0, vec1) {
    const newVec = []
    for (const amp0 of vec0) {
        for (const amp1 of vec1) {
            newVec.push([amp0[0] * amp1[0] - amp0[1] * amp1[1], amp0[0] * amp1[1] + amp0[1] * amp1[0]]);    
        }
    }

    return newVec;
}

// Normalise a statevector
function norm(ket) {
    let n = 0
    for (const amp of ket) n += amp[0] * amp[0] + amp[1] * amp[1];
    n = Math.sqrt(n);

    return ket.map((amp) => [amp[0] / n, amp[1] / n]);
}

export { simulate, rotate, superposition, phaseturn, kron, norm };