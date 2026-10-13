// A minimal stand-in for the Arduino core, so MicroMothArduino.h can be compiled and tested on a computer.
// It provides only what the header uses: Print, Serial, F() and the C headers Arduino.h pulls in.

#ifndef MICROMOTH_HOST_ARDUINO_H
#define MICROMOTH_HOST_ARDUINO_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

class __FlashStringHelper;
#define F(literal) (reinterpret_cast<const __FlashStringHelper*>(literal))

// Collects everything printed, so tests can inspect it.
class Print {
 public:
  std::string text;

  size_t print(const __FlashStringHelper* s) { return print(reinterpret_cast<const char*>(s)); }
  size_t print(const char* s) { text += s ? s : "(null)"; return 1; }
  size_t print(char c) { text += c; return 1; }
  size_t print(int v) { return printf_like("%d", v); }
  size_t print(unsigned int v) { return printf_like("%u", v); }
  size_t print(long v) { return printf_like("%ld", v); }
  size_t print(unsigned long v) { return printf_like("%lu", v); }
  size_t print(double v, int digits = 2) {
    char buf[64];
    snprintf(buf, sizeof buf, "%.*f", digits, v);
    text += buf;
    return 1;
  }

  size_t println() { text += "\r\n"; return 1; }
  template <typename T> size_t println(T v) { print(v); return println(); }
  size_t println(double v, int digits) { print(v, digits); return println(); }

 private:
  template <typename T> size_t printf_like(const char* format, T v) {
    char buf[32];
    snprintf(buf, sizeof buf, format, v);
    text += buf;
    return 1;
  }
};

// Like the real cores, Serial is a Print with a few extras.
class HostSerial : public Print {
 public:
  void begin(unsigned long) {}
};

static HostSerial Serial;

// Just enough of the pin API to compile and run the example sketch.
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define A0 14
#define LED_BUILTIN 13
static int host_led = -1;
inline int analogRead(int) { return 512; }
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int value) { if (pin == LED_BUILTIN) host_led = value; }
inline void delay(unsigned long) {}

#endif
