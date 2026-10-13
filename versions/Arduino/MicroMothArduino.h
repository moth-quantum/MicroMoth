// (C) Copyright Moth Quantum 2024, 2026.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

// (C) Copyright IBM 2023.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

// MicroMoth for Arduino: one header, no dependencies beyond the Arduino core.
//
// Behaviour follows the Python template (micromoth.py at the repo root).
// Structure follows the C# port: one gate record, validated gate methods,
// a clbit -> qubit map, integer outcomes and a seedable random generator.
//
// Microcontroller rules this file keeps:
//   - No exceptions. A bad call records an error, adds nothing, and simulate_*() returns false.
//   - No hidden heap use while you build a circuit beyond the gate list itself.
//   - Results go into buffers you own, so you decide where the memory comes from.
//   - Every function is inline, so the header can be included from several .cpp files.

#ifndef MICROMOTHARDUINO_H
#define MICROMOTHARDUINO_H

#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// The largest circuit this build accepts. Override it by defining it before the #include.
// The statevector needs 8 bytes per amplitude: 6 qubits = 512 bytes, 10 qubits = 8 KB, 14 qubits = 128 KB.
#ifndef MICROMOTH_MAX_QUBITS
  #if defined(__AVR__)
    #define MICROMOTH_MAX_QUBITS 10
  #else
    #define MICROMOTH_MAX_QUBITS 16
  #endif
#endif

// Outcomes are 16-bit integers, and simulate_counts() needs 2^num_clbits tallies of 2 bytes.
#ifndef MICROMOTH_MAX_CLBITS
  #if defined(__AVR__)
    #define MICROMOTH_MAX_CLBITS 14
  #else
    #define MICROMOTH_MAX_CLBITS 16
  #endif
#endif

// Buffer sizes are computed in size_t, which is only 16 bits on AVR boards.
// A statevector takes 2^n * 8 bytes and a counts buffer 2^m * 2 bytes; both must fit.
static_assert(MICROMOTH_MAX_QUBITS >= 1 && MICROMOTH_MAX_QUBITS <= 24 &&
              MICROMOTH_MAX_QUBITS + 3 < 8 * sizeof(size_t),
              "MICROMOTH_MAX_QUBITS is too large for this board");
static_assert(MICROMOTH_MAX_CLBITS >= 1 && MICROMOTH_MAX_CLBITS <= 16 &&
              MICROMOTH_MAX_CLBITS + 1 < 8 * sizeof(size_t),
              "MICROMOTH_MAX_CLBITS is too large for this board");

// A complex amplitude. Aggregate, so `ComplexNumber s[2] = {{1, 0}, {0, 0}};` works.
struct ComplexNumber {
  float real;
  float imag;
};

namespace micromoth_detail {

constexpr float SQRT1_2 = 0.70710678f;
constexpr float PI_F = 3.14159265f;

// The reason the last simulate_*() call failed, or nullptr.
inline const __FlashStringHelper*& last_error() {
  static const __FlashStringHelper* error = nullptr;
  return error;
}

inline bool fail(const __FlashStringHelper* why) {
  last_error() = why;
  return false;
}

// xorshift32: fast on 8-bit cores, and the same seed gives the same samples on every board.
inline uint32_t& rng_state() {
  static uint32_t state = 0x6D2B79F5u;
  return state;
}

// A float in [0, 1) with 24 random bits, the full precision of a float.
inline float random01() {
  uint32_t x = rng_state();
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state() = x;
  return (float)(x >> 8) * (1.0f / 16777216.0f);
}

// True for every finite float, false for NaN and infinity, without relying on isfinite().
inline bool finite(float v) {
  return (v - v) == 0.0f;
}

// Element count times element size, or 0 if it does not fit this board's memory addressing.
inline size_t bytes_for(uint32_t count, size_t element) {
  const uint32_t total = count * (uint32_t)element;
  if (count != 0 && total / count != element) return 0;
  if ((uint32_t)(size_t)total != total) return 0;
  return (size_t)total;
}

}  // namespace micromoth_detail

// Sets the random seed for sampling. For different results on every power-up, use for example
// micromoth_seed(analogRead(A0)) with A0 left unconnected.
inline void micromoth_seed(uint32_t seed) {
  uint32_t s = seed * 2654435761u ^ 0x6D2B79F5u;
  micromoth_detail::rng_state() = s ? s : 0x6D2B79F5u;
}

