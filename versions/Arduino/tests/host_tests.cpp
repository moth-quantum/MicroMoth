// Host tests for MicroMoth (Arduino).
// Gate results, bit order and the measurement rule are checked by the shared cases in tests/conformance.
// This file checks what is specific to the Arduino version: its API, its error reporting and its printing.
// Run with:  sh versions/Arduino/tests/run_host_tests.sh
// Exits non-zero if any check fails.

#include "MicroMothArduino.h"

#include <stdio.h>
#include <type_traits>

static_assert(!std::is_copy_constructible<QuantumCircuit>::value, "circuits must not be copyable");

int second_unit_bell_count();  // Defined in second_unit.cpp, which also includes the header.

static const float R2 = 0.70710678f;
static int failures = 0;

static void check(const char* name, bool ok) {
  printf("  [%s] %s\n", ok ? "PASS" : "FAIL", name);
  if (!ok) failures++;
}

static bool approx(float a, float b, float tol = 1e-5f) { return fabsf(a - b) < tol; }

static bool same_state(const ComplexNumber* a, const ComplexNumber* b, int n, float tol = 1e-5f) {
  for (int i = 0; i < n; i++) {
    if (!approx(a[i].real, b[i].real, tol) || !approx(a[i].imag, b[i].imag, tol)) return false;
  }
  return true;
}

static bool same_floats(const float* a, const float* b, int n, float tol) {
  for (int i = 0; i < n; i++) {
    if (!approx(a[i], b[i], tol)) return false;
  }
  return true;
}

// The shared cases in tests/conformance, generated as C++ because there is no JSON parser here.
static void run_conformance() {
  int agreed = 0, total = 0;
#define CONFORMANCE_TOLERANCE 1e-5f
#define CONFORM(name, ok)                                   \
  do {                                                      \
    total++;                                                \
    if (ok) agreed++;                                       \
    else check("Conformance: " name, false);                \
  } while (0)
#include "conformance_cases.inc"
#undef CONFORM
  char summary[80];
  snprintf(summary, sizeof summary, "Conformance: %d of %d shared cases agree", agreed, total);
  check(summary, agreed == total);
}

