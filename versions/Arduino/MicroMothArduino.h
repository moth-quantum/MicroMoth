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
    enum GateOp { INIT, X, RX, RZ, H, CX, CRX, SWAP, RY, Z, T, Y, M };

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

    // A constructor with the initial properties
    QuantumCircuit(int n, int m = 0) : num_qubits(n), num_clbits(m), size(0), capacity(10) {
      name = "";
      data = new Op[capacity]; // Allocate initial memory space
    }

    // A deconstructor
    ~QuantumCircuit() {
      delete[] data;
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
      data[size++] = Op(X, 0.0, q);
    }

    // Hadamard gate
    void h(int q) {
      if (size >= capacity) resize();
      data[size++] = Op(H, 0.0, q);
    }

    // CX gate (Control-X gate)
    void cx(int s, int t) {
      if (size >= capacity) resize();
      data[size++] = Op(CX, 0.0, s, t); // control qubit + target qubit
    }

    // Rotation-X gate
    void rx(float theta, int q) {
      if (size >= capacity) resize();
      data[size++] = Op(RX, theta, q);
    }

    // Rotation-Y gate
    void ry(float theta, int q) {
      rx(HALF_PI, q);
      rz(theta, q);
      rx((-PI) / 2.0, q);
    }

    // Rotation-Z gate
    void rz(float theta, int q) {
      if (size >= capacity) resize();
      data[size++] = Op(RZ, theta, q);
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
};

#endif