// Why the last simulate_*() call returned false, or nullptr if it succeeded. Print it with Serial.println().
inline const __FlashStringHelper* micromoth_error() {
  return micromoth_detail::last_error();
}

class QuantumCircuit {
 public:
  // The gate tags of the Python template.
  enum class Op : uint8_t { init, x, h, rx, rz, cx, crx, swap, measure };

  // One record shape for every gate, as in the C# port.
  //   single-qubit gates: a = target qubit
  //   two-qubit gates:    a = control qubit, b = target qubit
  //   measure:            a = qubit, b = classical bit
  struct Gate {
    Op op;
    uint8_t a;
    uint8_t b;
    float theta;
  };

  const char* name = "";

  explicit QuantumCircuit(int num_qubits, int num_clbits = 0) {
    if (num_qubits < 1 || num_qubits > MICROMOTH_MAX_QUBITS) {
      set_error(F("num_qubits must be from 1 to MICROMOTH_MAX_QUBITS"));
      num_qubits = 1;
    }
    if (num_clbits < 0 || num_clbits > MICROMOTH_MAX_CLBITS) {
      set_error(F("num_clbits must be from 0 to MICROMOTH_MAX_CLBITS"));
      num_clbits = 0;
    }
    _qubits = (uint8_t)num_qubits;
    _clbits = (uint8_t)num_clbits;
  }

  ~QuantumCircuit() {
    free(_gates);
    free(_init);
  }

  // A circuit owns its memory, so it cannot be copied by accident.
  QuantumCircuit(const QuantumCircuit&) = delete;
  QuantumCircuit& operator=(const QuantumCircuit&) = delete;

  // ===== Inspection =====

  int num_qubits() const { return _qubits; }
  int num_clbits() const { return _clbits; }
  int size() const { return _size; }
  const Gate* data() const { return _gates; }
  const ComplexNumber* initial_state() const { return _init; }

  // False once any call was invalid or memory ran out. error() says which problem came first.
  bool ok() const { return _error == nullptr; }
  const __FlashStringHelper* error() const { return _error; }

  // ===== Basic gates: the only ones the simulator knows =====

  QuantumCircuit& x(int q) { return single(Op::x, q, 0.0f); }
  QuantumCircuit& h(int q) { return single(Op::h, q, 0.0f); }
  QuantumCircuit& rx(float theta, int q) { return single(Op::rx, q, theta); }
  QuantumCircuit& rz(float theta, int q) { return single(Op::rz, q, theta); }
  QuantumCircuit& cx(int s, int t) { return pair(Op::cx, s, t, 0.0f); }
  QuantumCircuit& crx(float theta, int s, int t) { return pair(Op::crx, s, t, theta); }
  QuantumCircuit& swap(int s, int t) { return pair(Op::swap, s, t, 0.0f); }

  // ===== Gates built from the basic ones =====
  // Arguments are checked first, so a bad call never leaves half a gate behind.

  QuantumCircuit& ry(float theta, int q) {
    if (!qubit_ok(q)) return set_error(F("qubit index out of range"));
    if (!micromoth_detail::finite(theta)) return set_error(F("angle must be a finite number"));
    const float half_pi = micromoth_detail::PI_F / 2;
    return rx(half_pi, q).rz(theta, q).rx(-half_pi, q);
  }

  QuantumCircuit& z(int q) { return rz(micromoth_detail::PI_F, q); }
  QuantumCircuit& t(int q) { return rz(micromoth_detail::PI_F / 4, q); }

  QuantumCircuit& y(int q) {
    if (!qubit_ok(q)) return set_error(F("qubit index out of range"));
    return rz(micromoth_detail::PI_F, q).x(q);
  }

  // Controlled rz, using h·rx·h = rz on the target.
  QuantumCircuit& crz(float theta, int s, int t) {
    if (!pair_ok(s, t)) return *this;
    if (!micromoth_detail::finite(theta)) return set_error(F("angle must be a finite number"));
    return h(t).crx(theta, s, t).h(t);
  }

  // ===== Measurement =====

  QuantumCircuit& measure(int q, int b) {
    if (!qubit_ok(q)) return set_error(F("qubit index out of range"));
    if (b < 0 || b >= _clbits) return set_error(F("classical bit index out of range"));
    return push(Gate{Op::measure, (uint8_t)q, (uint8_t)b, 0.0f});
  }

