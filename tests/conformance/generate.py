# Generates the cases that every MicroMoth port must agree on, using the Python template as the reference.
#
#   python3 tests/conformance/generate.py           writes cases.json and the Arduino copy
#   python3 tests/conformance/generate.py --check   fails if either file is out of date (used by CI)
#
# Outputs:
#   tests/conformance/cases.json                    read by the Python and JavaScript tests
#   versions/Arduino/tests/conformance_cases.inc    the same cases as C++, for the Arduino host tests
#
# The cases are generated from a fixed seed, so the output only changes when the cases or the template change.

import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, ROOT)

from micromoth import QuantumCircuit, simulate  # noqa: E402  The commented template is the reference.

JSON_PATH = os.path.join(HERE, 'cases.json')
ARDUINO_PATH = os.path.join(ROOT, 'versions', 'Arduino', 'tests', 'conformance_cases.inc')

SINGLE = ['x', 'h', 'y', 'z', 't']
ROTATIONS = ['rx', 'ry', 'rz']
PAIRS = ['cx', 'swap']


def build(qubits, clbits, ops):
    qc = QuantumCircuit(qubits, clbits)
    for op in ops:
        getattr(qc, op[0])(*op[1:])
    return qc


def random_ops(rng, n, length):
    ops = []
    for _ in range(length):
        kind = rng.random()
        if n > 1 and kind < 0.35:
            s, t = rng.sample(range(n), 2)
            if rng.random() < 0.4:
                ops.append(['crx', round(rng.uniform(-7, 7), 6), s, t])
            else:
                ops.append([rng.choice(PAIRS), s, t])
        elif kind < 0.65:
            ops.append([rng.choice(ROTATIONS), round(rng.uniform(-7, 7), 6), rng.randrange(n)])
        else:
            ops.append([rng.choice(SINGLE), rng.randrange(n)])
    return ops


def statevector_cases(rng):
    fixed = [
        ('x on qubit 1 of 2', 2, [['x', 1]]),
        ('h on qubit 0 of 2', 2, [['h', 0]]),
        ('rx(pi/4)', 1, [['rx', 0.7853981633974483, 0]]),
        ('rz phase on |+>', 1, [['h', 0], ['rz', 1.1, 0]]),
        ('ry(pi/8)', 1, [['ry', 0.39269908169872414, 0]]),
        ('y on |0>', 1, [['y', 0]]),
        ('z and t on |+>', 1, [['h', 0], ['z', 0], ['t', 0]]),
        ('cx with control below target', 3, [['h', 0], ['cx', 0, 2]]),
        ('cx with control above target', 3, [['h', 2], ['cx', 2, 0]]),
        ('crx with control above target', 3, [['h', 2], ['crx', 0.9, 2, 1]]),
        ('swap of qubits 0 and 2', 3, [['x', 0], ['h', 1], ['swap', 0, 2]]),
        ('every gate', 3, [['h', 0], ['rx', 0.7, 1], ['rz', 1.1, 0], ['ry', 0.3, 2], ['y', 1], ['t', 0], ['z', 2],
                           ['cx', 2, 0], ['crx', 0.9, 2, 1], ['swap', 0, 2], ['crx', 0.4, 0, 2], ['cx', 1, 2], ['h', 2]]),
        ('initialize complex then gates', 2, [['initialize', [[0.5, 0], [0, 0.5], [-0.5, 0], [0, -0.5]]],
                                              ['h', 1], ['cx', 1, 0]]),
    ]
    cases = [{'name': name, 'qubits': n, 'clbits': 0, 'ops': ops} for name, n, ops in fixed]
    for i in range(40):
        n = rng.randint(1, 4)
        cases.append({'name': 'random circuit ' + str(i + 1), 'qubits': n, 'clbits': 0,
                      'ops': random_ops(rng, n, rng.randint(1, 15))})
    for case in cases:
        case['expected'] = simulate(build(case['qubits'], 0, case['ops']), get='statevector')
    return cases


def probability_cases(rng):
    cases = [
        {'name': 'noise 0.1 on both qubits of |00>', 'qubits': 2, 'clbits': 0, 'ops': [], 'noise': [0.1, 0.1]},
        {'name': 'noise only on qubit 0', 'qubits': 2, 'clbits': 0, 'ops': [['x', 0]], 'noise': [0.2, 0.0]},
        {'name': 'Bell state with noise', 'qubits': 2, 'clbits': 0, 'ops': [['h', 0], ['cx', 0, 1]], 'noise': [0.05, 0.15]},
    ]
    for i in range(10):
        n = rng.randint(1, 4)
        cases.append({'name': 'random noisy circuit ' + str(i + 1), 'qubits': n, 'clbits': 0,
                      'ops': random_ops(rng, n, rng.randint(1, 12)),
                      'noise': [round(rng.uniform(0, 0.3), 4) for _ in range(n)]})
    for case in cases:
        p = simulate(build(case['qubits'], 0, case['ops']), get='probabilities_dict', noise_model=case['noise'])
        n = case['qubits']
        case['expected'] = [p[('{0:0' + str(n) + 'b}').format(j)] for j in range(2 ** n)]
    return cases


