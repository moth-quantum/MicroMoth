# MicroMoth

There are many well-developed and feature-rich frameworks for quantum computing. These allow quantum programs to be created and then run on either advanced simulators or real prototype quantum devices. Though it's obviously a good thing to have many features and support many use-cases, it can sometimes be too much of a good thing!

For this reason we present MicroMoth: the smallest and most feature-poor framework for quantum computing. It has all the basic features and only the basic features. Making it much easier to learn, and much easier to get running.

MicroMoth is an actively maintained fork of [MicroQiskit](https://github.com/qiskit-community/MicroQiskit), which was initially conceived as a version of the [Qiskit](https://www.ibm.com/quantum/qiskit) SDK that could run on microcontroller devices. The strict need for simplicity and a lack of complex dependencies was the main guidance for the design. MicroMoth has some additional features in comparison with the original MicroQiskit.

MicroMoth also contains ports for other languages, so you can bring the world of quantum into your familiar workflows. If you don't see your favourite language, note that it might be available for [MicroQiskit](versions/update_required) instead since we haven't finished upgrading all the ports.


### Documentation

* [Documentation for MicroMoth](https://micromoth.readthedocs.io/en/latest/#)

### Tutorial

* [Tutorial for MicroPython/Python 2/Python 3 version](versions/MicroPython/tutorials/index.ipynb)

### Installation

Installation guides for the various versions of MicroMoth can be found in the corresponding README files.

* [MicroPython/Python 2/Python 3](versions/Python/README.md)
* [Lua](versions/Lua/README.md)


### Template Version

The [micromoth.py](micromoth.py) file found in this folder is intended as a template version. It contains comments to explain how each part of MicroMoth works, both to aid understanding and to help in the writing of ports. The MicroPython version is directly constructed from this template.