  QuantumCircuit& measure_all() {
    if (_clbits == 0) _clbits = _qubits;  // Create the classical register if absent.
    if (_clbits < _qubits) return set_error(F("measure_all needs at least num_qubits classical bits"));
    for (int q = 0; q < _qubits; q++) measure(q, q);
    return *this;
  }

  // ===== Initialisation =====
  // Takes 2^num_qubits amplitudes and discards any gates added before it, as in Python.
  // The state is copied, so it costs another 8 bytes per amplitude while the circuit exists.

  QuantumCircuit& initialize(const ComplexNumber* state, size_t length) {
    if (!begin_initialize(state != nullptr, length)) return *this;
    float total = 0;
    for (size_t i = 0; i < length; i++) {
      if (!micromoth_detail::finite(state[i].real) || !micromoth_detail::finite(state[i].imag)) {
        return set_error(F("amplitudes must be finite numbers"));
      }
      total += state[i].real * state[i].real + state[i].imag * state[i].imag;
    }
    if (total == 0) return set_error(F("statevector must not be all zeros"));
    if (!reserve_init()) return *this;
    memcpy(_init, state, length * sizeof(ComplexNumber));
    return finish_initialize();
  }

  // Real amplitudes only, as the Python template also accepts.
  QuantumCircuit& initialize(const float* state, size_t length) {
    if (!begin_initialize(state != nullptr, length)) return *this;
    float total = 0;
    for (size_t i = 0; i < length; i++) {
      if (!micromoth_detail::finite(state[i])) return set_error(F("amplitudes must be finite numbers"));
      total += state[i] * state[i];
    }
    if (total == 0) return set_error(F("statevector must not be all zeros"));
    if (!reserve_init()) return *this;
    for (size_t i = 0; i < length; i++) _init[i] = ComplexNumber{state[i], 0.0f};
    return finish_initialize();
  }

  // With a plain array the length is checked for you: qc.initialize(state);
  template <size_t N>
  QuantumCircuit& initialize(const ComplexNumber (&state)[N]) { return initialize(state, N); }
  template <size_t N>
  QuantumCircuit& initialize(const float (&state)[N]) { return initialize(state, N); }

 private:
  uint8_t _qubits = 1;
  uint8_t _clbits = 0;
  uint16_t _size = 0;
  uint16_t _capacity = 0;
  Gate* _gates = nullptr;
  ComplexNumber* _init = nullptr;
  const __FlashStringHelper* _error = nullptr;

  QuantumCircuit& set_error(const __FlashStringHelper* why) {
    if (!_error) _error = why;  // Keep the first problem: later ones are often caused by it.
    return *this;
  }

  bool qubit_ok(int q) const { return q >= 0 && q < _qubits; }

  bool pair_ok(int s, int t) {
    if (!qubit_ok(s) || !qubit_ok(t)) {
      set_error(F("qubit index out of range"));
      return false;
    }
    if (s == t) {
      set_error(F("control and target qubits must differ"));
      return false;
    }
    return true;
  }

  // The gate list grows by doubling, so adding n gates copies O(n) records in total.
  QuantumCircuit& push(const Gate& g) {
    if (_size == _capacity) {
      if (_capacity >= 0x8000u) return set_error(F("too many gates"));
      const uint16_t grown = _capacity ? (uint16_t)(_capacity * 2) : 8;
      Gate* bigger = (Gate*)realloc(_gates, (size_t)grown * sizeof(Gate));
      if (!bigger) return set_error(F("out of memory for gates"));
      _gates = bigger;
      _capacity = grown;
    }
    _gates[_size++] = g;
    return *this;
  }

  QuantumCircuit& single(Op op, int q, float theta) {
    if (!qubit_ok(q)) return set_error(F("qubit index out of range"));
    if (!micromoth_detail::finite(theta)) return set_error(F("angle must be a finite number"));
    return push(Gate{op, (uint8_t)q, 0, theta});
  }

  QuantumCircuit& pair(Op op, int s, int t, float theta) {
    if (!pair_ok(s, t)) return *this;
    if (!micromoth_detail::finite(theta)) return set_error(F("angle must be a finite number"));
    return push(Gate{op, (uint8_t)s, (uint8_t)t, theta});
  }

  bool begin_initialize(bool has_state, size_t length) {
    if (!has_state || length != ((size_t)1 << _qubits)) {
      set_error(F("initialize needs exactly 2^num_qubits amplitudes"));
      return false;
    }
    return true;
  }

