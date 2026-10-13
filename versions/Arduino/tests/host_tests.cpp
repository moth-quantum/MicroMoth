// Host tests for MicroMoth (Arduino), mirroring versions/Csharp/Tests/Tests.cs and versions/JavaScript/tests.js.
// Run with:  sh versions/Arduino/tests/run_host_tests.sh
// Exits non-zero if any check fails.

#include "MicroMothArduino.h"

#include <stdio.h>
#include <type_traits>

static_assert(!std::is_copy_constructible<QuantumCircuit>::value, "circuits must not be copyable");
static_assert(std::is_move_constructible<QuantumCircuit>::value, "circuits must be movable");

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

// Number of distinct outcomes seen, and whether every one is in the allowed list.
static bool only(const uint16_t* counts, int outcomes, const int* allowed, int n_allowed, int* distinct = nullptr) {
  int seen = 0;
  for (int i = 0; i < outcomes; i++) {
    if (!counts[i]) continue;
    seen++;
    bool listed = false;
    for (int k = 0; k < n_allowed; k++) listed = listed || allowed[k] == i;
    if (!listed) return false;
  }
  if (distinct) *distinct = seen;
  return true;
}

int main() {
  micromoth_seed(1);

  printf("Gates\n");
  {
    QuantumCircuit qc(1, 1);
    qc.x(0).measure_all();
    uint16_t c[2];
    check("X: all ones", simulate_counts(qc, 1000, c) && c[0] == 0 && c[1] == 1000);
  }
  {
    QuantumCircuit qc(1, 1);
    qc.h(0).measure_all();
    uint16_t c[2];
    check("H: two balanced outcomes", simulate_counts(qc, 1000, c) && c[0] > 400 && c[0] < 600 && c[0] + c[1] == 1000);
  }
  {
    QuantumCircuit qc(2);
    qc.h(0).cx(0, 1);
    ComplexNumber a[4];
    const ComplexNumber bell[4] = {{R2, 0}, {0, 0}, {0, 0}, {R2, 0}};
    check("Bell statevector = [.707, 0, 0, .707]", simulate_statevector(qc, a) && same_state(a, bell, 4));
  }
  {
    QuantumCircuit qc(2, 2);
    qc.h(0).cx(0, 1).measure_all();
    uint16_t c[4];
    const int allowed[] = {0, 3};
    int distinct = 0;
    check("Bell: only 00 and 11", simulate_counts(qc, 2000, c) && only(c, 4, allowed, 2, &distinct) && distinct == 2);
  }
  {
    QuantumCircuit qc(3, 3);
    qc.h(0).cx(0, 1).cx(1, 2).measure_all();
    uint16_t c[8];
    const int allowed[] = {0, 7};
    check("GHZ: only 000 and 111", simulate_counts(qc, 2000, c) && only(c, 8, allowed, 2));
  }
  {
    QuantumCircuit qc(2, 2);
    qc.x(0).measure_all();
    uint16_t c[4];
    check("Endianness: x(0) -> 01", simulate_counts(qc, 100, c) && c[1] == 100);
  }
  {
    QuantumCircuit qc(2, 2);
    qc.x(0).swap(0, 1).measure_all();
    uint16_t c[4];
    check("Swap: x(0) then swap -> 10", simulate_counts(qc, 100, c) && c[2] == 100);
  }
  {
    QuantumCircuit qc(1);
    qc.ry(micromoth_detail::PI_F / 2, 0);
    ComplexNumber a[2];
    const ComplexNumber want[2] = {{R2, 0}, {R2, 0}};
    check("Ry(pi/2): [.707, .707] real", simulate_statevector(qc, a) && same_state(a, want, 2));
  }
  {
    QuantumCircuit qc(1);
    qc.h(0).rz(micromoth_detail::PI_F / 3, 0);
    float p[2];
    check("Rz: phase only (probs 0.5/0.5)", simulate_probabilities(qc, p) && approx(p[0], 0.5f) && approx(p[1], 0.5f));
  }
  {
    QuantumCircuit qc(2);
    qc.h(0).h(1).crz(micromoth_detail::PI_F, 0, 1);
    ComplexNumber a[4];
    const ComplexNumber want[4] = {{0.5f, 0}, {0, -0.5f}, {0.5f, 0}, {0, 0.5f}};
    check("crz: controlled phase", simulate_statevector(qc, a) && same_state(a, want, 4));
  }
  {
    // Every gate, with controls above and below targets, against the Python template's statevector.
    QuantumCircuit qc(3);
    qc.h(0).rx(0.7f, 1).rz(1.1f, 0).ry(0.3f, 2).y(1).t(0).z(2)
      .cx(2, 0).crx(0.9f, 2, 1).swap(0, 2).crx(0.4f, 0, 2).cx(1, 2).h(2);
    const ComplexNumber want[8] = {
      {0.0f, -0.19922637949540753f}, {-0.0017416246978755984f, -0.008591704369767583f},
      {-0.5457830890379092f, 0.0f}, {0.08563046244323613f, -0.017358154082492475f},
      {-0.27433922922096493f, 0.0f}, {0.011830971181960227f, 0.0023982565883972308f},
      {0.0f, -0.7515556541645066f}, {0.023902570652293995f, -0.11791508295242457f},
    };
    ComplexNumber a[8];
    check("All gates: matches Python template statevector", simulate_statevector(qc, a) && same_state(a, want, 8));
  }

  printf("Measurement and outputs\n");
  {
    QuantumCircuit qc(2, 2);
    qc.x(0).measure(0, 1).measure(1, 0);
    uint16_t c[4];
    check("measure(0,1): qubit 0 lands on bit 1 -> 10", simulate_counts(qc, 50, c) && c[2] == 50);
  }
  {
    QuantumCircuit qc(2, 2);
    qc.x(0).x(1).measure(0, 0);
    uint16_t c[4];
    check("Unmeasured clbit reads 0 -> 01", simulate_counts(qc, 50, c) && c[1] == 50);
  }
  {
    QuantumCircuit qc(3, 1);
    qc.x(2).measure(2, 0);
    uint16_t c[2];
    check("Partial: measure(2,0) on 3 qubits -> 1", simulate_counts(qc, 50, c) && c[1] == 50);
  }
  {
    QuantumCircuit qc(2);
    qc.h(0).cx(0, 1).measure_all();
    check("measure_all: creates 2 clbits", qc.num_clbits() == 2 && qc.ok());
  }
  {
    QuantumCircuit qc(1, 1);
    qc.ry(1.0f, 0).measure_all();
    uint16_t c[2];
    const float want = sinf(0.5f) * sinf(0.5f);
    check("Sampling: frequency within 0.02 of sin^2(0.5)", simulate_counts(qc, 20000, c) && approx(c[1] / 20000.0f, want, 0.02f));
  }
  {
    QuantumCircuit qc(2);
    qc.x(1);
    float p[4];
    check("probabilities: |10> certain", simulate_probabilities(qc, p) && approx(p[2], 1) && approx(p[0], 0));
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

  printf("Noise model\n");
  {
    QuantumCircuit qc(2);
    const float noise[2] = {0.1f, 0.1f};
    float p[4];
    check("Noise on both qubits: 0.81 / 0.09 / 0.09 / 0.01",
          simulate_probabilities(qc, p, noise) && approx(p[0], 0.81f) && approx(p[1], 0.09f) && approx(p[2], 0.09f) && approx(p[3], 0.01f));
  }
  {
    QuantumCircuit qc(2);
    qc.x(0);
    const float noise[2] = {0.2f, 0.0f};
    float p[4];
    check("Noise per qubit: only qubit 0 flips", simulate_probabilities(qc, p, noise) && approx(p[1], 0.8f) && approx(p[0], 0.2f) && approx(p[2], 0));
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
    QuantumCircuit qc(2);
    const float short_state[2] = {1, 0};
    qc.initialize(short_state);
    check("initialize: wrong length is an error", !qc.ok() && qc.size() == 0);
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

  printf("Composition\n");
  {
    QuantumCircuit a(2), b(2), sequential(2);
    a.name = "left";
    a.h(0);
    b.cx(0, 1);
    sequential.h(0).cx(0, 1);
    QuantumCircuit composed = a.compose(b);
    ComplexNumber x[4], y[4];
    check("compose: matches sequential gates",
          simulate_statevector(composed, x) && simulate_statevector(sequential, y) && same_state(x, y, 4));
    check("compose: operands untouched", a.size() == 1 && b.size() == 1);
    check("compose: name comes from left operand", strcmp(composed.name, "left") == 0);
  }
  {
    QuantumCircuit a(1), b(1);
    a.x(0);
    b.x(0);
    QuantumCircuit sum = a + b;
    ComplexNumber s[2];
    check("operator+: x then x returns to |0>", simulate_statevector(sum, s) && approx(s[0].real, 1));
    QuantumCircuit small(1, 1), wide(3, 2);
    small.x(0);
    wide.x(2);
    QuantumCircuit both = small + wide;
    check("operator+: takes the larger register sizes", both.num_qubits() == 3 && both.num_clbits() == 2 && both.ok());
  }
  {
    QuantumCircuit big(2), small(1);
    const float one[2] = {1, 0};
    small.initialize(one);
    QuantumCircuit bad = big.compose(small);
    check("compose: smaller initialize into larger circuit is an error", !bad.ok());
  }
  {
    QuantumCircuit a(1);
    a.h(0);
    QuantumCircuit moved(static_cast<QuantumCircuit&&>(a));
    check("move: the new circuit owns the gates, the old one is empty", moved.size() == 1 && a.size() == 0);
  }

  printf("Validation\n");
  {
    QuantumCircuit none(0), too_many(MICROMOTH_MAX_QUBITS + 1);
    check("Constructor: 0 qubits is an error", !none.ok());
    check("Constructor: too many qubits is an error", !too_many.ok());
  }
  {
    QuantumCircuit qc(2);
    qc.x(2);
    check("Gate: qubit out of range is an error and adds nothing", !qc.ok() && qc.size() == 0);
  }
  {
    QuantumCircuit qc(2);
    qc.h(-1);
    check("Gate: negative qubit is an error", !qc.ok());
  }
  {
    QuantumCircuit qc(2);
    qc.cx(1, 1);
    check("Gate: control equal to target is an error", !qc.ok() && qc.size() == 0);
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

  printf("Measurement order (Python template rule)\n");
  {
    QuantumCircuit qc(1, 1);
    qc.measure(0, 0).x(0);
    uint16_t c[2];
    check("Gate after measure fails", !simulate_counts(qc, 10, c));
  }
  {
    QuantumCircuit qc(2, 2);
    qc.h(0).measure(0, 0).cx(0, 1).measure(1, 1);
    uint16_t c[4];
    check("cx after measuring its control fails", !simulate_counts(qc, 10, c));
  }
  {
    QuantumCircuit qc(2, 2);
    qc.measure(0, 1).measure(1, 0).x(0);
    uint16_t c[4];
    check("Gate after measure(q, b) with q != b fails", !simulate_counts(qc, 10, c));
  }
  {
    QuantumCircuit qc(1, 1);
    qc.measure(0, 0).x(0);
    ComplexNumber a[2];
    check("Statevector ignores the rule", simulate_statevector(qc, a));
  }
  {
    QuantumCircuit qc(1);
    qc.h(0);
    uint16_t c[1];
    check("counts without classical bits fails", !simulate_counts(qc, 10, c));
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

  printf("Linking\n");
  check("Header included from two .cpp files links and runs", second_unit_bell_count() == 100);

  printf(failures == 0 ? "\nAll host tests passed.\n" : "\n%d test(s) FAILED.\n", failures);
  return failures == 0 ? 0 : 1;
}
