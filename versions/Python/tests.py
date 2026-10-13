# Tests for MicroMoth (Python).
# Gate results, bit order and the measurement rule are checked by the shared cases in tests/conformance.
# This file checks what is specific to the Python version: its API, its errors and the inputs it accepts.
#
# Runs on Python 3 and on MicroPython, from any folder:
#   python3 versions/Python/tests.py
#   micropython versions/Python/tests.py
# By default it tests versions/Python/micromoth.py. Add --template to test the commented micromoth.py at the top.
# Exits with status 1 if any check fails.

import sys
import math

try:
    _here = __file__.replace('\\', '/').rsplit('/', 1)[0] if '/' in __file__.replace('\\', '/') else '.'
except NameError:
    _here = '.'
sys.path.insert(0, _here + '/../..' if '--template' in sys.argv else _here)

import micromoth
from micromoth import QuantumCircuit, simulate

R2 = math.sqrt(0.5)
PI = math.pi
failures = [0]


def check(name, ok):
    print('  [' + ('PASS' if ok else 'FAIL') + '] ' + name)
    if not ok:
        failures[0] += 1


def approx(a, b, tol=1e-9):
    return abs(a - b) < tol


def same_state(a, b, tol=1e-9):
    if len(a) != len(b):
        return False
    for x, y in zip(a, b):
        if not (approx(x[0], y[0], tol) and approx(x[1], y[1], tol)):
            return False
    return True


def raises(fn, *types):
    try:
        fn()
    except types:
        return True
    except Exception:
        return False
    return False


def circuit(n, m=0, *ops):
    qc = QuantumCircuit(n, m)
    for op in ops:
        getattr(qc, op[0])(*op[1:])
    return qc


def statevector(qc):
    return simulate(qc, get='statevector')


def counts(qc, shots, seed=1, noise_model=[]):
    return simulate(qc, shots, 'counts', noise_model, seed)


def has_complex():
    try:
        complex(1, 2)
        return True
    except Exception:
        return False


class NumberLike:
    '''Behaves like a number from another library, such as a NumPy scalar: it converts with float() and int(),
    and has .real and .imag. It lets the tests cover such values without depending on any package.'''
    def __init__(self, re, im=0.0):
        self.real = re
        self.imag = im

    def __float__(self):
        return float(self.real)

    def __int__(self):
        return int(self.real)

    def __eq__(self, other):
        return self.real == other and self.imag == 0

    def __ne__(self, other):
        return not self.__eq__(other)


class ArrayLike:
    '''Behaves like an array from another library: it has a length and can be indexed, but is not a list or tuple.'''
    def __init__(self, items):
        self._items = list(items)

    def __len__(self):
        return len(self._items)

    def __getitem__(self, i):
        return self._items[i]

    def __iter__(self):
        return iter(self._items)