  bool reserve_init() {
    if (_init) return true;
    _init = (ComplexNumber*)malloc(((size_t)1 << _qubits) * sizeof(ComplexNumber));
    if (!_init) set_error(F("out of memory for initialize"));
    return _init != nullptr;
  }

  QuantumCircuit& finish_initialize() {
    _size = 0;
    return push(Gate{Op::init, 0, 0, 0.0f});
  }
};

namespace micromoth_detail {

using Op = QuantumCircuit::Op;
using Gate = QuantumCircuit::Gate;

// ===== Maths helpers. Each updates two amplitudes in place, reading both before writing either. =====

// x: swap the two amplitudes.
inline void exchange(ComplexNumber& p, ComplexNumber& q) {
  const ComplexNumber held = p;
  p = q;
  q = held;
}

// h: (x + y)/sqrt(2) and (x - y)/sqrt(2).
inline void superpose(ComplexNumber& p, ComplexNumber& q) {
  const float xr = p.real, xi = p.imag, yr = q.real, yi = q.imag;
  p.real = SQRT1_2 * (xr + yr); p.imag = SQRT1_2 * (xi + yi);
  q.real = SQRT1_2 * (xr - yr); q.imag = SQRT1_2 * (xi - yi);
}

// rx: cos(θ/2)·x − i·sin(θ/2)·y and cos(θ/2)·y − i·sin(θ/2)·x, with c = cos(θ/2), s = sin(θ/2).
inline void turn(ComplexNumber& p, ComplexNumber& q, float c, float s) {
  const float xr = p.real, xi = p.imag, yr = q.real, yi = q.imag;
  p.real = c * xr + s * yi; p.imag = c * xi - s * yr;
  q.real = c * yr + s * xi; q.imag = c * yi - s * xr;
}

// rz: e^(−iθ/2)·x and e^(iθ/2)·y.
inline void phaseturn(ComplexNumber& p, ComplexNumber& q, float c, float s) {
  const float xr = p.real, xi = p.imag, yr = q.real, yi = q.imag;
  p.real = c * xr + s * xi; p.imag = c * xi - s * xr;
  q.real = c * yr - s * yi; q.imag = c * yi + s * yr;
}

// ===== Gate application =====

// A single-qubit gate mixes amplitude pairs whose indices differ only at the target bit.
inline void apply_single(ComplexNumber* amp, size_t dim, const Gate& g) {
  const size_t step = (size_t)1 << g.a;
  float c = 1, s = 0;
  if (g.op == Op::rx || g.op == Op::rz) {  // Trig once per gate: it is slow on chips without an FPU.
    c = cosf(g.theta * 0.5f);
    s = sinf(g.theta * 0.5f);
  }
  for (size_t base = 0; base < dim; base += step << 1) {
    for (size_t i = base; i < base + step; i++) {
      ComplexNumber& p = amp[i];
      ComplexNumber& q = amp[i + step];  // Same index with the target bit set.
      switch (g.op) {
        case Op::x:  exchange(p, q); break;
        case Op::h:  superpose(p, q); break;
        case Op::rx: turn(p, q, c, s); break;
        default:     phaseturn(p, q, c, s); break;  // Op::rz
      }
    }
  }
}

// A two-qubit gate visits each group of four amplitudes that differ only at the control and target bits.
inline void apply_two(ComplexNumber* amp, size_t dim, const Gate& g) {
  const uint8_t lo = g.a < g.b ? g.a : g.b;
  const uint8_t hi = g.a < g.b ? g.b : g.a;
  const size_t lo_bit = (size_t)1 << lo, hi_bit = (size_t)1 << hi;
  const size_t s_bit = (size_t)1 << g.a, t_bit = (size_t)1 << g.b;
  float c = 1, s = 0;
  if (g.op == Op::crx) {
    c = cosf(g.theta * 0.5f);
    s = sinf(g.theta * 0.5f);
  }
  for (size_t high = 0; high < dim; high += hi_bit << 1) {
    for (size_t mid = high; mid < high + hi_bit; mid += lo_bit << 1) {
      for (size_t i00 = mid; i00 < mid + lo_bit; i00++) {  // control 0, target 0
        const size_t i10 = i00 + s_bit;                     // control 1, target 0
        const size_t i11 = i10 + t_bit;                     // control 1, target 1
        switch (g.op) {
          case Op::cx:  exchange(amp[i10], amp[i11]); break;
          case Op::crx: turn(amp[i10], amp[i11], c, s); break;
          default:      exchange(amp[i00 + t_bit], amp[i10]); break;  // Op::swap
        }
      }
    }
  }
}

// Runs every gate on |0...0> and leaves the statevector in amp (2^num_qubits entries).
inline void evolve(const QuantumCircuit& qc, ComplexNumber* amp) {
  const size_t dim = (size_t)1 << qc.num_qubits();
  memset(amp, 0, dim * sizeof(ComplexNumber));
  amp[0].real = 1;

  const Gate* gates = qc.data();
  for (int k = 0; k < qc.size(); k++) {
    const Gate& g = gates[k];
    switch (g.op) {
      case Op::init:    memcpy(amp, qc.initial_state(), dim * sizeof(ComplexNumber)); break;
      case Op::measure: break;  // Measurements are sampled at the end.
      case Op::x: case Op::h: case Op::rx: case Op::rz:
        apply_single(amp, dim, g);
        break;
      default:  // Op::cx, Op::crx, Op::swap
        apply_two(amp, dim, g);
        break;
    }
  }
}

// ===== Checks shared by the simulate functions =====

inline bool check_circuit(const QuantumCircuit& qc) {
  last_error() = nullptr;
  if (!qc.ok()) return fail(qc.error());
  return true;
}

// The noise model is one measurement-error probability per qubit, as in the Python template.
inline bool check_noise(const QuantumCircuit& qc, const float* noise) {
  if (!noise) return true;
  for (int j = 0; j < qc.num_qubits(); j++) {
    if (!(noise[j] >= 0.0f && noise[j] <= 1.0f)) return fail(F("noise probabilities must be within 0 and 1"));
  }
  return true;
}

// Measurements are deferred to the end, so no gate may touch a qubit after it is measured.
inline bool check_measured_last(const QuantumCircuit& qc) {
  uint32_t measured = 0;
  const Gate* gates = qc.data();
  for (int k = 0; k < qc.size(); k++) {
    const Gate& g = gates[k];
    switch (g.op) {
      case Op::measure:
        measured |= (uint32_t)1 << g.a;
        break;
      case Op::init:
        if (measured) return fail(F("initialize cannot follow a measurement"));
        break;
      case Op::x: case Op::h: case Op::rx: case Op::rz:
        if (measured & ((uint32_t)1 << g.a)) return fail(F("a gate acts on a qubit after it was measured"));
        break;
      default:
        if (measured & (((uint32_t)1 << g.a) | ((uint32_t)1 << g.b))) {
          return fail(F("a gate acts on a qubit after it was measured"));
        }
        break;
    }
  }
  return true;
}

// A measurement error on qubit j mixes the probabilities of outcomes that differ only at bit j.
// Probabilities are read from and written to the `real` field of each entry.
inline void apply_noise(ComplexNumber* probs, size_t dim, int n, const float* noise) {
  if (!noise) return;
  for (int j = 0; j < n; j++) {
    const float p = noise[j];
    if (p == 0) continue;
    const size_t step = (size_t)1 << j;
    for (size_t base = 0; base < dim; base += step << 1) {
      for (size_t i = base; i < base + step; i++) {
        const float p0 = probs[i].real, p1 = probs[i + step].real;
        probs[i].real = (1 - p) * p0 + p * p1;
        probs[i + step].real = (1 - p) * p1 + p * p0;
      }
    }
  }
}

// Samples outcomes using one statevector-sized buffer: the amplitudes are replaced, in place,
// by probabilities and then by running totals, which are searched by bisection for each shot.
class Sampler {
 public:
  Sampler(const QuantumCircuit& qc, const float* noise) : _qc(qc) {
    if (!check_circuit(qc) || !check_noise(qc, noise)) return;
    if (qc.num_clbits() == 0) { fail(F("counts and memory need classical bits: use measure or measure_all")); return; }
    if (!check_measured_last(qc)) return;

    _dim = (size_t)1 << qc.num_qubits();
    _cdf = (ComplexNumber*)malloc(_dim * sizeof(ComplexNumber));
    if (!_cdf) { fail(F("out of memory for the statevector")); return; }
    evolve(qc, _cdf);

    for (size_t i = 0; i < _dim; i++) {
      _cdf[i].real = _cdf[i].real * _cdf[i].real + _cdf[i].imag * _cdf[i].imag;
    }
    apply_noise(_cdf, _dim, qc.num_qubits(), noise);
    float total = 0;
    for (size_t i = 0; i < _dim; i++) {
      if (_cdf[i].real > 0) _last_nonzero = i;
      total += _cdf[i].real;
      _cdf[i].real = total;
    }
    _total = total;

    for (int b = 0; b < MICROMOTH_MAX_CLBITS; b++) _map[b] = -1;  // Unmeasured bits read 0.
    const Gate* gates = qc.data();
    for (int k = 0; k < qc.size(); k++) {
      if (gates[k].op == Op::measure) _map[gates[k].b] = (int8_t)gates[k].a;
    }
  }

