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
QuantumCircuit qc(2); // 2 qubits

// double r2 = 1 / sqrt(2.0);
// (for .h) double r2 = 0.707106781; // modified for Arduino

void test() {
  // int init_state[] = {0};
  qc.initialise(new int[1]{0}, 1);

  qc.cx(0, 1);
  qc.h(0);
  qc.rx(HALF_PI, 1);
  
  // qc.measure_all();

  qc.simulate(qc, 1024, "statevector");

  // ComplexNumber* states = qc.statevectors;
  
  // qc.circuitPrint();
  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Serial.println(r2, 11); // In Arduino, r2 = 0.707106781, based on the result.
  // Serial.print() prints two decimals ONLY.
  // Serial.println(PI, 11); // PI is defined in arduino.h
  test();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.println(custom_random(0.00, 1.00), 11); // 8 for float, 11 for double
}
