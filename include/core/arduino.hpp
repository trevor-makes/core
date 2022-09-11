// Copyright (c) 2022 Trevor Makes

#pragma once

// TODO it would be nice to abort without an error if Arduino.h doesn't exist or if we're not using that framework
// Maybe there's a macro or environment variable we can check for that

// Try to preemptively exclude "binary.h"
#ifndef Binary_h
#define Binary_h
#endif

#include <Arduino.h>

// Arduino.h defines a number of preprocessor macros with commonly used symbol names.
// This is bad practice in C++ because macros pollute all namespaces (including enums)
// and cannot be shadowed like regular symbol names.

#define SANITIZE(NAME) TEMP_##NAME##_SANITIZED

// Macro can't expand to another macro, so I don't think this pattern can be reduced to one
#ifdef HIGH
constexpr uint8_t SANITIZE(HIGH) = HIGH;
#undef HIGH
constexpr uint8_t HIGH = SANITIZE(HIGH);
#endif

#ifdef LOW
constexpr uint8_t SANITIZE(LOW) = LOW;
#undef LOW
constexpr uint8_t LOW = SANITIZE(LOW);
#endif

#ifdef INPUT
constexpr uint8_t SANITIZE(INPUT) = INPUT;
#undef INPUT
constexpr uint8_t INPUT = SANITIZE(INPUT);
#endif

#ifdef OUTPUT
constexpr uint8_t SANITIZE(OUTPUT) = OUTPUT;
#undef OUTPUT
constexpr uint8_t OUTPUT = SANITIZE(OUTPUT);
#endif

#ifdef INPUT_PULLUP
constexpr uint8_t SANITIZE(INPUT_PULLUP) = INPUT_PULLUP;
#undef INPUT_PULLUP
constexpr uint8_t INPUT_PULLUP = SANITIZE(INPUT_PULLUP);
#endif

#ifdef SERIAL
constexpr uint8_t SANITIZE(SERIAL) = SERIAL;
#undef SERIAL
constexpr uint8_t SERIAL = SANITIZE(SERIAL);
#endif

#ifdef DISPLAY
constexpr uint8_t SANITIZE(DISPLAY) = DISPLAY;
#undef DISPLAY
constexpr uint8_t DISPLAY = SANITIZE(DISPLAY);
#endif

#ifdef CHANGE
constexpr uint8_t SANITIZE(CHANGE) = CHANGE;
#undef CHANGE
constexpr uint8_t CHANGE = SANITIZE(CHANGE);
#endif

#ifdef FALLING
constexpr uint8_t SANITIZE(FALLING) = FALLING;
#undef FALLING
constexpr uint8_t FALLING = SANITIZE(FALLING);
#endif

#ifdef RISING
constexpr uint8_t SANITIZE(RISING) = RISING;
#undef RISING
constexpr uint8_t RISING = SANITIZE(RISING);
#endif

#ifdef DEFAULT
constexpr uint8_t SANITIZE(DEFAULT) = DEFAULT;
#undef DEFAULT
constexpr uint8_t DEFAULT = SANITIZE(DEFAULT);
#endif

#ifdef EXTERNAL
constexpr uint8_t SANITIZE(EXTERNAL) = EXTERNAL;
#undef EXTERNAL
constexpr uint8_t EXTERNAL = SANITIZE(EXTERNAL);
#endif

#ifdef INTERNAL
constexpr uint8_t SANITIZE(INTERNAL) = INTERNAL;
#undef INTERNAL
constexpr uint8_t INTERNAL = SANITIZE(INTERNAL);
#endif

#ifdef min
#undef min
template <typename A, typename B>
constexpr auto min(A a, B b) -> decltype(a + b) {
  return a < b ? a : b;
}
#endif

#ifdef max
#undef max
template <typename A, typename B>
constexpr auto max(A a, B b) -> decltype(a + b) {
  return a > b ? a : b;
}
#endif

#ifdef abs
#undef abs
template <typename T>
constexpr T abs(T x) {
  return x >= 0 ? x : -x;
}
#endif

#ifdef bit
#undef bit
template <typename T>
constexpr auto bit(T b) -> decltype(1UL << b) {
  return 1UL << b;
}
#endif
