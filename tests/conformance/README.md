# Conformance cases

[cases.json](cases.json) holds circuits and the results every MicroMoth port must agree on:

- **Anchors**, whose expected values were worked out by hand from the gate definitions, such as rx(t)|0> = cos(t/2)|0> − i·sin(t/2)|1>
- statevectors for the remaining gates and for longer circuits, with controls above and below targets
- probabilities with a measurement-noise model
- counts for circuits with a certain outcome, which test bit order and measurement mapping
- circuits every port must reject, such as a gate acting on a qubit after it was measured

The non-anchor values come from running the Python template at the top of the repository. Regenerate them after changing the template or the cases, and commit both outputs:

```bash
python3 tests/conformance/generate.py
```

The generator first checks the template against every anchor, and stops if they disagree. So a bug in the template is reported instead of being written into the expected values.

**When a port disagrees, don't assume the template is right.** Check which cases fail. If an anchor fails, the port is wrong. If only template-generated cases fail, compare the gate's definition with both the port and the template before changing either.

The Arduino tests have no JSON parser, so the generator also writes the same cases as C++ to `versions/Arduino/tests/conformance_cases.inc`. `--check` reports whether the committed files are up to date without changing them, which CI uses.

## Where each port runs them

| Port | Test command |
|---|---|
| Python and MicroPython | `python3 versions/Python/tests.py` |
| JavaScript | `npm test --prefix versions/JavaScript` |
| Arduino | `sh versions/Arduino/tests/run_host_tests.sh` |

Double-precision ports compare within 1e-9. Arduino uses single-precision floats, so it compares within 1e-5.
