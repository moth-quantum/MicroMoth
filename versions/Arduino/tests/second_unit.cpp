// A second translation unit that includes the header, to prove it links without duplicate symbols.

#include "MicroMothArduino.h"

int second_unit_bell_count() {
  QuantumCircuit qc(2, 2);
  qc.h(0).cx(0, 1).measure_all();
  uint16_t counts[4];
  if (!simulate_counts(qc, 100, counts)) return -1;
  return counts[0] + counts[3];
}
