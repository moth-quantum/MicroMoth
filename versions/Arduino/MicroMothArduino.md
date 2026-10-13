# MicroMoth for Arduino

A single header, [MicroMothArduino.h](MicroMothArduino.h), with no dependencies beyond the Arduino core. It works on AVR boards such as the Uno and Mega, and on 32-bit boards such as the ESP32 and Raspberry Pi Pico.

Its behaviour follows the Python template ([micromoth.py](../../micromoth.py)), and its structure follows the [C# port](../Csharp/MicroMoth/MicroMoth.cs).

## Setup

Copy `MicroMothArduino.h` into your sketch folder, next to your `.ino` file, and include it:

```cpp
#include "MicroMothArduino.h"
```

The older `MicroMothArduinoMath.h` is no longer needed. Its contents are now part of the main header.

[MicroMothArduino.ino](MicroMothArduino.ino) is a complete example. It prints a Bell state, then flips a quantum coin on the built-in LED.

## Usage

```cpp
QuantumCircuit qc(2, 2);            // 2 qubits, 2 classical bits
qc.h(0).cx(0, 1).measure_all();     // gate methods can be chained

simulate(qc, 1024);                 // prints "00: 509" and "11: 515" to Serial

uint16_t counts[4];                 // one slot per outcome: 2^num_clbits
simulate_counts(qc, 1024, counts);  // counts[0] is "00", counts[3] is "11"
```

An outcome's number is its bitstring read in binary, with classical bit 0 on the right, as in Qiskit. So outcome 2 is "10".

### `QuantumCircuit(num_qubits, num_clbits = 0)`

| Method | What it adds |
|---|---|
| `x(q)`, `h(q)`, `y(q)`, `z(q)`, `t(q)` | Single-qubit gates |
| `rx(theta, q)`, `ry(theta, q)`, `rz(theta, q)` | Rotations by `theta` radians |
| `cx(s, t)`, `crx(theta, s, t)`, `crz(theta, s, t)` | Controlled gates, control `s`, target `t` |
| `swap(s, t)` | Exchanges two qubits |
| `measure(q, b)` | Reads qubit `q` into classical bit `b` |
| `measure_all()` | Measures qubit `i` into bit `i`, creating the bits if there are none |
| `initialize(state)` | Starts from `state` and discards earlier gates |
| `compose(other)`, `a + b` | Returns a new circuit: this one followed by `other` |

`initialize` takes an array of 2^num_qubits amplitudes. Use `ComplexNumber` values such as `{0.6, 0.8}`, or plain `float` values for real amplitudes. With a plain array the length is checked for you.

### Simulating

Each function returns `true` on success. On failure it returns `false`, and `micromoth_error()` gives the reason, ready for `Serial.println()`.

| Function | Fills |
|---|---|
| `simulate_counts(qc, shots, counts, noise)` | `counts[outcome]`, 2^num_clbits entries |
| `simulate_memory(qc, shots, outcomes, noise)` | One outcome per shot, `shots` entries |
| `simulate_probabilities(qc, probs, noise)` | One probability per basis state, 2^num_qubits floats |
| `simulate_statevector(qc, amplitudes)` | One `ComplexNumber` per basis state |

`simulate(qc, shots, get, noise, out)` is the Python-style shortcut. It prints the result to `Serial`, or to another `out` stream. `get` is `"counts"`, `"memory"`, `"statevector"` or `"probabilities_dict"`.

- **`noise`** is optional. It is an array with one measurement-error probability per qubit, as in the Python template's noise model.
- **Measurements happen at the end.** For counts and memory, a gate on a qubit after it was measured is an error.
- **`micromoth_seed(n)`** sets the random seed. Without it, every power-up gives the same samples. `micromoth_seed(analogRead(A0))` with A0 unconnected gives different ones.

### Errors

Arduino code has no exceptions, so a bad call such as `qc.x(5)` on a 2-qubit circuit adds nothing and marks the circuit as broken. `qc.ok()` reports this, and `qc.error()` gives the first problem. Simulating a broken circuit fails with the same message.

## Memory

The simulator needs 8 bytes per amplitude while it runs, which doubles with each qubit. It allocates that once per call and frees it before returning. `initialize` keeps a copy of its state, which costs the same again. Each gate costs 8 bytes.

| Qubits | Statevector |
|---|---|
| 6 | 512 bytes |
| 8 | 2 KB |
| 10 | 8 KB |
| 14 | 128 KB |

Estimated practical limits, leaving room for the rest of a sketch:

| Board | Memory | Qubits |
|---|---|---|
| Uno, Nano | 2 KB | about 6 |
| Mega 2560 | 8 KB | about 9 |
| ESP32, Pico | over 200 KB | about 14 |

A circuit that needs more memory than is free fails with "out of memory" instead of crashing. The build accepts at most 10 qubits on AVR boards and 16 elsewhere. Define `MICROMOTH_MAX_QUBITS` before the `#include` to change that.

## Tests

The tests run on a computer, with a small stand-in for the Arduino core. They build with the same strict settings the Arduino AVR toolchain uses.

```bash
sh versions/Arduino/tests/run_host_tests.sh
```

Add `SANITIZE=1` in front to also check for memory errors.

## Changes from the previous version

- Every gate checks its arguments. Bad indices used to corrupt memory silently.
- Results go into your own arrays through `simulate_counts`, `simulate_memory`, `simulate_probabilities` and `simulate_statevector`. Before, counts could only be printed.
- `simulate` is a plain function. Replace `qc.simulate(qc, 64, "counts")` with `simulate(qc, 64, "counts")`.
- `initialize` takes amplitudes, as in Python. `initialise(k, s)` with a basis-state index is gone. Use `x` gates to start from a basis state.
- `"memory"` and `"probabilities_dict"` work, along with `compose`, `+` and `micromoth_seed`.
- The memory check that only worked on AVR boards, and could stop the board, is gone. Running out of memory is now an error you can handle.
- The header can be included from several `.cpp` files, and works on ESP32, SAMD and RP2040 boards as well as AVR.
- Rotation gates compute their sine and cosine once per gate instead of once per amplitude pair. Trigonometry is slow on boards without a floating-point unit, so rotations there should be several times faster. This has not yet been measured on a board.
- Sampling uses 8 bytes per amplitude instead of 12, and picks each shot by binary search.
- `num_qubits` and `num_clbits` are now methods: `qc.num_qubits()`.
