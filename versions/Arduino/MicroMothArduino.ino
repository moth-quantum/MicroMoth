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

// MicroMoth example: a Bell state printed to the Serial Monitor, then a quantum coin on the built-in LED.
// Works on any board with Serial, for example Uno, Mega, Nano, ESP32 or Raspberry Pi Pico.

#include "MicroMothArduino.h"

QuantumCircuit coin(1, 1);  // One qubit, one classical bit.

void setup() {
  Serial.begin(115200);
  micromoth_seed(analogRead(A0));  // An unconnected analog pin gives a different seed on each power-up.

  // A Bell state: two qubits that always agree when measured.
  QuantumCircuit bell(2, 2);
  bell.h(0).cx(0, 1).measure_all();

  // The quick way: simulate and print, like the Python version. Prints lines such as "00: 509".
  simulate(bell, 1024);

  // The programmable way: the results land in your own array, one slot per outcome.
  uint16_t counts[4];
  if (simulate_counts(bell, 1024, counts)) {
    Serial.print(F("Shots where both qubits agreed: "));
    Serial.println(counts[0] + counts[3]);
  } else {
    Serial.println(micromoth_error());
  }

  // The quantum coin: h puts the qubit into an equal superposition of 0 and 1.
  coin.h(0).measure_all();
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
#endif
}

void loop() {
  uint16_t outcome;
  if (simulate_memory(coin, 1, &outcome)) {  // One shot.
#ifdef LED_BUILTIN
    digitalWrite(LED_BUILTIN, outcome ? HIGH : LOW);
#endif
    Serial.println(outcome ? F("coin: 1") : F("coin: 0"));
  }
  delay(500);
}