def counts_cases():
    # Only circuits with a certain outcome, so every port gives exactly the same counts whatever its random numbers.
    cases = [
        ('x(0) reads as the rightmost bit', 2, 2, [['x', 0], ['measure_all']]),
        ('measure_all creates the output bits', 3, 0, [['x', 2], ['measure_all']]),
        ('measure(0, 1) puts qubit 0 on bit 1', 2, 2, [['x', 0], ['measure', 0, 1], ['measure', 1, 0]]),
        ('unmeasured output bits read 0', 2, 2, [['x', 0], ['x', 1], ['measure', 0, 0]]),
        ('partial measurement into a smaller register', 3, 1, [['x', 2], ['measure', 2, 0]]),
        ('two qubits into one bit: the last wins', 2, 1, [['x', 1], ['measure', 0, 0], ['measure', 1, 0]]),
        ('four qubits read in a custom order', 5, 4, [['x', 1], ['x', 3], ['x', 4], ['measure', 1, 0], ['measure', 3, 1],
                                                   ['measure', 4, 2], ['measure', 0, 3]]),
        ('swap then measure', 2, 2, [['x', 0], ['swap', 0, 1], ['measure_all']]),
        ('rx(pi) flips the qubit', 1, 1, [['rx', 3.141592653589793, 0], ['measure_all']]),
        ('Bell circuit run backwards returns to 00', 2, 2, [['h', 0], ['cx', 0, 1], ['cx', 0, 1], ['h', 0], ['measure_all']]),
    ]
    out = []
    for name, n, m, ops in cases:
        result = simulate(build(n, m, ops), 50, 'counts', [], 1)
        assert len(result) == 1, name + ' must have a certain outcome'
        clbits = m if m else n
        out.append({'name': name, 'qubits': n, 'clbits': m, 'ops': ops, 'shots': 50, 'expected': result,
                    'outcome': int(list(result.keys())[0], 2), 'outcomes': 2 ** clbits})
    return out


def error_cases():
    cases = [
        ('a gate after its measurement, with another measure between', 2, 2,
         [['measure', 0, 0], ['measure', 1, 1], ['x', 0]]),
        ('cx after measuring its control', 2, 2, [['h', 0], ['measure', 0, 0], ['cx', 0, 1], ['measure', 1, 1]]),
        ('a gate after measure(q, b) with q != b', 2, 2, [['measure', 0, 1], ['measure', 1, 0], ['x', 0]]),
        ('qubit index out of range', 2, 2, [['x', 2], ['measure_all']]),
        ('negative qubit index', 2, 2, [['h', -1], ['measure_all']]),
        ('source equal to target', 2, 2, [['cx', 1, 1], ['measure_all']]),
        ('output bit out of range', 2, 2, [['measure', 0, 2]]),
        ('initialize with the wrong length', 2, 2, [['initialize', [[1, 0], [0, 0]]], ['measure_all']]),
        ('counts without output bits', 1, 0, [['h', 0]]),
    ]
    out = []
    for name, n, m, ops in cases:
        try:
            simulate(build(n, m, ops), 4, 'counts', [], 1)
        except (ValueError, TypeError):
            out.append({'name': name, 'qubits': n, 'clbits': m, 'ops': ops, 'get': 'counts'})
            continue
        raise AssertionError('the template accepted an error case: ' + name)
    return out


def make_cases():
    rng = random.Random(2026)
    return {
        'about': 'Cases every MicroMoth port must agree on. Generated by tests/conformance/generate.py: do not edit.',
        'tolerance': {'double': 1e-9, 'float': 1e-5},
        'statevector': statevector_cases(rng),
        'probabilities': probability_cases(rng),
        'counts': counts_cases(),
        'errors': error_cases(),
    }


# ===== The Arduino copy, as C++ =====

def cpp_number(v, as_float=True):
    if isinstance(v, int) and not as_float:
        return str(v)
    text = repr(float(v))
    return text + 'f' if as_float else text


def cpp_ops(ops):
    lines = []
    for op in ops:
        name, args = op[0], op[1:]
        if name == 'initialize':
            amps = ', '.join('{' + cpp_number(re) + ', ' + cpp_number(im) + '}' for re, im in args[0])
            lines.append('{ static const ComplexNumber s[] = {' + amps + '}; qc.initialize(s); }')
        elif name in ('rx', 'ry', 'rz', 'crx'):
            lines.append('qc.' + name + '(' + ', '.join([cpp_number(args[0])] + [str(a) for a in args[1:]]) + ');')
        else:
            lines.append('qc.' + name + '(' + ', '.join(str(a) for a in args) + ');')
    return lines


