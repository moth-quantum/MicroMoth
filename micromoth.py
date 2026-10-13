# -*- coding: utf-8 -*-

# (C) Copyright Moth Quantum 2024, 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

# (C) Copyright IBM 2023.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

# This file must keep running on MicroPython and CircuitPython as well as Python 3.
# So it uses no f-strings, no type hints, and only the `math` and `random` modules.

import random as _random
from math import cos as _cos, sin as _sin, pi

# `from micromoth import *` gives only these names. `pi` is kept for code such as qc.rx(pi/2, 0).
# MicroPython may ignore __all__, so everything else internal starts with an underscore.
__all__ = ['QuantumCircuit', 'simulate', 'pi']

_r2 = 0.70710678118 # 1/sqrt(2) will come in handy

# The kinds of output that simulate() can give.
_GET_MODES = ('statevector', 'probabilities_dict', 'counts', 'memory')


# ===== Checking inputs =====
# Every check raises an error at the call that caused it, so mistakes are reported where they are made.
# They use `raise` rather than `assert`, because Python removes asserts when run with the -O flag.

def _is_text(value):
  return isinstance(value, str) or isinstance(value, bytes)

def _is_sequence(value):
  '''True for lists, tuples and NumPy arrays. MicroPython may not report __len__ through hasattr.'''
  if isinstance(value, list) or isinstance(value, tuple):
    return True
  if _is_text(value):
    return False
  try:
    len(value)
  except TypeError:
    return False
  return True

def _whole(value, what):
  '''Returns `value` as an int if it is a whole number, and raises an error otherwise.'''
  if _is_text(value):
    raise TypeError(what + ' must be an integer, got ' + repr(value))
  try:
    i = int(value)
  except (TypeError, ValueError):
    raise TypeError(what + ' must be an integer, got ' + repr(value))
  if i != value:
    raise TypeError(what + ' must be an integer, got ' + repr(value))
  return i

def _index(value, size, what):
  '''Returns `value` as an int if it is a valid index for a register of the given size.'''
  i = _whole(value, what)
  if i < 0 or i >= size:
    raise ValueError(what + ' ' + str(i) + ' is out of range for ' + str(size) + ' available')
  return i

def _real(value, what):
  '''Returns `value` as a finite float.'''
  if _is_text(value):
    raise TypeError(what + ' must be a number, got ' + repr(value))
  try:
    x = float(value)
  except (TypeError, ValueError):
    raise TypeError(what + ' must be a number, got ' + repr(value))
  if x - x != 0: # True for inf and nan only.
    raise ValueError(what + ' must be finite, got ' + repr(value))
  return x

def _amplitude(e):
  '''Converts one amplitude into the [real, imaginary] pair used by the simulator.'''
  if _is_sequence(e): # A pair such as [0.6, 0.8] or (0.6, 0.8), or a NumPy row.
    if len(e) != 2:
      raise ValueError('an amplitude given as a pair must have 2 parts, got ' + repr(e))
    return [_real(e[0], 'amplitude'), _real(e[1], 'amplitude')]
  if hasattr(e, 'imag') and not _is_text(e): # Python and NumPy numbers, including complex ones.
    return [_real(e.real, 'amplitude'), _real(e.imag, 'amplitude')]
  return [_real(e, 'amplitude'), 0.0] # MicroPython ints and floats have no .imag.

def _gate_qubits(gate):
  '''The qubits that a gate in `data` acts on.'''
  if gate[0] in ('x', 'h'):
    return (gate[1],)
  if gate[0] in ('rx', 'rz'):
    return (gate[2],)
  if gate[0] in ('cx', 'swap'):
    return (gate[1], gate[2])
  if gate[0] == 'crx':
    return (gate[2], gate[3])
  return ()