def main():
    print('Outputs')
    c = counts(circuit(2, 2, ('h', 0), ('cx', 0, 1), ('measure_all',)), 2000)
    check('Bell sampling: only 00 and 11', sorted(c.keys()) == ['00', '11'])
    c = counts(circuit(1, 1, ('ry', 1.0, 0), ('measure_all',)), 20000)
    check('Sampling: frequency within 0.02 of sin^2(0.5)', approx(c.get('1', 0) / 20000, math.sin(0.5) ** 2, 0.02))
    p = simulate(circuit(2, 0, ('x', 1)), get='probabilities_dict')
    check('probabilities_dict: 4 keys, |10> certain', sorted(p.keys()) == ['00', '01', '10', '11'] and approx(p['10'], 1))
    m = simulate(circuit(1, 1, ('h', 0), ('measure_all',)), 300, 'memory', [], 3)
    check('memory: one entry per shot, only 0/1', len(m) == 300 and all(s in ('0', '1') for s in m))
    qc = circuit(2, 2, ('h', 0), ('cx', 0, 1), ('measure_all',))
    check('Determinism: same seed reproduces memory',
          simulate(qc, 200, 'memory', [], 9) == simulate(qc, 200, 'memory', [], 9))
    check('Shots: an unnormalised state still gives every shot',
          len(simulate(circuit(1, 1, ('initialize', [0.5, 0.5]), ('measure', 0, 0)), 1000, 'memory', [], 1)) == 1000)
    check("Shots: unnormalised |1> samples only '1'",
          counts(circuit(1, 1, ('initialize', [[0, 0], [0.6, 0]]), ('measure', 0, 0)), 1000) == {'1': 1000})

    print('Noise model')
    p = simulate(circuit(2), get='probabilities_dict', noise_model=0.1)
    check('Noise as one number: 0.81 / 0.09 / 0.09 / 0.01',
          approx(p['00'], 0.81) and approx(p['01'], 0.09) and approx(p['10'], 0.09) and approx(p['11'], 0.01))
    p = simulate(circuit(2), get='probabilities_dict', noise_model=1)
    check('Noise int 1 accepted: every bit flips', approx(p['11'], 1))
    check('Noise list of the wrong length raises ValueError',
          raises(lambda: simulate(circuit(2), get='probabilities_dict', noise_model=[0.1]), ValueError))
    check('Noise outside [0, 1] raises ValueError',
          raises(lambda: simulate(circuit(1), get='probabilities_dict', noise_model=1.5), ValueError))

    print('Initialisation')
    check('initialize: real amplitudes', same_state(statevector(circuit(1, 0, ('initialize', [R2, R2]))), [[R2, 0], [R2, 0]]))
    check('initialize: tuple pairs', same_state(statevector(circuit(1, 0, ('initialize', [(0.6, 0), (0, 0.8)]))), [[0.6, 0], [0, 0.8]]))
    if has_complex():
        check('initialize: Python complex numbers',
              same_state(statevector(circuit(1, 0, ('initialize', [complex(0.6, 0), complex(0, 0.8)]))), [[0.6, 0], [0, 0.8]]))
    qc = circuit(1, 0, ('x', 0), ('initialize', [[1, 0], [0, 0]]))
    check('initialize: clears earlier gates', len(qc.data) == 1 and approx(statevector(qc)[0][0], 1))
    check('initialize: all zeros raises ValueError', raises(lambda: circuit(1, 0, ('initialize', [0, 0])), ValueError))
    source = [[0.0, 0.0], [1.0, 0.0]]
    qc = circuit(1, 0, ('initialize', source), ('x', 0))
    source[1][0] = 5.0
    sv = statevector(qc)
    sv[0][0] = 99.0
    check("Copies: neither the caller's list nor the returned statevector changes the circuit",
          qc.data[0][1] == [[0.0, 0.0], [1.0, 0.0]] and approx(statevector(qc)[0][0], 1))

    print('Composition')
    a = circuit(2, 0, ('h', 0))
    a.name = 'left'
    b = circuit(2, 0, ('cx', 0, 1))
    added = a + b
    check('add: matches sequential gates', same_state(statevector(added), statevector(circuit(2, 0, ('h', 0), ('cx', 0, 1)))))
    check('add: operands untouched', len(a.data) == 1 and len(b.data) == 1)
    check('add: name comes from left operand', added.name == 'left')
    wide = circuit(1, 1, ('x', 0)) + circuit(3, 2, ('x', 2))
    check('add: takes the larger register sizes', wide.num_qubits == 3 and wide.num_clbits == 2)
    check('add: smaller initialize into larger circuit raises ValueError',
          raises(lambda: circuit(2) + circuit(1, 0, ('initialize', [1, 0])), ValueError))

    print('Errors')
    check('Constructor: 0 qubits raises ValueError', raises(lambda: QuantumCircuit(0), ValueError))
    check('Gate: fractional qubit raises TypeError', raises(lambda: circuit(2, 0, ('x', 0.5)), TypeError))
    check('Gate: text qubit raises TypeError', raises(lambda: circuit(2, 0, ('x', '1')), TypeError))
    check('Gate: infinite angle raises ValueError', raises(lambda: circuit(1, 0, ('rx', float('inf'), 0)), ValueError))
    qc = QuantumCircuit(2)
    raises(lambda: qc.ry(float('nan'), 0), ValueError)
    raises(lambda: qc.y(5), ValueError)
    check('Composite gates: a bad call adds nothing', qc.data == [])
    qc = QuantumCircuit(2, 1)
    check('measure_all: too few output bits raises and adds nothing', raises(qc.measure_all, ValueError) and qc.data == [])
    check('Statevector ignores the measurement rule',
          not raises(lambda: statevector(circuit(1, 1, ('measure', 0, 0), ('x', 0))), Exception))
    check('Unknown get raises ValueError', raises(lambda: simulate(QuantumCircuit(1), get='bogus'), ValueError))

    print('Module')
    names = {}
    exec('from micromoth import *', names)
    exported = sorted(k for k in names if not k.startswith('__'))
    check('Star import gives QuantumCircuit, simulate and pi only', exported == ['QuantumCircuit', 'pi', 'simulate'])

    print('Number-like values from other libraries')
    check('Array of complex number-likes for initialize',
          same_state(statevector(circuit(1, 0, ('initialize', ArrayLike([NumberLike(0.6), NumberLike(0, 0.8)])))), [[0.6, 0], [0, 0.8]]))
    p = simulate(circuit(1), get='probabilities_dict', noise_model=NumberLike(0.25))
    check('Number-like noise model', approx(p['1'], 0.25))
    check('Number-like qubit indices',
          counts(circuit(NumberLike(2), NumberLike(2), ('x', NumberLike(1)), ('measure_all',)), 10) == {'10': 10})

    conformance()

    print('')
    if failures[0]:
        print(str(failures[0]) + ' test(s) FAILED.')
        sys.exit(1)
    print('All tests passed.')


def conformance():
    '''The cases every port must agree on. See tests/conformance/README.md.'''
    try:
        import json
    except ImportError:
        import ujson as json
    try:
        with open(_here + '/../../tests/conformance/cases.json') as f:
            cases = json.load(f)
    except OSError:
        print('Conformance: tests/conformance/cases.json not found, skipped')
        return

    def build(case):
        qc = QuantumCircuit(case['qubits'], case['clbits'])
        for op in case['ops']:
            getattr(qc, op[0])(*op[1:])
        return qc

    tol = cases['tolerance']['double']
    passed = [0, 0]

    def tally(ok, name):
        passed[1] += 1
        if ok:
            passed[0] += 1
        else:
            check('Conformance: ' + name, False)

    for case in cases['statevector']:
        tally(same_state(statevector(build(case)), case['expected'], tol), case['name'])
    for case in cases['probabilities']:
        p = simulate(build(case), get='probabilities_dict', noise_model=case['noise'])
        n = case['qubits']
        got = [p[('{0:0' + str(n) + 'b}').format(j)] for j in range(2 ** n)]
        tally(all(approx(x, y, tol) for x, y in zip(got, case['expected'])), case['name'])
    for case in cases['counts']:
        tally(counts(build(case), case['shots']) == case['expected'], case['name'])
    for case in cases['errors']:
        def attempt(case=case):
            simulate(build(case), 4, case['get'], [], 1)
        tally(raises(attempt, ValueError, TypeError), case['name'])
    check('Conformance: ' + str(passed[0]) + ' of ' + str(passed[1]) + ' shared cases agree', passed[0] == passed[1])


main()
