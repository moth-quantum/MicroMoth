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

float r2 = 0.707106781; // modified for Arduino 

float custom_random(double minFloat, double maxFloat)
{
  // using bitshift left (<<) for the operation
  return minFloat + random(1UL << 31) * (maxFloat - minFloat) / (1UL << 31);  // half double max values. 63 for max values
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