int main() {
  micromoth_seed(1);

  printf("Gates and outputs\n");
  {
    // crz is only in the Arduino and JavaScript versions, so the shared cases do not cover it.
    QuantumCircuit qc(2);
    qc.h(0).h(1).crz(micromoth_detail::PI_F, 0, 1);
    ComplexNumber a[4];
    const ComplexNumber want[4] = {{0.5f, 0}, {0, -0.5f}, {0.5f, 0}, {0, 0.5f}};
    check("crz: controlled phase", simulate_statevector(qc, a) && same_state(a, want, 4));
  }
  {
    QuantumCircuit qc(2, 2);
    qc.h(0).cx(0, 1).measure_all();
    uint16_t c[4];
    check("Bell sampling: only 00 and 11", simulate_counts(qc, 2000, c) && c[1] == 0 && c[2] == 0 && c[0] > 0 && c[3] > 0);
  }
  {
    QuantumCircuit qc(1, 1);
    qc.ry(1.0f, 0).measure_all();
    uint16_t c[2];
    const float want = sinf(0.5f) * sinf(0.5f);
    check("Sampling: frequency within 0.02 of sin^2(0.5)", simulate_counts(qc, 20000, c) && approx(c[1] / 20000.0f, want, 0.02f));
  }
  {
    QuantumCircuit qc(1, 1);
    qc.h(0).measure_all();
    uint16_t m[300];
    bool binary = simulate_memory(qc, 300, m);
    for (int k = 0; k < 300; k++) binary = binary && m[k] <= 1;
    check("memory: one outcome per shot, only 0/1", binary);
  }
  {
    QuantumCircuit qc(2, 2);
    qc.h(0).cx(0, 1).measure_all();
    uint16_t a[4], b[4];
    micromoth_seed(7);
    simulate_counts(qc, 500, a);
    micromoth_seed(7);
    simulate_counts(qc, 500, b);
    check("Determinism: same seed reproduces counts", memcmp(a, b, sizeof a) == 0);
  }
  {
    QuantumCircuit qc(1);
    const float noise[1] = {1.5f};
    float p[2];
    check("Noise outside [0, 1] fails", !simulate_probabilities(qc, p, noise) && micromoth_error() != nullptr);
  }

  printf("Initialisation\n");
  {
    QuantumCircuit qc(1);
    const float real[2] = {R2, R2};
    qc.initialize(real);
    ComplexNumber a[2];
    const ComplexNumber want[2] = {{R2, 0}, {R2, 0}};
    check("initialize: real amplitudes", simulate_statevector(qc, a) && same_state(a, want, 2));
  }
  {
    QuantumCircuit qc(1);
    const ComplexNumber zero[2] = {{1, 0}, {0, 0}};
    qc.x(0).initialize(zero);
    ComplexNumber a[2];
    check("initialize: clears earlier gates", qc.size() == 1 && simulate_statevector(qc, a) && approx(a[0].real, 1));
  }
  {
    QuantumCircuit qc(1);
    const float zeros[2] = {0, 0};
    qc.initialize(zeros);
    check("initialize: all zeros is an error", !qc.ok());
  }
  {
    QuantumCircuit qc(1, 1);
    const ComplexNumber unnormalised[2] = {{0, 0}, {0.6f, 0}};
    qc.initialize(unnormalised).measure(0, 0);
    uint16_t c[2];
    check("initialize: unnormalised |1> samples only 1", simulate_counts(qc, 1000, c) && c[1] == 1000);
  }

  printf("Errors\n");
  {
    QuantumCircuit none(0), too_many(MICROMOTH_MAX_QUBITS + 1);
    check("Constructor: 0 qubits is an error", !none.ok());
    check("Constructor: too many qubits is an error", !too_many.ok());
  }
  {
    QuantumCircuit qc(2);
    qc.x(2).cx(1, 1);
    check("Bad gates add nothing", !qc.ok() && qc.size() == 0);
  }
  {
    QuantumCircuit qc(1);
    qc.rx(NAN, 0);
    check("Gate: NaN angle is an error", !qc.ok());
  }
  {
    QuantumCircuit qc(2, 2);
    qc.measure(0, -1);
    check("measure: negative clbit is an error", !qc.ok());
  }
  {
    QuantumCircuit qc(2);
    qc.ry(INFINITY, 0).crz(1.0f, 0, 0);
    check("Composite gates: a bad call adds nothing", qc.size() == 0);
  }
  {
    QuantumCircuit qc(2);
    qc.x(5).cx(0, 0);
    check("Errors: the first one is kept", strcmp(reinterpret_cast<const char*>(qc.error()), "qubit index out of range") == 0);
    ComplexNumber a[4];
    check("Errors: a broken circuit does not simulate", !simulate_statevector(qc, a) && micromoth_error() == qc.error());
  }
  {
    QuantumCircuit qc(1, 1);
    qc.measure(0, 0).x(0);
    ComplexNumber a[2];
    check("Statevector ignores the measurement rule", simulate_statevector(qc, a));
  }

  printf("Printing\n");
  {
    QuantumCircuit qc(2, 2);
    qc.x(0).measure_all();
    Print out;
    check("simulate counts prints \"01: 64\"", simulate(qc, 64, "counts", nullptr, out) && out.text == "01: 64\r\n");
  }
  {
    QuantumCircuit qc(1);
    qc.h(0).rz(micromoth_detail::PI_F, 0);
    Print out;
    check("simulate statevector prints signed imaginary parts",
          simulate(qc, 0, "statevector", nullptr, out) && out.text == "0: 0.0000 - 0.7071i\r\n1: 0.0000 + 0.7071i\r\n");
  }
  {
    QuantumCircuit qc(1);
    Print out;
    check("simulate rejects an unknown get mode with a message",
          !simulate(qc, 1, "bogus", nullptr, out) && out.text.find("MicroMoth error: get must be") == 0);
  }

  printf("Conformance (tests/conformance/cases.json)\n");
  run_conformance();

  printf("Linking\n");
  check("Header included from two .cpp files links and runs", second_unit_bell_count() == 100);

  printf(failures == 0 ? "\nAll host tests passed.\n" : "\n%d test(s) FAILED.\n", failures);
  return failures == 0 ? 0 : 1;
}