class QuantumCircuit:

  def __init__(self,n,m=0):
    '''Defines and initializes the attributes'''
    # The number of qubits and the number of output bits are attributes of QuantumCircuit objects.
    self.num_qubits=_whole(n, 'num_qubits')
    self.num_clbits=_whole(m, 'num_clbits')
    if self.num_qubits < 1:
      raise ValueError('num_qubits must be at least 1, got ' + str(self.num_qubits))
    if self.num_clbits < 0:
      raise ValueError('num_clbits must not be negative, got ' + str(self.num_clbits))
    # It is possible to set a name for a circuit
    self.name = ''
    # Like Qiskit, QuantumCircuit objects in MicroQiskit have a `data` attribute, which is essentially a list of gates.
    # The contents of this in MicroQiskit are tailored to the needs of the `simulate` function.
    self.data=[]

  def __add__(self,self2):
    '''Allows QuantumCircuit objects to be added, as in Qiskit.'''
    if not isinstance(self2, QuantumCircuit):
      raise TypeError('only a QuantumCircuit can be added to a QuantumCircuit')
    self3=QuantumCircuit(max(self.num_qubits,self2.num_qubits),max(self.num_clbits,self2.num_clbits))
    self3.data=self.data+self2.data
    self3.name = self.name
    # An initialize() on fewer qubits would not cover the combined circuit.
    for gate in self3.data:
      if gate[0]=='init' and len(gate[1])!=2**self3.num_qubits:
        raise ValueError('cannot add a circuit whose initialize() has fewer qubits')
    return self3

  def initialize(self,k):
    '''Initializes the qubits in a given state.'''
    # Each amplitude can be a real number, a complex number, or a [real, imaginary] pair.
    if not _is_sequence(k):
      raise TypeError('initialize needs a list of amplitudes')
    if len(k) != 2**self.num_qubits:
      raise ValueError('initialize needs ' + str(2**self.num_qubits) + ' amplitudes, got ' + str(len(k)))
    state = [_amplitude(e) for e in k] # A private copy, so later changes to `k` cannot affect the circuit.
    if not any(e[0] or e[1] for e in state):
      raise ValueError('the statevector must not be all zeros')
    self.data[:] = [] # Clear existing gates.
    self.data.append(('init',state)) # Add the instruction to initialize, including the required state.

  def x(self,q):
    '''Applies an x gate to the given qubit.'''
    self.data.append(('x',_index(q,self.num_qubits,'qubit')))

  def rx(self,theta,q):
    '''Applies an rx gate to the given qubit by the given angle.'''
    self.data.append(('rx',_real(theta,'angle'),_index(q,self.num_qubits,'qubit')))

  def rz(self,theta,q):
    '''Applies an rz gate to the given qubit by the given angle.'''
    self.data.append(('rz',_real(theta,'angle'),_index(q,self.num_qubits,'qubit')))

  def h(self,q):
    '''Applies an h gate to the given qubit.'''
    self.data.append(('h',_index(q,self.num_qubits,'qubit')))

  def _pair(self,s,t):
    '''Checks a source and target qubit for a two-qubit gate.'''
    s = _index(s,self.num_qubits,'qubit')
    t = _index(t,self.num_qubits,'qubit')
    if s == t:
      raise ValueError('source and target qubits must differ, both are ' + str(s))
    return s, t

  def cx(self,s,t):
    '''Applies a cx gate to the given source and target qubits.'''
    s, t = self._pair(s,t)
    self.data.append(('cx',s,t))

  def crx(self,theta,s,t):
    '''Applies a crx gate to the given source and target qubits.'''
    s, t = self._pair(s,t)
    self.data.append(('crx',_real(theta,'angle'),s,t))

  def swap(self,s,t):
    '''Applies a swap to the given source and target qubits.'''
    s, t = self._pair(s,t)
    self.data.append(('swap',s,t))

  def measure(self,q,b):
    '''Applies a measure gate to the given qubit and bit.'''
    q = _index(q,self.num_qubits,'qubit')
    b = _index(b,self.num_clbits,'output bit')
    self.data.append(('m',q,b))

  def measure_all(self):
      '''Applies a measure gate to all qubits. Creates a classical register if none exists.'''
      if self.num_clbits==0:
        self.num_clbits = self.num_qubits
      # Check before adding anything, so a failed call leaves the circuit unchanged.
      if self.num_clbits < self.num_qubits:
        raise ValueError('measure_all needs at least ' + str(self.num_qubits) + ' output bits, the circuit has ' + str(self.num_clbits))
      for q in range(self.num_qubits):
        self.measure(q,q)

  # The gates below are built from the ones above. Their arguments are checked first,
  # so a bad call never leaves half a gate behind.

  def ry(self,theta,q):
    '''Applies an ry gate to the given qubit by the given angle.'''
    # This gate is constructed from `rx` and `rz`.
    theta = _real(theta,'angle')
    q = _index(q,self.num_qubits,'qubit')
    self.rx(pi/2,q)
    self.rz(theta,q)
    self.rx(-pi/2,q)

  def z(self,q):
    # This gate is constructed from `rz`.
    '''Applies a z gate to the given qubit.'''
    self.rz(pi,q)

  def t(self,q):
    # This gate is constructed from `rz`.
    '''Applies a t gate to the given qubit.'''
    self.rz(pi/4,q)

  def y(self,q):
    '''Applies an y gate to the given qubit.'''
    # This gate is constructed from `rz` and `x`.
    q = _index(q,self.num_qubits,'qubit')
    self.rz(pi,q)
    self.x(q)


