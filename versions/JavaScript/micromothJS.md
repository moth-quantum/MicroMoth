# MicroMoth for JavaScript

A single file, [micromoth.js](micromoth.js), with no dependencies. It runs in the browser (including p5.js) and in Node 18 or newer.

Its behaviour follows the Python template ([micromoth.py](../../micromoth.py)), and its structure follows the [C# port](../Csharp/MicroMoth/MicroMoth.cs).

## Setup

### Browser and p5.js

Copy `micromoth.js` next to your page and load it before your sketch. It defines the globals `QuantumCircuit` and `simulate`, plus a `MicroMoth` object that also holds the helpers `kron` and `norm`. The helpers are not global, because p5.js has its own `norm()`.

```html
<script src="micromoth.js"></script>
<script src="sketch.js"></script>
```

### p5.js Editor

The quickest way is to load the file from the jsDelivr CDN. In `index.html`, add this line after the p5.js line and before `sketch.js`:

```html
<script src="https://cdn.jsdelivr.net/gh/moth-quantum/MicroMoth@main/versions/JavaScript/micromoth.js"></script>
```

To keep a copy inside the sketch instead, add a file named `micromoth.js` to it. Logged-in users can upload the file. Anyone can use "Add File" and paste in the contents. Then add `<script src="micromoth.js"></script>` to `index.html` in the same place.

A copy inside the sketch is rewritten by the Editor to stop loops that run longer than 100 ms. Small circuits are unaffected. Around 18 qubits the rewrite makes MicroMoth about ten times slower, and around 20 qubits it can stop a loop inside the simulator. MicroMoth then throws an error rather than returning a wrong result. Files loaded from the CDN are not rewritten. To turn the rewrite off for a sketch, make this the first line of `index.html`:

```html
<!-- noprotect -->
```

### Node

```js
const { QuantumCircuit, simulate } = require('./micromoth.js');
```

ES modules work too:

```js
import { QuantumCircuit, simulate } from './micromoth.js';
```

## Usage

```js
const qc = new QuantumCircuit(2, 2); // 2 qubits, 2 classical bits
qc.h(0).cx(0, 1).measure_all();      // gate methods can be chained

simulate(qc);                        // { '00': 509, '11': 515 }
simulate(qc, 8, 'memory');           // [ '11', '00', '00', '11', ... ]
simulate(qc, 1024, 'counts', [], 7); // the same counts every time for seed 7
```

Bitstrings put qubit 0, or classical bit 0, on the right, as in Qiskit.

### `new QuantumCircuit(num_qubits, num_clbits = 0)`

| Method | What it adds |
|---|---|
| `x(q)`, `h(q)`, `y(q)`, `z(q)`, `t(q)` | Single-qubit gates |
| `rx(theta, q)`, `ry(theta, q)`, `rz(theta, q)` | Rotations by `theta` radians |
| `cx(s, t)`, `crx(theta, s, t)`, `crz(theta, s, t)` | Controlled gates, control `s`, target `t` |
| `swap(s, t)` | Exchanges two qubits |
| `measure(q, b)` | Reads qubit `q` into classical bit `b` |
| `measure_all()` | Measures qubit `i` into bit `i`, creating the bits if there are none |
| `initialize(state)` | Starts from `state` and discards earlier gates |
| `compose(other)`, `add(other)` | Returns a new circuit: this one followed by `other` |

`initialize` takes 2^num_qubits amplitudes. Each amplitude is a real number or a `[re, im]` pair.

The circuit also exposes `num_qubits`, `num_clbits`, a writable `name`, and `data`. `data` is a read-only snapshot of the gate list.

### `simulate(qc, shots = 1024, get = 'counts', noise_model = [], seed)`

| `get` | Returns |
|---|---|
| `'counts'` | `{ bitstring: count }` over `shots` samples |
| `'memory'` | One bitstring per shot, in sampling order |
| `'probabilities_dict'` | `{ bitstring: probability }` for every basis state |
| `'statevector'` | `[re, im]` per basis state |

- **`noise_model`** adds measurement errors. Pass one probability for every qubit, or an array with one probability per qubit. The statevector ignores it.
- **`seed`** is an optional integer. The same seed always gives the same samples.
- **Measurements happen at the end.** For counts and memory, a gate on a qubit after it was measured throws an error.

`kron(a, b)` and `norm(ket)` combine and normalise `[re, im]` statevectors, which is handy for `initialize`. In the browser, call them as `MicroMoth.kron` and `MicroMoth.norm`.

### Limits and errors

A circuit holds 1 to 20 qubits and 0 to 30 classical bits. Invalid arguments throw a `RangeError` or `TypeError` at the call that caused them, rather than failing later in `simulate`.

## Tests

```bash
npm test
```

## Changes from 1.0

- Gate methods validate their arguments.
- `initialize` accepts real amplitudes, checks the length, and clears earlier gates.
- `compose`, `add` and the `seed` argument are new.
- `crz` is now simulated. It was silently ignored before.
- `probabilities_dict` is the output name, matching Python. The 1.0 spelling `probability_dictionary` still works.
- `counts` and `memory` need classical bits. Use `measure` or `measure_all`.
- `getData()` now returns a read-only snapshot.
- The internal helpers `rotate`, `superposition` and `phaseturn` are no longer exported.
- In the browser, `kron` and `norm` moved from globals to the `MicroMoth` object, so they no longer replace p5.js's `norm()`.
- The file no longer uses `#private` class syntax, which stopped sketches from starting in the p5.js Editor.
