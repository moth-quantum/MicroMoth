# MicroMoth (C) Copyright Moth Quantum 2024, 2026; (C) Copyright IBM 2023. Apache License 2.0.
# Generated from micromoth.py at the top of the repository: edit that file, not this one.
import random as _random
from math import cos as _cos, sin as _sin, pi
__all__ = ['QuantumCircuit', 'simulate', 'pi']
_r2 = 0.70710678118
_GET_MODES = ('statevector', 'probabilities_dict', 'counts', 'memory')
def _is_sequence(value):
  if isinstance(value, list) or isinstance(value, tuple):
    return True
  try:
    len(value)
  except TypeError:
    return False
  return True
def _whole(value, what):
  try:
    i = int(value)
  except (TypeError, ValueError):
    i = None
  if i is None or i != value:
    raise TypeError(what + ' must be an integer')
  return i
def _index(value, size, what):
  i = _whole(value, what)
  if i < 0 or i >= size:
    raise ValueError(what + ' ' + str(i) + ' out of range')
  return i
def _real(value, what):
  try:
    x = float(value)
  except (TypeError, ValueError):
    raise TypeError(what + ' must be a number')
  if x - x != 0:
    raise ValueError(what + ' must be finite')
  return x
def _amplitude(e):
  if _is_sequence(e):
    if len(e) != 2:
      raise ValueError('amplitude pairs need 2 parts')
    return [_real(e[0], 'amplitude'), _real(e[1], 'amplitude')]
  if hasattr(e, 'imag'):
    return [_real(e.real, 'amplitude'), _real(e.imag, 'amplitude')]
  return [_real(e, 'amplitude'), 0.0]
def _gate_qubits(gate):
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
    self.num_qubits=_whole(n, 'num_qubits')
    self.num_clbits=_whole(m, 'num_clbits')
    if self.num_qubits < 1:
      raise ValueError('num_qubits must be at least 1')
    if self.num_clbits < 0:
      raise ValueError('num_clbits must not be negative')
    self.name = ''
    self.data=[]
  def __add__(self,self2):
    if not isinstance(self2, QuantumCircuit):
      raise TypeError('can only add a QuantumCircuit')
    self3=QuantumCircuit(max(self.num_qubits,self2.num_qubits),max(self.num_clbits,self2.num_clbits))
    self3.data=self.data+self2.data
    self3.name = self.name
    for gate in self3.data:
      if gate[0]=='init' and len(gate[1])!=2**self3.num_qubits:
        raise ValueError('initialize() has too few qubits')
    return self3
  def initialize(self,k):
    if not _is_sequence(k):
      raise TypeError('initialize needs a list')
    if len(k) != 2**self.num_qubits:
      raise ValueError('initialize needs ' + str(2**self.num_qubits) + ' amplitudes')
    state = [_amplitude(e) for e in k]
    if not any(e[0] or e[1] for e in state):
      raise ValueError('statevector is all zeros')
    self.data[:] = []
    self.data.append(('init',state))
  def x(self,q):
    self.data.append(('x',_index(q,self.num_qubits,'qubit')))
  def rx(self,theta,q):
    self.data.append(('rx',_real(theta,'angle'),_index(q,self.num_qubits,'qubit')))
  def rz(self,theta,q):
    self.data.append(('rz',_real(theta,'angle'),_index(q,self.num_qubits,'qubit')))
  def h(self,q):
    self.data.append(('h',_index(q,self.num_qubits,'qubit')))
  def _pair(self,s,t):
    s = _index(s,self.num_qubits,'qubit')
    t = _index(t,self.num_qubits,'qubit')
    if s == t:
      raise ValueError('source and target must differ')
    return s, t
  def cx(self,s,t):
    s, t = self._pair(s,t)
    self.data.append(('cx',s,t))
  def crx(self,theta,s,t):
    s, t = self._pair(s,t)
    self.data.append(('crx',_real(theta,'angle'),s,t))
  def swap(self,s,t):
    s, t = self._pair(s,t)
    self.data.append(('swap',s,t))
  def measure(self,q,b):
    q = _index(q,self.num_qubits,'qubit')
    b = _index(b,self.num_clbits,'output bit')
    self.data.append(('m',q,b))
  def measure_all(self):
      if self.num_clbits==0:
        self.num_clbits = self.num_qubits
      if self.num_clbits < self.num_qubits:
        raise ValueError('measure_all needs ' + str(self.num_qubits) + ' output bits')
      for q in range(self.num_qubits):
        self.measure(q,q)
  def ry(self,theta,q):
    theta = _real(theta,'angle')
    q = _index(q,self.num_qubits,'qubit')
    self.rx(pi/2,q)
    self.rz(theta,q)
    self.rx(-pi/2,q)
  def z(self,q):
    self.rz(pi,q)
  def t(self,q):
    self.rz(pi/4,q)
  def y(self,q):
    q = _index(q,self.num_qubits,'qubit')
    self.rz(pi,q)
    self.x(q)
def _noise(noise_model, n):
  if noise_model is None:
    return None
  if _is_sequence(noise_model):
    if len(noise_model) == 0:
      return None
    if len(noise_model) != n:
      raise ValueError('noise_model needs ' + str(n) + ' probabilities')
    probs = [_real(p, 'noise probability') for p in noise_model]
  else:
    probs = [_real(noise_model, 'noise probability')] * n
  for p in probs:
    if p < 0 or p > 1:
      raise ValueError('noise probabilities must be from 0 to 1')
  return probs
