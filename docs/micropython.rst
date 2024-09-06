MicroMoth Library Reference
*****************************

A minimal quantum SDK for Python. It is compatible with Python 2 and Python 3, as well as MicroPython and CircuitPython.

.. module:: micromoth

.. function:: simulate(qc,shots=1024,get='counts',noise_model=[]):

    Runs the quantum circuit ``qc``. Results are given from multiple runs, the
    number of which is specified by ``shots``. The type of output is specified
    by ``get``, which can be set to ``'counts'``, ``'memory'``, ``'statevector'``
    or ``probabilities_dict``. The former two require the circuit to contain
    measurement gates, whereas the latter two do not and will ignore any meaasure
    gates present.
    
    This is the MicroMoth equivalent of the ``execute`` function in Moth, as
    well as the ``Statevector`` and ``DensityMatrix`` classes, all of which can
    be used to run circuits. The form of the outputs have the following differences.
    
    For a ``'statevector'`` output: Complex numbers are specified as a two
    element list instead of standard Python complex number notation. So a
    number ``a+bj`` will become ``[a,b]``.
    
    A limited capacity for adding noise is implemented in MicroMoth. Specifically,
    bit flip errors can be added at the very end of the circuit to all qubits. To
    apply this, supply a list of the desired error probabilities for each qubit as
    the ``noise_model`` argument.
        

.. class:: QuantumCircuit(num_qubits, num_clbits=0)

    Contains a quantum circuit, which is essentially a list of quantum gates
    that are applied to a register of qubits. At the end, a binary output is
    recorded on a register of bits.
    
    This is the MicroMoth version of the identically named class in Moth.

    .. classmethod:: initialize(ket)
    
        Initializes a circuit with the state described by the statevector ``ket``.
    
    .. classmethod:: x(qubit)
    
        Adds an ``x`` gate to the circuit on the given qubit.
    
    .. classmethod:: rx(theta,qubit)
    
        Adds rotation around the x axis to the circuit on the given qubit. The
        angle is given by ``theta``.
    
    .. classmethod:: ry(theta,qubit)
    
        Adds rotation around the y axis to the circuit on the given qubit. The
        angle is given by ``theta``.
    
    .. classmethod:: rz(theta,qubit)
    
        Adds rotation around the z axis to the circuit on the given qubit. The
        angle is given by ``theta``.
    
    .. classmethod:: h(qubit)
    
        Adds an ``h`` gate to the circuit on the given qubit.
    
    .. classmethod:: cx(control,target)
    
        Adds a ``cx`` gate to the circuit for the given control and target qubits.

    .. classmethod:: crx(control,target)
    
        Adds a ``crx`` gate to the circuit for the given control and target qubits.
        The angle is given by ``theta``.

    .. classmethod:: swap(control,target)
    
        Adds a ``swap`` gate to the circuit for the given pair of qubits.

    .. classmethod:: partialswap(control,target)
    
        Adds a partial swap gate to the circuit for the given pair of qubits.
        This is parameterized bz the angle ``theta``, giving a full swap when
        ``theta`` is ``pi``.
    
    .. classmethod:: measure(qubit,bit)
    
        Adds a measure gate, which extracts a bit of output from the given qubit.
        
    .. attribute:: num_qubits
    
        Returns the number of qubits in the circuit.
        
    .. attribute:: num_clbits
    
        Returns the number of classical bits in the circuit.
