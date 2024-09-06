# MicroMoth

There are many well-developed and feature-rich frameworks for quantum computing. These allow quantum programs to be created and then run on either advanced simulators or real prototype quantum devices. Though its obviously a good thing to have many features and support many use-cases, it can sometimes be too much of a good thing!

For this reason we present MicroMoth: the smallest and most feature-poor framework for quantum computing. It has all the basic features and only the basic features. Making it much easier to learn, and much easier to get running.

MicroMoth is a fork of [MicroQiskit](https://github.com/qiskit-community/MicroQiskit), which was initially conceived as a version of the [Qiskit](https://www.ibm.com/quantum/qiskit) SDK that could run on microcontroller devices. The strict need for simplicity and a lack of complex dependencies was the main guidance for the design. MicroMoth has some additional features in comparison with the original MicroQiskit. In particular, `swap`and `partialswap` gates.

MicroMoth also contains ports for other languages, so you can bring the world of quantum into your familiar workflows. If you don't see your favourite language, note that it might be available for [MicroQiskit](https://github.com/moth-quantum/MicroMoth/versions/update_required) instead since we haven't finished upgrading all the ports.


### Documentation

* [Documentation for MicroQiskit](https://microqiskit.readthedocs.io/en/latest/#)

### Tutorial

* [Tutorial for MicroPython/Python 2/Python 3 version](versions/MicroPython/tutorials/index.ipynb)

### Installation

Installation guides for the various versions of MicroQiskit can be found in the corresponding README files.

* [MicroPython/Python 2/Python 3](versions/MicroPython/README.md)
* [Lua](versions/Lua/README.md)
* [C++](versions/C++/README.md)
* [C#](versions/C%23/README.md)
* [Ruby](versions/Ruby/README.md)
* [Racket](versions/Racket/README.org)


### Template Version

The [microqiskit.py](microqiskit.py) file found in this folder is intended as a template version. It contains comments to explain how each part of MicroMoth works, both to aid understanding and to help in the writing of ports. The MicroPython version is directly constructed from this template.

## Differences with Qiskit

### Available gates

Quantum gates are added to circuits in MicroMoth in the same way as in Qiskit. The main difference is that only the gates `x`, `y`, `z`, `h`, `cx`, `rx`, `ry` and `rz` can be added to a circuit in MicroQiskit. All others can be created if and when required from this basic set.

During simulation, or when exporting the circuit in a Python-compatible form, it is compiled into the set consisting only of `x`, `h`, `cx` and `rx`.

### Running Circuits

In Qiskit, the way circuits are run must account for the many different possible backends, including various simulators as well as prototype quantum devices. In MicroMoth, there is only one simulator. As such, the process is reduced to either a single `simulate` function or a single `Simulator` class, depending on language. These functions and classes are not present in Qiskit.

