# MicroMoth for Python

This version of MicroMoth is compatible with MicroPython, CircuitPython, Python 2 and Python 3.

The [micromoth.py](micromoth.py) file found here is the same as that in the top-level folder, but with comments and blank lines removed. This is to reduce the file size, in order to better fit on a microcontroller. If it still doesn't fit, you can try deleting methods from the `QuantumCircuit`class that you don't use.

The creation of this file is done automatically by running [initialize_micromoth.py](initialize_micromoth.py). This process also performs tests to ensure that MicroMoth is working correctly.

### Installation

Dowloading a single file is all that is needed, so the word 'installation' is perhaps overkill. All you need to do is take the [micromoth.py](micromoth.py) file, and place somewhere that it can be found by Python when importing. The easiest option is simply to put it in the same folder as any Python scripts that will use it.

### Documentation

* [Documentation for MicroQiskit](https://microqiskit.readthedocs.io/en/latest/micropython.html)
