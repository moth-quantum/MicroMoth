# Conformance cases

[cases.json](cases.json) holds circuits and the results every MicroMoth port must agree on:

- statevectors for every gate, with controls above and below targets
- probabilities with a measurement-noise model
- counts for circuits with a certain outcome, which test bit order and measurement mapping
- circuits every port must reject, such as a gate acting on a qubit after it was measured

The Python template at the top of the repository is the reference. The cases are generated from it with a fixed seed:

```bash
python3 tests/conformance/generate.py
```

Run that after changing the template or the cases, and commit both outputs. The Arduino tests have no JSON parser, so the generator also writes the same cases as C++ to `versions/Arduino/tests/conformance_cases.inc`.

`--check` reports whether the committed files are up to date without changing them, which CI uses.

## Where each port runs them

| Port | Test command |
|---|---|
| Python and MicroPython | `python3 versions/Python/tests.py` |
| JavaScript | `npm test --prefix versions/JavaScript` |
| Arduino | `sh versions/Arduino/tests/run_host_tests.sh` |

Double-precision ports compare within 1e-9. Arduino uses single-precision floats, so it compares within 1e-5.
