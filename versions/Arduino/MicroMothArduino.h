// (C) Copyright Moth Quantum 2024.
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

#ifndef MICROMOTHARDUINO_H
#define MICROMOTHARDUINO_H

#include "MicroMothArduinoMath.h"

class QuantumCircuit {
  public:
    enum GateOp { INIT, X, RX, RZ, H, CX, CRX, CRZ, SWAP, RY, Z, T, Y, M };

    struct Op {
      GateOp gate;
      float angle;
      int control;
      int target;

      Op(GateOp g = INIT, float a = 0.0f, int q1 = 0, int q2 = 0) : gate(g), angle(a), control(q1), target(q2) {}
    };

    int num_qubits;
    int num_clbits;

    Op* data;
    int size;
    int capacity;

    String name;

    ComplexNumber* statevectors;

    QuantumCircuit(int n, int m = 0) : num_qubits(n), num_clbits(m), size(0), capacity(10) {
      name = "";
      data = new Op[capacity];
      statevectors = nullptr;
    }

    ~QuantumCircuit() {
      delete[] data;
      delete[] statevectors;
    }

    void resize() {
      capacity += 2;
      Op* temp = new Op[capacity];
      for (int i = 0; i < size; i++) {
        temp[i] = data[i];
      }
      delete[] data;
      data = temp;
    }

    // State initialisation: k[0] is the computational basis state index to start from.
    // Clears all previous gates and inserts an INIT op as the first instruction.
    void initialise(const int* k, int s) {
      size = 0;
      data[0].gate = INIT;
      data[0].angle = 0.0f;
      data[0].control = s;
      data[0].target = (s > 0) ? k[0] : 0;
      size = 1;
    }

    void x(int q) {
      if (size >= capacity) resize();
      data[size++] = Op(X, 0.0f, 0, q);
    }

    void h(int q) {
      if (size >= capacity) resize();
      data[size++] = Op(H, 0.0f, 0, q);
    }