def cpp_string(text):
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"') + '"'


def arduino_source(cases):
    out = ['// The shared conformance cases as C++, included by host_tests.cpp.',
           '// Generated by tests/conformance/generate.py: do not edit.', '']
    for case in cases['statevector']:
        dim = 2 ** case['qubits']
        want = ', '.join('{' + cpp_number(re) + ', ' + cpp_number(im) + '}' for re, im in case['expected'])
        out.append('{')
        out.append('  QuantumCircuit qc(' + str(case['qubits']) + ');')
        out += ['  ' + line for line in cpp_ops(case['ops'])]
        out.append('  static const ComplexNumber want[] = {' + want + '};')
        out.append('  ComplexNumber got[' + str(dim) + '];')
        out.append('  CONFORM(' + cpp_string(case['name']) + ', simulate_statevector(qc, got) && same_state(got, want, '
                   + str(dim) + ', CONFORMANCE_TOLERANCE));')
        out.append('}')
    for case in cases['probabilities']:
        dim = 2 ** case['qubits']
        out.append('{')
        out.append('  QuantumCircuit qc(' + str(case['qubits']) + ');')
        out += ['  ' + line for line in cpp_ops(case['ops'])]
        out.append('  static const float noise[] = {' + ', '.join(cpp_number(p) for p in case['noise']) + '};')
        out.append('  static const float want[] = {' + ', '.join(cpp_number(p) for p in case['expected']) + '};')
        out.append('  float got[' + str(dim) + '];')
        out.append('  CONFORM(' + cpp_string(case['name']) + ', simulate_probabilities(qc, got, noise) && same_floats(got, want, '
                   + str(dim) + ', CONFORMANCE_TOLERANCE));')
        out.append('}')
    for case in cases['counts']:
        out.append('{')
        out.append('  QuantumCircuit qc(' + str(case['qubits']) + ', ' + str(case['clbits']) + ');')
        out += ['  ' + line for line in cpp_ops(case['ops'])]
        out.append('  uint16_t got[' + str(case['outcomes']) + '];')
        out.append('  CONFORM(' + cpp_string(case['name']) + ', simulate_counts(qc, ' + str(case['shots']) + ', got) && got['
                   + str(case['outcome']) + '] == ' + str(case['shots']) + ');')
        out.append('}')
    for case in cases['errors']:
        outcomes = 2 ** case['clbits'] if case['clbits'] else 1
        out.append('{')
        out.append('  QuantumCircuit qc(' + str(case['qubits']) + ', ' + str(case['clbits']) + ');')
        out += ['  ' + line for line in cpp_ops(case['ops'])]
        out.append('  uint16_t got[' + str(outcomes) + '];')
        out.append('  CONFORM(' + cpp_string('rejects ' + case['name']) + ', !qc.ok() || !simulate_counts(qc, 4, got));')
        out.append('}')
    return '\n'.join(out) + '\n'


def close(a, b):
    '''Equal, except that numbers may differ by rounding: sin and cos differ slightly between operating systems.'''
    if isinstance(a, (int, float)) and isinstance(b, (int, float)) and not isinstance(a, bool):
        return abs(a - b) <= 1e-12
    if isinstance(a, dict) and isinstance(b, dict):
        return a.keys() == b.keys() and all(close(a[k], b[k]) for k in a)
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(close(x, y) for x, y in zip(a, b))
    return a == b


def check():
    '''Fails if the committed cases no longer match the template, or the Arduino copy no longer matches them.'''
    stale = []
    try:
        with open(JSON_PATH) as f:
            committed = json.load(f)
    except (OSError, ValueError):
        committed = None
    fresh = json.loads(json.dumps(make_cases()))
    if committed is None or not close(committed, fresh):
        stale.append(JSON_PATH)
    try:
        with open(ARDUINO_PATH) as f:
            if committed is None or f.read() != arduino_source(committed):
                stale.append(ARDUINO_PATH)
    except OSError:
        stale.append(ARDUINO_PATH)
    for path in stale:
        print('Out of date: ' + os.path.relpath(path, ROOT) + '. Run python3 tests/conformance/generate.py.')
    if stale:
        sys.exit(1)
    print('Conformance cases are up to date.')


def main():
    if '--check' in sys.argv:
        check()
        return
    cases = json.loads(json.dumps(make_cases()))
    outputs = {
        JSON_PATH: json.dumps(cases, indent=1) + '\n',
        ARDUINO_PATH: arduino_source(cases),
    }
    for path, text in outputs.items():
        with open(path, 'w') as f:
            f.write(text)
    total = sum(len(cases[k]) for k in ('statevector', 'probabilities', 'counts', 'errors'))
    print('Wrote ' + str(total) + ' cases to ' + os.path.relpath(JSON_PATH, ROOT) + ' and '
          + os.path.relpath(ARDUINO_PATH, ROOT) + '.')


if __name__ == '__main__':
    main()