  ~Sampler() { free(_cdf); }
  Sampler(const Sampler&) = delete;
  Sampler& operator=(const Sampler&) = delete;

  bool ready() const { return _cdf != nullptr; }

  // One shot. Bit b of the result is the value read into classical bit b.
  uint16_t shot() const {
    const float r = random01() * _total;  // Scaling by the total tolerates an unnormalised state.
    size_t lo = 0, hi = _dim - 1;
    while (lo < hi) {
      const size_t mid = lo + ((hi - lo) >> 1);
      if (_cdf[mid].real > r) hi = mid; else lo = mid + 1;
    }
    if (_cdf[lo].real <= r) lo = _last_nonzero;  // Floating-point round-off guard.

    uint16_t out = 0;
    for (int b = 0; b < _qc.num_clbits(); b++) {
      if (_map[b] >= 0 && ((lo >> _map[b]) & 1u)) out |= (uint16_t)(1u << b);
    }
    return out;
  }

 private:
  const QuantumCircuit& _qc;
  ComplexNumber* _cdf = nullptr;
  size_t _dim = 0;
  size_t _last_nonzero = 0;
  float _total = 0;
  int8_t _map[MICROMOTH_MAX_CLBITS];
};

}  // namespace micromoth_detail

// ===== Simulation into your own buffers. Each returns false on failure; micromoth_error() says why. =====

