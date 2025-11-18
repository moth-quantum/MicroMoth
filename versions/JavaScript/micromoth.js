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

const r2 = Math.SQRT1_2; // 1/sqrt(2) for Hadamard gate

class QuantumCircuit {
    #data;
  
    constructor(n, m = 0) {
        if (!Number.isInteger(n) || n < 1 || n > 20) {
            throw new Error('num_qubits must be an integer between 1 and 20');
        }
        if (!Number.isInteger(m) || m < 0) {
            throw new Error('num_clbits must be a non-negative integer');
        }
        
        this.num_qubits = n;
        this.num_clbits = m;
        this.name = '';

        this.#data = [];
    };

    getData() {
        return this.#data;
    }

    initialize(state) (
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
        return this;
    };

    crx(theta, s, t) {
        this.#data.push(['crx', theta, s, t]);
        return this;
    };

    crz(theta, s, t) {
        this.#data.push(['crz', theta, s, t]);
        return this;
    }; 
    
    swap(s, t) {
        this.#data.push(['swap', s, t]);
        return this;
    };

    measure(q, b) {
        if (q >= this.num_qubits) {
            throw new Error(`Qubit index ${q} out of range. Must be < ${this.num_qubits}`);
        }

        if (b >= this.num_clbits) {
            throw new Error(`Classical bit index ${b} out of range. Must be < ${this.num_clbits}`);
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

    // Limitation
    if (nq > 20) {
        console.warn(`Warning: Simulating ${nq} qubits requires ${Math.pow(2, nq).toLocaleString()} state amplitudes. This may cause performance issues or crash.`);
    }
    if (nq > 25) {
        throw new Error(`Cannot simulate ${nq} qubits (requires ${Math.pow(2, nq).toLocaleString()} state amplitudes). Maximum recommended: 20 qubits.`);
    }

    let state = Array(2 ** nq).fill(null).map(() => [0, 0]);
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
            const mask = 1 << j;
            const step = 1 << (j + 1);
            
            // Nested loops to single iteration
            for (let i = 0; i < (1 << nq); i += step) {
                for (let k = 0; k < mask; k++) {
                    const b0 = i + k;
                    const b1 = b0 + mask;

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
            
            // Cache bit shifts for performance
            const mask_l = 1 << l;
            const mask_s = 1 << s;
            const mask_t = 1 << t;
            const step_l = 1 << (l + 1);
            const step_h = 1 << (h + 1);

            // Iteration with cached masks
            for (let i = 0; i < (1 << nq); i += step_h) {
                for (let j = 0; j < (1 << h); j += step_l) {
                    for (let k = 0; k < mask_l; k++) {
                        const b00 = i + j + k;
                        const b01 = b00 + mask_t;
                        const b10 = b00 + mask_s;
                        const b11 = b10 + mask_t;

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
            if (p === 0) continue; // Skip if no noise on this qubit
            
            const mask = 1 << j;
            const step = 1 << (j + 1);
            const p0_coeff = 1 - p;
            
            // Reduced nested loop with bit manipulation
            for (let i = 0; i < (1 << nq); i += step) {
                for (let k = 0; k < mask; k++) {
                    const b0 = i + k;
                    const b1 = b0 + mask;
                    const p0 = probs[b0], p1 = probs[b1];
                    probs[b0] = p0_coeff * p0 + p * p1;
                    probs[b1] = p0_coeff * p1 + p * p0;
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
    const outIndices = Object.keys(outmap).map(Number);
    const hasMapping = outIndices.length > 0;

    for (let _ = 0; _ < shots; _++) {
        let r = Math.random();
        let cumu = 0;
        let chosen = 0;

        for (let j = 0; j < probs.length; j++) {
            cumu += probs[j];
            if (r < cumu) {
                chosen = j;
                break;
            }
        }

        if (hasMapping) {
            const raw = chosen.toString(2).padStart(nq, '0');
            let out_arr = Array(nc).fill('0');
            for (let bit of outIndices) {
                out_arr[nc - 1 - bit] = raw[nq - 1 - outmap[bit]];
            }
            results.push(out_arr.join(''));
        } else {
            results.push(chosen.toString(2).padStart(nc || nq, '0'));
        }
    }

    if (get == 'memory') return results; // accumulating changes

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

// Global exports for browser usage (p5.js compatible)
if (typeof window !== 'undefined') {
    window.QuantumCircuit = QuantumCircuit;
    window.simulate = simulate;
    window.rotate = rotate;
    window.superposition = superposition;
    window.phaseturn = phaseturn;
    window.kron = kron;
    window.norm = norm;
}