def _noise(noise_model, n):
  '''Turns a noise model into a list of n measurement error probabilities, or None for no noise.'''
  if noise_model is None:
    return None
  if _is_sequence(noise_model):
    if len(noise_model) == 0:
      return None
    if len(noise_model) != n:
      raise ValueError('noise_model needs one probability per qubit (' + str(n) + '), got ' + str(len(noise_model)))
    probs = [_real(p, 'noise probability') for p in noise_model]
  else: # A single probability applies to every qubit.
    probs = [_real(noise_model, 'noise probability')] * n
  for p in probs:
    if p < 0 or p > 1:
      raise ValueError('noise probabilities must be from 0 to 1, got ' + repr(p))
  return probs

def _check_measured_last(qc):
  '''Measurements are applied at the end, so no gate may act on a qubit after it has been measured.'''
  measured = set()
  for gate in qc.data:
    if gate[0] == 'm':
      measured.add(gate[1])
    elif gate[0] == 'init':
      if measured:
        raise ValueError('initialize() cannot follow a measurement')
    else:
      for q in _gate_qubits(gate):
        if q in measured:
          raise ValueError("gate '" + gate[0] + "' acts on qubit " + str(q) + ' after it was measured. Measure each qubit after its last gate.')

def _random_source(seed):
  '''Returns a function giving random floats from 0 to 1. The same seed always gives the same samples.'''
  if seed is None:
    return _random.random
  seed = _whole(seed, 'seed')
  try:
    return _random.Random(seed).random # A private generator, so other code's random numbers are unaffected.
  except AttributeError: # MicroPython has no random.Random, so the shared generator is seeded instead.
    _random.seed(seed)
    return _random.random


