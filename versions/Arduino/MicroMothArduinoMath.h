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

#ifndef MICROMOTHARDUINOMATH_H
#define MICROMOTHARDUINOMATH_H

#include <Arduino.h>

static constexpr float r2 = 0.707106781f; // 1/sqrt(2), constexpr avoids ODR violation when included in multiple TUs

float custom_random(double minFloat, double maxFloat)
{
  // 1UL<<31 overflows signed long on AVR (wraps to -2147483648); Arduino random() returns 0
  // for non-positive arguments, collapsing the entire range to 0.  Use a bound that fits
  // safely in a positive signed long; result is in [0, 1) so CDF sampling is correct.
  return (float)(minFloat + (float)random(1000000L) * (maxFloat - minFloat) / 1000000.0f);
}

struct ComplexNumber {
  float real;
  float imag;

  // A constructor
  ComplexNumber(float r = 0.0, float i = 0.0) : real(r), imag(i) { }

  // Addition of two Complex numbers
  ComplexNumber operator+(const ComplexNumber& other) const {
    return ComplexNumber(real + other.real, imag + other.imag);
  }

  // Subtration of two Complex numbers
  ComplexNumber operator-(const ComplexNumber& other) const {
    return ComplexNumber(real - other.real, imag - other.imag);
  }

  // Multiplicatin of two Complex numbers
  ComplexNumber operator*(const ComplexNumber& other) const {
    return ComplexNumber(real * other.real - imag * other.imag, real * other.imag + imag * other.real);
  }

  // Magnitude of the Complex number
  float magnitude() const {
    return sqrt(real * real + imag * imag);
  }

  // Conjugate of the Complex number
  ComplexNumber conjugate() const {
    return ComplexNumber(real, -imag);
  }
};

#endif