// The statevector, 2^num_qubits amplitudes. Uses no memory beyond `out`.
inline bool simulate_statevector(const QuantumCircuit& qc, ComplexNumber* out) {
  if (!micromoth_detail::check_circuit(qc)) return false;
  if (!out) return micromoth_detail::fail(F("no output buffer"));
  micromoth_detail::evolve(qc, out);
  return true;
}

// The probability of each basis state, 2^num_qubits floats. Briefly allocates the statevector.
inline bool simulate_probabilities(const QuantumCircuit& qc, float* out, const float* noise = nullptr) {
  if (!micromoth_detail::check_circuit(qc) || !micromoth_detail::check_noise(qc, noise)) return false;
  if (!out) return micromoth_detail::fail(F("no output buffer"));
  const size_t dim = (size_t)1 << qc.num_qubits();
  ComplexNumber* amp = (ComplexNumber*)malloc(dim * sizeof(ComplexNumber));
  if (!amp) return micromoth_detail::fail(F("out of memory for the statevector"));
  micromoth_detail::evolve(qc, amp);
  for (size_t i = 0; i < dim; i++) amp[i].real = amp[i].real * amp[i].real + amp[i].imag * amp[i].imag;
  micromoth_detail::apply_noise(amp, dim, qc.num_qubits(), noise);
  for (size_t i = 0; i < dim; i++) out[i] = amp[i].real;
  free(amp);
  return true;
}

// Tallies `shots` samples into counts[outcome], which needs 2^num_clbits entries.
// The outcome's bitstring is its binary form with classical bit 0 on the right.
inline bool simulate_counts(const QuantumCircuit& qc, uint16_t shots, uint16_t* counts, const float* noise = nullptr) {
  micromoth_detail::Sampler sampler(qc, noise);
  if (!sampler.ready()) return false;
  if (!counts) return micromoth_detail::fail(F("no output buffer"));
  memset(counts, 0, ((size_t)1 << qc.num_clbits()) * sizeof(uint16_t));
  for (uint16_t k = 0; k < shots; k++) counts[sampler.shot()]++;
  return true;
}

// Writes one outcome per shot into outcomes[0 .. shots-1], in sampling order.
inline bool simulate_memory(const QuantumCircuit& qc, uint16_t shots, uint16_t* outcomes, const float* noise = nullptr) {
  micromoth_detail::Sampler sampler(qc, noise);
  if (!sampler.ready()) return false;
  if (!outcomes) return micromoth_detail::fail(F("no output buffer"));
  for (uint16_t k = 0; k < shots; k++) outcomes[k] = sampler.shot();
  return true;
}