def simulate(qc,shots=1024,get='counts',noise_model=[],seed=None):
  '''Simulates the given circuit `qc`, and outputs the results in the form specified by `shots` and `get`.'''

  if not isinstance(qc, QuantumCircuit):
    raise TypeError('simulate needs a QuantumCircuit')
  if get not in _GET_MODES:
    raise ValueError('get must be one of ' + ', '.join(_GET_MODES) + ", got '" + str(get) + "'")

  # Check everything before simulating, so mistakes are reported quickly.
  noise = None if get=='statevector' else _noise(noise_model, qc.num_qubits)
  if get in ('counts', 'memory'):
    shots = _whole(shots, 'shots')
    if shots < 0:
      raise ValueError('shots must not be negative, got ' + str(shots))
    if qc.num_clbits == 0:
      raise ValueError("'" + get + "' needs output bits. Use measure() or measure_all() first.")
    _check_measured_last(qc)
    rand = _random_source(seed)

  def superpose(x,y):
    '''For two elements of the statevector, x and y, return (x+y)/sqrt(2) and (x-y)/sqrt(2)'''
    return [_r2*(x[j]+y[j])for j in range(2)],[_r2*(x[j]-y[j])for j in range(2)]

  def turn(x,y,c,s):
    '''For two elements of the statevector, x and y, return cos(theta/2)*x - i*sin(theta/2)*y and cos(theta/2)*y - i*sin(theta/2)*x'''
    # c and s are cos(theta/2) and sin(theta/2). They are worked out once per gate rather than for every pair.
    return [x[0]*c+y[1]*s,x[1]*c-y[0]*s],[y[0]*c+x[1]*s,y[1]*c-x[0]*s]

  def phaseturn(x,y,c,s):
     '''For two elements of the statevector, x and y, return e^(-i theta/2)*x and e^(i theta/2)*y'''
     return [x[0]*c+x[1]*s,x[1]*c-x[0]*s],[y[0]*c-y[1]*s,y[1]*c+y[0]*s]

  # Initialize a 2^n element statevector. Complex numbers are expressed as a list of two real numbers.
  k = [[0.0,0.0] for _ in range(2**qc.num_qubits)] # First with zeros everywhere.
  k[0] = [1.0,0.0] # Then a single 1 to create the all |0> state.

  # The `outputmap` dictionary keeps track of which qubits are read out to which output bits
  outputmap = {}

  # Now we go through the gates and apply them to the statevector.
  # Each gate is specified by a tuple, as defined in the QuantumCircuit class
  for gate in qc.data:

    if gate[0]=='init': # For initializion, copy in the given statevector.
      k = [[e[0],e[1]] for e in gate[1]] # A fresh copy, so the circuit's stored state never changes.

    elif gate[0]=='m': # For measurement, keep a record of which bit goes with which qubit.
      outputmap[gate[2]] = gate[1]

    elif gate[0] in ['x','h','rx','rz']: # These are the only single qubit gates recognized by the simulator.

      j = gate[-1] # The qubit on which these gates act is the final element of the tuple.
      if gate[0] in ['rx','rz']:
        c, s = _cos(gate[1]/2), _sin(gate[1]/2)

      # These gates affect elements of the statevector in pairs.
      # These pairs are the elements whose corresponding bit strings differ only on bit `j`.
      # The following loops allow us to loop over all of these pairs.
      for i0 in range(2**j):
        for i1 in range(2**(qc.num_qubits-j-1)):
          b0=i0+2**(j+1)*i1 # Index corresponding to bit string for which the `j`th digit is '0'.
          b1=b0+2**j # Index corresponding to the same bit string except that the `j`th digit is '1'.
          if gate[0]=='x': # For x, just flip the values
            k[b0],k[b1]=k[b1],k[b0]
          elif gate[0]=='h': # For h, superpose them
            k[b0],k[b1]=superpose(k[b0],k[b1])
          elif gate[0]=='rx': # For rx, construct the superposition required for the given angle
            k[b0],k[b1]=turn(k[b0],k[b1],c,s)
          elif gate[0]=='rz': # For rz, change the phases by the given angle
            k[b0],k[b1]=phaseturn(k[b0],k[b1],c,s)

    elif gate[0] in ['cx','crx', 'swap']:

      # Get the source and target qubits
      if gate[0] in ['cx', 'swap']:
        [s,t] = gate[1:]
      else:
        c_, s_ = _cos(gate[1]/2), _sin(gate[1]/2)
        [s,t] = gate[2:]

      # Also get them sorted as highest and lowest
      [l,h] = sorted([s,t])

      # This gate only effects elements whose corresponding bit strings have a '1' on bit 's'.
      # Of those, it effects elements in pairs whose corresponding bit strings differ only on bit `t`.
      # The following loops allow us to loop over all of these pairs.
      for i0 in range(2**l):
        for i1 in range(2**(h-l-1)):
          for i2 in range(2**(qc.num_qubits-h-1)):
            b00=i0+2**(l+1)*i1+2**(h+1)*i2 # Index for the bit string where digits `s` and `t` are both `0``.
            b01=b00+2**t # As above but the bits for `s` and `t` are `0` and `1`.
            b10=b00+2**s # As above but the bits for `t` and `s` are `1` and `0`.
            b11=b10+2**t # As above but the bits for `t` and `s` are `1` and `1`.
            if gate[0]=='cx':
                k[b10],k[b11]=k[b11],k[b10] # Flip the values.
            elif gate[0]=='crx':
                k[b10],k[b11]=turn(k[b10],k[b11],c_,s_) # Perform the rotation.
            elif gate[0]=='swap':
                k[b01],k[b10]=k[b10],k[b01] # Flip the values.

  # Now for the outputs.

  # For the statevector output, simply return the statevector.
  # Every pair in `k` was created during this call, so the caller can change it freely.
  if get=='statevector':
    return k

  # To calculate outputs, we convert the statevector into a list of probabilities.
  # Here `probs[j]` is the probability for the output bit string to be the n bit representation of j.
  probs = [e[0]**2+e[1]**2 for e in k]

  # If there is a noise model, apply its effects
  if noise:
    for j in range(qc.num_qubits):
      p_meas = noise[j]
      for i0 in range(2**j):
        for i1 in range(2**(qc.num_qubits-j-1)):
          b0=i0+2**(j+1)*i1 # Index corresponding to bit string for which the `j`th digit is '0'.
          b1=b0+2**j # Index corresponding to the same bit string except that the `j`th digit is '1'.
          # change the probs to reproduce the effect of a measurement error
          p0 = probs[b0]
          p1 = probs[b1]
          probs[b0] = (1-p_meas)*p0 + p_meas*p1
          probs[b1] = (1-p_meas)*p1 + p_meas*p0

  # This can be output directly (as with Statevector or DensityMatrix in Qiskit
  if get=='probabilities_dict':
    # For each p=probs[j], the key is the n bit representation of j, and the value is p.
    return {('{0:0'+str(qc.num_qubits)+'b}').format(j):p for j,p in enumerate(probs)}

  # Otherwise, we need to sample: `counts` and `memory` outputs.

  # Running totals of the probabilities. Each shot picks the first total above a random number,
  # found by repeatedly halving the search range, so a shot costs about n steps rather than 2^n.
  cdf = []
  total = 0.0
  last = 0 # The last outcome with any probability, used if rounding pushes a random number past the end.
  for j,p in enumerate(probs):
    total += p
    cdf.append(total)
    if p > 0:
      last = j

  # The `shots` samples that result are then collected in the list `m`.
  labels = {} # Bit strings already built, keyed by outcome.
  m=[]
  for _ in range(shots):
    r = rand()*total # Scaling by the total means an unnormalised state still gives `shots` samples.
    lo, hi = 0, len(cdf)-1
    while lo < hi:
      mid = (lo+hi)//2
      if cdf[mid] > r:
        hi = mid
      else:
        lo = mid+1
    if cdf[lo] <= r:
      lo = last
    out = labels.get(lo)
    if out is None:
      # Convert outcome `lo` into an m bit string, with the order specified by the measure commands.
      # Output bits that are never measured read '0'.
      out_list = ['0']*qc.num_clbits
      for bit in outputmap:
        if (lo >> outputmap[bit]) & 1:
          out_list[qc.num_clbits-1-bit] = '1'
      out = ''.join(out_list)
      labels[lo] = out
    # Add this to the list of samples
    m.append(out)

  # For the memory output, we simply return `m`
  if get=='memory':
    return m
  # For the counts output, we turn it into a counts dictionary first
  counts = {}
  for out in m:
    if out in counts:
      counts[out] += 1
    else:
      counts[out] = 1
  return counts


  # Note: Ports should also contain the possibility to get a Qiskit output, which returns a string containing a Python
  # program to create the given circuit qc. This is not needed here, since the same syntax as standard Qiskit is used.
  # See the C++ port for an example.