def _check_measured_last(qc):
  measured = set()
  for gate in qc.data:
    if gate[0] == 'm':
      measured.add(gate[1])
    elif gate[0] == 'init':
      if measured:
        raise ValueError('initialize() after a measurement')
    else:
      for q in _gate_qubits(gate):
        if q in measured:
          raise ValueError(gate[0] + ' on qubit ' + str(q) + ' after measuring it')
def _random_source(seed):
  if seed is None:
    return _random.random
  seed = _whole(seed, 'seed')
  try:
    return _random.Random(seed).random
  except AttributeError:
    _random.seed(seed)
    return _random.random
def simulate(qc,shots=1024,get='counts',noise_model=[],seed=None):
  if not isinstance(qc, QuantumCircuit):
    raise TypeError('simulate needs a QuantumCircuit')
  if get not in _GET_MODES:
    raise ValueError('get must be one of ' + ', '.join(_GET_MODES))
  noise = None if get=='statevector' else _noise(noise_model, qc.num_qubits)
  if get in ('counts', 'memory'):
    shots = _whole(shots, 'shots')
    if shots < 0:
      raise ValueError('shots must not be negative')
    if qc.num_clbits == 0:
      raise ValueError(get + ' needs output bits')
    _check_measured_last(qc)
    rand = _random_source(seed)
  def superpose(x,y):
    return [_r2*(x[j]+y[j])for j in range(2)],[_r2*(x[j]-y[j])for j in range(2)]
  def turn(x,y,c,s):
    return [x[0]*c+y[1]*s,x[1]*c-y[0]*s],[y[0]*c+x[1]*s,y[1]*c-x[0]*s]
  def phaseturn(x,y,c,s):
     return [x[0]*c+x[1]*s,x[1]*c-x[0]*s],[y[0]*c-y[1]*s,y[1]*c+y[0]*s]
  k = [[0.0,0.0] for _ in range(2**qc.num_qubits)]
  k[0] = [1.0,0.0]
  outputmap = {}
  for gate in qc.data:
    if gate[0]=='init':
      k = [[e[0],e[1]] for e in gate[1]]
    elif gate[0]=='m':
      outputmap[gate[2]] = gate[1]
    elif gate[0] in ['x','h','rx','rz']:
      j = gate[-1]
      if gate[0] in ['rx','rz']:
        c, s = _cos(gate[1]/2), _sin(gate[1]/2)
      for i0 in range(2**j):
        for i1 in range(2**(qc.num_qubits-j-1)):
          b0=i0+2**(j+1)*i1
          b1=b0+2**j
          if gate[0]=='x':
            k[b0],k[b1]=k[b1],k[b0]
          elif gate[0]=='h':
            k[b0],k[b1]=superpose(k[b0],k[b1])
          elif gate[0]=='rx':
            k[b0],k[b1]=turn(k[b0],k[b1],c,s)
          elif gate[0]=='rz':
            k[b0],k[b1]=phaseturn(k[b0],k[b1],c,s)
    elif gate[0] in ['cx','crx', 'swap']:
      if gate[0] in ['cx', 'swap']:
        [s,t] = gate[1:]
      else:
        c_, s_ = _cos(gate[1]/2), _sin(gate[1]/2)
        [s,t] = gate[2:]
      [l,h] = sorted([s,t])
      for i0 in range(2**l):
        for i1 in range(2**(h-l-1)):
          for i2 in range(2**(qc.num_qubits-h-1)):
            b00=i0+2**(l+1)*i1+2**(h+1)*i2
            b01=b00+2**t
            b10=b00+2**s
            b11=b10+2**t
            if gate[0]=='cx':
                k[b10],k[b11]=k[b11],k[b10]
            elif gate[0]=='crx':
                k[b10],k[b11]=turn(k[b10],k[b11],c_,s_)
            elif gate[0]=='swap':
                k[b01],k[b10]=k[b10],k[b01]
  if get=='statevector':
    return k
  probs = [e[0]**2+e[1]**2 for e in k]
  if noise:
    for j in range(qc.num_qubits):
      p_meas = noise[j]
      for i0 in range(2**j):
        for i1 in range(2**(qc.num_qubits-j-1)):
          b0=i0+2**(j+1)*i1
          b1=b0+2**j
          p0 = probs[b0]
          p1 = probs[b1]
          probs[b0] = (1-p_meas)*p0 + p_meas*p1
          probs[b1] = (1-p_meas)*p1 + p_meas*p0
  if get=='probabilities_dict':
    return {('{0:0'+str(qc.num_qubits)+'b}').format(j):p for j,p in enumerate(probs)}
  cdf = []
  total = 0.0
  last = 0
  for j,p in enumerate(probs):
    total += p
    cdf.append(total)
    if p > 0:
      last = j
  labels = {}
  m=[]
  for _ in range(shots):
    r = rand()*total
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
      out_list = ['0']*qc.num_clbits
      for bit in outputmap:
        if (lo >> outputmap[bit]) & 1:
          out_list[qc.num_clbits-1-bit] = '1'
      out = ''.join(out_list)
      labels[lo] = out
    m.append(out)
  if get=='memory':
    return m
  counts = {}
  for out in m:
    if out in counts:
      counts[out] += 1
    else:
      counts[out] = 1
  return counts
