#ifndef MICROMOTHARDUINO_H
#define MICROMOTHARDUINO_H

#include <math.h>
#include <stdlib.h>
#include <Arduino.h>


// Define constants for error handling
#define ERROR(MESSAGE) error_handler(MESSAGE)

void error_handler(const String msg) {
  Serial.println(msg);
  while (true); // Halt execution indefinitely
}

class QuantumCircuit {
  public:
    int nQubits, nBits;
    String* data; // Replace vector<vector<string>> with dynamic array of strings
    int size;

    // Constructor
    QuantumCircuit() {
      data = nullptr;
      size = 0;
    }

    // Constructor with given qubits and bits
    QuantumCircuit(int n, int m = 0) {
      set_registers(n, m);
      data = nullptr;
      size = 0;
    }

    // Set registers and validate the qubit/bit relationship
    void set_registers(int n, int m = 0) {
      nQubits = n;
      nBits = m;
      if (!(nQubits == nBits || nBits == 0)) {
        ERROR("Only the cases nQubits = nBits and nBits = 0 are allowed in MicroMoth!");
      }
    }
};

#endif