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

#include <stdint.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "MicroMothArduino.h"

QuantumCircuit qc(2); // 2 qubits, 0 classical bits (measure_all will set to 2)

// Bell state demo: h(0), cx(0,1), measure_all(), 64 shots
// Expected output: ~50% "00" and ~50% "11"
void test() {
  // Fix 9: use stack-allocated array instead of new int[1]{0}
  int init_state[1] = {0};
  qc.initialise(init_state, 1);

  qc.h(0);
  qc.cx(0, 1);
  qc.measure_all();

  qc.simulate(qc, 64, "counts");
}

void setup() {
  Serial.begin(115200);
  test();
}

void loop() {
  // put your main code here, to run repeatedly:
}
