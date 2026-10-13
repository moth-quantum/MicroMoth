# MicroMoth for Python

This version of MicroMoth runs on Python 3, MicroPython and CircuitPython. Python 2 is no longer supported.

The [micromoth.py](micromoth.py) file found here is the same as that in the top-level folder, but with comments, docstrings and blank lines removed. This is to reduce the file size, in order to better fit on a microcontroller. If it still doesn't fit, you can try deleting methods from the `QuantumCircuit` class that you don't use, or precompile it with MicroPython's `mpy-cross` tool.

Don't edit this copy directly. Edit the top-level `micromoth.py`, then regenerate this copy and run the tests:

```bash
python3 versions/Python/initialize_micromoth.py
```

### Installation

Downloading a single file is all that is needed, so the word 'installation' is perhaps overkill. All you need to do is take the [micromoth.py](micromoth.py) file, and place it somewhere that it can be found by Python when importing. The easiest option is simply to put it in the same folder as any Python scripts that will use it.

It also works inside tools that embed Python 3, such as Blender, and accepts NumPy numbers and arrays where it takes numbers or amplitudes.

### Usage

```python
from micromoth import QuantumCircuit, simulate

qc = QuantumCircuit(2, 2)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

simulate(qc)                           # {'00': 509, '11': 515}
simulate(qc, 1024, 'counts', [], 7)    # the same counts every time for seed 7
```

Invalid arguments raise `ValueError` or `TypeError` at the call that caused them.

### Tests

The tests run on both Python 3 and MicroPython:

```bash
python3 versions/Python/tests.py
```

```bash
micropython versions/Python/tests.py
```

Add `--template` to test the commented top-level file instead of this copy.

### Documentation

* [Documentation for MicroMoth](https://micromoth.readthedocs.io/en/latest/micropython.html)
