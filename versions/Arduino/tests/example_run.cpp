// Compiles the example sketch on a computer and runs setup() and loop() once, so the example cannot rot.

#include "../MicroMothArduino.ino"

#include <stdio.h>

int main() {
  setup();
  loop();
  printf("%s", Serial.text.c_str());
  const bool printed_bell = Serial.text.find("00: ") != std::string::npos && Serial.text.find("Shots where both qubits agreed: 1024") != std::string::npos;
  const bool coin = Serial.text.find("coin: ") != std::string::npos && (host_led == HIGH || host_led == LOW);
  printf(printed_bell && coin ? "Example sketch ran.\n" : "Example sketch FAILED.\n");
  return printed_bell && coin ? 0 : 1;
}