    void cx(int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CX, 0.0f, s, t);
    }

    void rx(float theta, int q) {
      if (size >= capacity) resize();
      data[size++] = Op(RX, theta, 0, q);
    }

    void ry(float theta, int q) {
      rx(HALF_PI, q);
      rz(theta, q);
      rx(-(HALF_PI), q);
    }

    void rz(float theta, int q) {
      if (size >= capacity) resize();
      data[size++] = Op(RZ, theta, 0, q);
    }

    void y(int q) {
      rz(PI, q);
      x(q);
    }

    void z(int q) {
      rz(PI, q);
    }

    void t(int q) {
      rz(PI / 4.0f, q);
    }

    void crx(float theta, int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CRX, theta, s, t);
    }

    void crz(float theta, int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CRZ, theta, s, t);
    }

    void swap(int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(SWAP, 0.0f, s, t);
    }

    void measure(int q, int b) {
      if (b >= num_clbits) {
        Serial.println(F("Error: Index for output bit out of range."));
        return;
      }
      if (q >= num_qubits) {
        Serial.println(F("Error: Index for qubit out of range."));
        return;
      }
      if (size >= capacity) resize();
      data[size++] = Op(M, 0.0f, q, b);
    }

    void measure_all() {
      if (num_clbits == 0) {
        num_clbits = num_qubits;
      }
      for (int q = 0; q < num_qubits; q++) {
        measure(q, q);
      }
    }

    // Simulates the quantum circuit qc and outputs results via Serial.
    // get: "counts" (default), "statevector", or "memory"
    // noiseModel: array of num_qubits measurement-error probabilities, or nullptr
    void simulate(QuantumCircuit &qc, int shots = 1024, const char* get = "counts", const float* noiseModel = nullptr) {
      int ssize = 1 << qc.num_qubits;

      // Free stale allocation first so system_check sees accurately-available RAM
      delete[] statevectors;
      statevectors = nullptr;

      int required = (int)(sizeof(ComplexNumber) * ssize) + 200;
      system_check(required);

      statevectors = new ComplexNumber[ssize];
      if (!statevectors) {
        Serial.println(F("Error: out of memory (statevectors)"));
        return;
      }

      for (int i = 0; i < ssize; i++) {
        statevectors[i] = {0.0f, 0.0f};
      }
      statevectors[0] = {1.0f, 0.0f};

      // Fix 3: pre-scan to build outputmap (clbit -> qubit) from M ops
      int clbits = (qc.num_clbits > 0) ? qc.num_clbits : 1;
      int* outputmap = new int[clbits];
      if (!outputmap) {
        Serial.println(F("Error: out of memory (outputmap)"));
        return;
      }
      for (int i = 0; i < clbits; i++) outputmap[i] = -1;
      for (int i = 0; i < qc.size; i++) {
        if (qc.data[i].gate == QuantumCircuit::M) {
          int qubit = qc.data[i].control;
          int clbit = qc.data[i].target;
          if (clbit < clbits) outputmap[clbit] = qubit;
        }
      }

      // Gate operations
      for (int i = 0; i < qc.size; i++) {
        Op g = qc.data[i];
        int j = g.target;

        // Fix 1+2: INIT handler — reset statevector to given basis state
        if (g.gate == QuantumCircuit::INIT) {
          for (int idx = 0; idx < ssize; idx++) statevectors[idx] = {0.0f, 0.0f};
          statevectors[g.target] = {1.0f, 0.0f};
        }
        // Fix 3: M ops are bookkeeping only; statevector is unchanged
        else if (g.gate == QuantumCircuit::M) {
          // handled by outputmap pre-scan above
        }
        else if (g.gate == QuantumCircuit::X) {
          for (int i0 = 0; i0 < (1 << j); i0++) {
            for (int i1 = 0; i1 < (1 << (qc.num_qubits - j - 1)); i1++) {
              int b0 = i0 + (1 << (j + 1)) * i1;
              int b1 = b0 + (1 << j);
              ComplexNumber temp = statevectors[b0];
              statevectors[b0] = statevectors[b1];
              statevectors[b1] = temp;
            }
          }
        }
        else if (g.gate == QuantumCircuit::H) {
          for (int i0 = 0; i0 < (1 << j); i0++) {
            for (int i1 = 0; i1 < (1 << (qc.num_qubits - j - 1)); i1++) {
              int b0 = i0 + (1 << (j + 1)) * i1;
              int b1 = b0 + (1 << j);
              ComplexNumber* r = qc.superposition(statevectors[b0], statevectors[b1]);
              statevectors[b0] = r[0];
              statevectors[b1] = r[1];
            }
          }
        }
        else if (g.gate == QuantumCircuit::RX) {
          float th = g.angle;
          for (int i0 = 0; i0 < (1 << j); i0++) {
            for (int i1 = 0; i1 < (1 << (qc.num_qubits - j - 1)); i1++) {
              int b0 = i0 + (1 << (j + 1)) * i1;
              int b1 = b0 + (1 << j);
              ComplexNumber* r = qc.rotate(statevectors[b0], statevectors[b1], th);
              statevectors[b0] = r[0];
              statevectors[b1] = r[1];
            }
          }
        }
        else if (g.gate == QuantumCircuit::RZ) {
          float th = g.angle;
          for (int i0 = 0; i0 < (1 << j); i0++) {
            for (int i1 = 0; i1 < (1 << (qc.num_qubits - j - 1)); i1++) {
              int b0 = i0 + (1 << (j + 1)) * i1;
              int b1 = b0 + (1 << j);
              ComplexNumber* r = qc.phaseturn(statevectors[b0], statevectors[b1], th);
              statevectors[b0] = r[0];
              statevectors[b1] = r[1];
            }
          }
        }
        else if (g.gate == QuantumCircuit::CX) {
          int c = g.control;
          int t = g.target;
          int l = min(c, t);
          int h = max(c, t);
          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b10 = b00 + (1 << c);
                int b11 = b10 + (1 << t);
                ComplexNumber temp = statevectors[b10];
                statevectors[b10] = statevectors[b11];
                statevectors[b11] = temp;
              }
            }
          }
        }
        else if (g.gate == QuantumCircuit::SWAP) {
          int c = g.control;
          int t = g.target;
          int l = min(c, t);
          int h = max(c, t);
          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b01 = b00 + (1 << t);
                int b10 = b00 + (1 << c);
                ComplexNumber temp = statevectors[b01];
                statevectors[b01] = statevectors[b10];
                statevectors[b10] = temp;
              }
            }
          }
        }
        else if (g.gate == QuantumCircuit::CRX) {
          float th = g.angle;
          int c = g.control;
          int t = g.target;
          int l = min(c, t);
          int h = max(c, t);
          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b10 = b00 + (1 << c);
                int b11 = b10 + (1 << t);
                ComplexNumber* r = qc.rotate(statevectors[b10], statevectors[b11], th);
                statevectors[b10] = r[0];
                statevectors[b11] = r[1];
              }
            }
          }
        }
        else if (g.gate == QuantumCircuit::CRZ) {
          float th = g.angle;
          int c = g.control;
          int t = g.target;
          int l = min(c, t);
          int h = max(c, t);
          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b10 = b00 + (1 << c);
                int b11 = b10 + (1 << t);
                ComplexNumber* r = qc.phaseturn(statevectors[b10], statevectors[b11], th);
                statevectors[b10] = r[0];
                statevectors[b11] = r[1];
              }
            }
          }
        }
        // Fix 6: circuitPrint removed from gate loop
      }

      if (strcmp(get, "statevector") == 0) {
        delete[] outputmap;
        return;
      }

      // Fix 3: compute probabilities from statevector
      float* probs = new float[ssize];
      if (!probs) {
        Serial.println(F("Error: out of memory (probs)"));
        delete[] outputmap;
        return;
      }
      for (int i = 0; i < ssize; i++) {
        probs[i] = statevectors[i].real * statevectors[i].real + statevectors[i].imag * statevectors[i].imag;
      }

      // Fix 5: apply noise model AFTER probs are computed; read-only access to noiseModel
      if (noiseModel) {
        for (int j = 0; j < qc.num_qubits; j++) {
          float p_meas = noiseModel[j];
          for (int i0 = 0; i0 < (1 << j); i0++) {
            for (int i1 = 0; i1 < (1 << (qc.num_qubits - j - 1)); i1++) {
              int b0 = i0 + (1 << (j + 1)) * i1;
              int b1 = b0 + (1 << j);
              float p0 = probs[b0];
              float p1 = probs[b1];
              probs[b0] = (1.0f - p_meas) * p0 + p_meas * p1;
              probs[b1] = (1.0f - p_meas) * p1 + p_meas * p0;
            }
          }
        }
      }

      // Fix 3: sample shots and accumulate counts, matching Python micromoth.py lines 255-271
      if (strcmp(get, "counts") == 0 || strcmp(get, "memory") == 0) {
        int num_cstates = 1 << qc.num_clbits;
        int* counts = new int[num_cstates];
        if (!counts) {
          Serial.println(F("Error: out of memory (counts)"));
          delete[] probs;
          delete[] outputmap;
          return;
        }
        for (int i = 0; i < num_cstates; i++) counts[i] = 0;

        for (int shot = 0; shot < shots; shot++) {
          float cumu = 0.0f;
          float r = custom_random(0.0f, 1.0f);
          for (int j = 0; j < ssize; j++) {
            cumu += probs[j];
            if (r < cumu) {
              // Map qubit-state index j to classical output integer via outputmap
              int out = 0;
              for (int bit = 0; bit < qc.num_clbits; bit++) {
                if (outputmap[bit] >= 0) {
                  out |= (((j >> outputmap[bit]) & 1) << bit);
                }
              }
              counts[out]++;
              break;
            }
          }
        }

        Serial.println(F("Counts:"));
        for (int i = 0; i < num_cstates; i++) {
          if (counts[i] > 0) {
            for (int b = qc.num_clbits - 1; b >= 0; b--) {
              Serial.print((i >> b) & 1);
            }
            Serial.print(F(": "));
            Serial.println(counts[i]);
          }
        }

        delete[] counts;
      }

      delete[] probs;
      delete[] outputmap;
    }

    void circuitPrint(QuantumCircuit::GateOp tg) {
      Serial.println(F("+++Statevector+++"));
      Serial.println(tg);
      for (int i = 0; i < (1 << num_qubits); i++) {
        Serial.print(F("Amplitude of state |"));
        Serial.print(i, BIN);
        Serial.print(F(">: "));
        Serial.print(statevectors[i].real, 4);
        Serial.print(F(" + "));
        Serial.print(statevectors[i].imag, 4);
        Serial.println(F("i"));
      }
    }

  private:

    ComplexNumber* superposition(ComplexNumber x, ComplexNumber y) {
      static ComplexNumber superposResult[2];
      superposResult[0] = x * r2 + y * r2;
      superposResult[1] = x * r2 - y * r2;
      return superposResult;
    }

    ComplexNumber* rotate(ComplexNumber x, ComplexNumber y, float tt) {
      static ComplexNumber rotResult[2];
      float cos_tt = cos(tt / 2.0f);
      float sin_tt = sin(tt / 2.0f);
      rotResult[0] = ComplexNumber(x.real * cos_tt + y.imag * sin_tt, x.imag * cos_tt - y.real * sin_tt);
      rotResult[1] = ComplexNumber(y.real * cos_tt + x.imag * sin_tt, y.imag * cos_tt - x.real * sin_tt);
      return rotResult;
    }

    // Fix 10: use precomputed sin_tt instead of calling sin(-tt/2) again
    ComplexNumber* phaseturn(ComplexNumber x, ComplexNumber y, float tt) {
      static ComplexNumber phaseResult[2];
      float cos_tt = cos(tt / 2.0f);
      float sin_tt = sin(tt / 2.0f);
      phaseResult[0] = ComplexNumber(x.real * cos_tt + x.imag * sin_tt, x.imag * cos_tt - x.real * sin_tt);
      phaseResult[1] = ComplexNumber(y.real * cos_tt - y.imag * sin_tt, y.imag * cos_tt + y.real * sin_tt);
      return phaseResult;
    }

    int how_many_memory() {
      extern int __heap_start, *__brkval;
      int v;
      return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    }

    // Fix 7: threshold is caller-supplied required bytes; called before statevector allocation
    void system_check(int required) {
      int ram = how_many_memory();
      Serial.print(F("Free memory: "));
      Serial.println(ram);
      if (ram < required) {
        Serial.println(F(":::::::Warning:::::::"));
        exit(1);
      }
    }
};

#endif