// ===== Printing =====

namespace micromoth_detail {
// Round-off such as cos(pi/2) = -4e-8 would otherwise print as "-0.0000".
inline float printable(float v) {
  return (v > -0.00005f && v < 0.00005f) ? 0.0f : v;
}
}  // namespace micromoth_detail

// Prints `value` as `width` binary digits, most significant first.
inline void print_bits(Print& out, uint32_t value, int width) {
  for (int b = width - 1; b >= 0; b--) out.print(((value >> b) & 1u) ? '1' : '0');
}

// The Python-style entry point: simulates and prints the result, one line per entry.
//   get = "counts"             -> "00: 509" for each outcome seen
//   get = "memory"             -> one bitstring per shot
//   get = "statevector"        -> "00: 0.7071 + 0.0000i" for each basis state
//   get = "probabilities_dict" -> "00: 0.5000" for each basis state
// It allocates the result buffer for you and prints the reason if anything fails.
inline bool simulate(const QuantumCircuit& qc, uint16_t shots = 1024, const char* get = "counts",
                     const float* noise = nullptr, Print& out = Serial) {
  using micromoth_detail::bytes_for;
  const int n = qc.num_qubits(), m = qc.num_clbits();
  const uint32_t states = (uint32_t)1 << n, outcomes = (uint32_t)1 << m;
  bool done = false;

  if (strcmp(get, "counts") == 0) {
    const size_t bytes = bytes_for(outcomes, sizeof(uint16_t));
    uint16_t* counts = bytes ? (uint16_t*)malloc(bytes) : nullptr;
    if (!counts) {
      micromoth_detail::fail(F("out of memory for counts"));
    } else if ((done = simulate_counts(qc, shots, counts, noise))) {
      for (uint32_t i = 0; i < outcomes; i++) {
        if (counts[i] == 0) continue;
        print_bits(out, i, m);
        out.print(F(": "));
        out.println(counts[i]);
      }
    }
    free(counts);
  } else if (strcmp(get, "memory") == 0) {
    const size_t bytes = bytes_for(shots ? shots : 1, sizeof(uint16_t));
    uint16_t* memory = bytes ? (uint16_t*)malloc(bytes) : nullptr;
    if (!memory) {
      micromoth_detail::fail(F("out of memory for memory"));
    } else if ((done = simulate_memory(qc, shots, memory, noise))) {
      for (uint16_t k = 0; k < shots; k++) {
        print_bits(out, memory[k], m);
        out.println();
      }
    }
    free(memory);
  } else if (strcmp(get, "statevector") == 0) {
    const size_t bytes = bytes_for(states, sizeof(ComplexNumber));
    ComplexNumber* amp = bytes ? (ComplexNumber*)malloc(bytes) : nullptr;
    if (!amp) {
      micromoth_detail::fail(F("out of memory for the statevector"));
    } else if ((done = simulate_statevector(qc, amp))) {
      for (uint32_t i = 0; i < states; i++) {
        print_bits(out, i, n);
        out.print(F(": "));
        const float re = micromoth_detail::printable(amp[i].real);
        const float im = micromoth_detail::printable(amp[i].imag);
        out.print(re, 4);
        out.print(im < 0 ? F(" - ") : F(" + "));
        out.print(im < 0 ? -im : im, 4);
        out.println('i');
      }
    }
    free(amp);
  } else if (strcmp(get, "probabilities_dict") == 0) {
    const size_t bytes = bytes_for(states, sizeof(float));
    float* probs = bytes ? (float*)malloc(bytes) : nullptr;
    if (!probs) {
      micromoth_detail::fail(F("out of memory for probabilities"));
    } else if ((done = simulate_probabilities(qc, probs, noise))) {
      for (uint32_t i = 0; i < states; i++) {
        print_bits(out, i, n);
        out.print(F(": "));
        out.println(micromoth_detail::printable(probs[i]), 4);
      }
    }
    free(probs);
  } else {
    micromoth_detail::fail(F("get must be counts, memory, statevector or probabilities_dict"));
  }

  if (!done) {
    out.print(F("MicroMoth error: "));
    out.println(micromoth_error());
  }
  return done;
}

#endif  // MICROMOTHARDUINO_H
