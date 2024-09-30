#include "MicroMothArduino.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  QuantumCircuit qc;
  qc.set_registers(2);

  Serial.println(qc.size);
}

void loop() {
  // put your main code here, to run repeatedly:

}
