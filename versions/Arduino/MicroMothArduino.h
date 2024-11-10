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
    // Gate-based quantum computing
    enum GateOp { INIT, X, RX, RZ, H, CX, CRX, CRZ, SWAP, RY, Z, T, Y, M };
    // If it passed onto it starts from the index 0. So for initialisation, printCircuit will produce 0.

    struct Op {
      GateOp gate; // which gate?
      float angle; // for phase rotation
      int control; // control qubit for cx gate, etc.
      int target; // which qubit should I put this gate operation?

      Op(GateOp g = INIT, float a = 0.0, int q1 = 0, int q2 = 0) : gate(g), angle(a), control(q1), target(q2) {
        // a constructor
      }
    };

    int num_qubits; // the number of qubits
    int num_clbits; // the number of classical bits
    
    Op* data; // the actual circuit data
    int size; // the size of the operation
    int capacity; // the initialised size of data collection -> will be updated

    String name;

    ComplexNumber* statevectors;

    // A constructor with the initial properties
    QuantumCircuit(int n, int m = 0) : num_qubits(n), num_clbits(m), size(0), capacity(10) {
      name = "";
      data = new Op[capacity]; // Allocate initial memory space
      statevectors = nullptr;
    }

    // A deconstructor
    ~QuantumCircuit() {
      delete[] data;
      delete[] statevectors; // reset
    }

    // Because Arduino doesn't support std::vector, this process is necessary.
    void resize() {
      capacity += 2;
      Op* temp = new Op[capacity];
      for (int i = 0; i < size; i++) {
        temp[i] = data[i];
      }
      delete[] data;
      data = temp;
    }

    // State initialisation
    void initialise(const int* k, int s) {
      size = 0; // Clears all previous modification

      data[size].gate = INIT;
      data[size].angle = 0.0;

      for (int i = 0; i < s; i++) {
        data[size].control = 0;
        data[size].target = k[i];
      }

      size++;
    }

    // Pauli-X gate
    void x(int q) {
      if (size >= capacity) resize();
      data[size++] = Op(X, 0.0, 0, q);
    }

    // Hadamard gate
    void h(int q) {
      if (size >= capacity) resize();
      data[size++] = Op(H, 0.0, 0, q);
    }

    // CX gate (Control-X gate)
    void cx(int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CX, 0.0, s, t); // control qubit + target qubit
    }

    // Rotation-X gate
    void rx(float theta, int q) {
      if (size >= capacity) resize();
      data[size++] = Op(RX, theta, 0, q);
    }

    // Rotation-Y gate
    void ry(float theta, int q) {
      rx(HALF_PI, q);
      rz(theta, q);
      rx(-(HALF_PI), q);
    }

    // Rotation-Z gate
    void rz(float theta, int q) {
      if (size >= capacity) resize();
      data[size++] = Op(RZ, theta, 0, q);
    }

    // Pauli-Y gate
    void y(int q) {
      rz(PI, q);
      x(q);
    }

    // Pauli-Z gate
    void z(int q) {
      rz(PI, q);
    }

    // T gate
    void t(int q) {
      rz(PI / 4.0, q);
    }

    // CRX gate (Control-RX gate)
    void crx(float theta, int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CRX, theta, s, t);
    }

    // CRZ gate (Control-RZ gate)
    void crz(float theta, int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CRZ, theta, s, t);
    }

    // SWAP gate
    void swap(int s, int t) { // Applies a swap to the given source and target qubits.
      if (size >= capacity) resize();
      data[size++] = Op(SWAP, 0.0, s, t);
    }

    // Storing the results to classical bits
    void measure(int q, int b) {
      if (b >= num_clbits) {
        Serial.println("Error: Index for output bit out of range.");
      }

      if (q >= num_qubits) {
        Serial.println("Error: Index for qubit out of range.");
      }

      if (size >= capacity) resize();
      data[size++] = Op(M, 0.0, q, b);
    }

    // Same operation above for all qubits
    void measure_all() {
      if (num_clbits == 0) {
        num_clbits = num_qubits;
      }
      for (int q = 0; q < num_qubits; q++) {
        measure(q, q);
      }
    }

    // Calculates the statevector for our quantum circuit
    void simulate(QuantumCircuit &qc, int shots = 1024, char* get = "counts", float* noiseModel = nullptr) {
      statevectors = new ComplexNumber[1 << qc.num_qubits];
      int ssize = 1 << qc.num_qubits;

      for (int i = 0; i < ssize; i++) {
        statevectors[i] = {0.0, 0.0};
      }
      statevectors[0] = {1.0, 0.0}; // Initialise: |0>

      // simulate noise model
      if (noiseModel) {
        for (int j = 0; j < qc.num_qubits; j++) {
          noiseModel[j] = noiseModel[0] * qc.num_qubits;
        }
      }

      // gate operations
      for (int i = 0; i < qc.size; i++) {
        Op g = qc.data[i];
        int j = g.target;

        /*
        if (g.gate == QuantumCircuit::INIT) {
          statevectors[0] = {1.0, 0.0}; // Initialise statevectors to given state (for now simplified)
        }
        */
        
        if (g.gate == QuantumCircuit::X) {
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

          int l = min(g.control, g.target);
          int h = max(g.control, g.target);

          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b01 = b00 + (1 << t);
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

          int l = min(g.control, g.target);
          int h = max(g.control, g.target);

          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b01 = b00 + (1 << t);
                int b10 = b00 + (1 << c);
                int b11 = b10 + (1 << t);

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

          int l = min(g.control, g.target);
          int h = max(g.control, g.target);

          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b01 = b00 + (1 << t);
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

          int l = min(g.control, g.target);
          int h = max(g.control, g.target);

          for (int i0 = 0; i0 < (1 << l); i0++) {
            for (int i1 = 0; i1 < (1 << (h - l - 1)); i1++) {
              for (int i2 = 0; i2 < (1 << (qc.num_qubits - h - 1)); i2++) {
                int b00 = i0 + (1 << (l + 1)) * i1 + (1 << (h + 1)) * i2;
                int b01 = b00 + (1 << t);
                int b10 = b00 + (1 << c);
                int b11 = b10 + (1 << t);
                
                ComplexNumber* r = qc.phaseturn(statevectors[b10], statevectors[b11], th);
                statevectors[b10] = r[0];
                statevectors[b11] = r[1];
              }
            }
          }
        }

        circuitPrint(g.gate);

        if (strcmp(get, "statevector") == 0) {
          // already stored statevectors as a member variable
        }

        system_check();
        
      }

      // delete[] statevectors; // reset
    }

    void circuitPrint(QuantumCircuit::GateOp tg) {
      Serial.println("+++Statevector+++");
      Serial.println(tg);
      for (int i = 0; i < (1 << num_qubits); i++) {
        Serial.print("Amplitude of state |");
        Serial.print(i, BIN);
        Serial.print(">: ");
        Serial.print(statevectors[i].real, 4);
        Serial.print(" + ");
        Serial.print(statevectors[i].imag, 4);
        Serial.println("i");
      }
    }

  private:

    ComplexNumber* superposition(ComplexNumber x, ComplexNumber y) {
      static ComplexNumber superposResult[2];
      superposResult[0] = x * r2 + y * r2; // (x + y) / sqrt(2)
      superposResult[1] = x * r2 - y * r2; // (x - y) / sqrt(2)

      return superposResult;
    }

    ComplexNumber* rotate(ComplexNumber x, ComplexNumber y, float tt) {
      // rotates a qubit by an angle tt(theta)
      static ComplexNumber rotResult[2];

      float cos_tt = cos(tt / 2.0);
      float sin_tt = sin(tt / 2.0);

      rotResult[0] = ComplexNumber(x.real * cos_tt + y.imag * sin_tt, x.imag * cos_tt - y.real * sin_tt);
      rotResult[1] = ComplexNumber(y.real * cos_tt + x.imag * sin_tt, y.imag * cos_tt - x.real * sin_tt);

      /*
      Serial.print("+++++++++++++++++++++++++++\n");
      Serial.print("===========================\n");
      Serial.print("+++++++++++++++++++++++++++\n");
      Serial.println(rotResult[0].real);
      Serial.println(rotResult[0].imag);
      Serial.println(rotResult[1].real);
      Serial.print(rotResult[1].imag);
      Serial.print("+++++++++++++++++++++++++++\n");
      Serial.print("===========================\n");
      Serial.print("+++++++++++++++++++++++++++\n");
      */

      return rotResult;
    }

    ComplexNumber* phaseturn(ComplexNumber x, ComplexNumber y, float tt) {
      // Phase shift rotation for a qubit by an angle tt(theta)
      static ComplexNumber phaseResult[2];
      float cos_tt = cos(tt / 2.0);
      float sin_tt = sin(tt / 2.0);

      phaseResult[0] = ComplexNumber(x.real * cos_tt - x.imag * sin(-tt / 2.0), x.imag * cos_tt + x.real * sin(-tt / 2.0));
      phaseResult[1] = ComplexNumber(y.real * cos_tt - y.imag * sin(tt / 2.0), y.imag * cos_tt + y.real * sin(tt / 2.0));

      /*
      Serial.print("+++++++++++++++++++++++++++\n");
      Serial.print("===========================\n");
      Serial.print("+++++++++++++++++++++++++++\n");
      Serial.println(phaseResult[0].real);
      Serial.println(phaseResult[0].imag);
      Serial.println(phaseResult[1].real);
      Serial.print(phaseResult[1].imag);
      Serial.print("+++++++++++++++++++++++++++\n");
      Serial.print("===========================\n");
      Serial.print("+++++++++++++++++++++++++++\n");
      */

      return phaseResult;
    }
    
    // Calculate the left memory space for Arduino Mega 2560
    int how_many_memory() {
      extern int __heap_start, *__brkval;
      int v;
      return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    }

    void system_check() {
      int ram = how_many_memory();
      Serial.print("Free memory: ");
      Serial.println(ram);

      if (ram < 100) {  // Threshold for minimum memory, adjust as needed
        Serial.println(":::::::Warning:::::::");
        exit(1); // Stop all execution before frying the board
      }
    }
};

#endif