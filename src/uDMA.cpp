// Copyright (c) 2021 Trevor Makes

#include "uDMA.hpp"
#include "uIO.hpp"

#include <Arduino.h>
#include <stdint.h>

namespace uDMA {

#if defined(ARDUINO_AVR_MICRO)

// PORTC [* C - - - - - -]
using Clock = uIO::PortC::Bit<6>;

// PORTD [CS WE - R A3 A2 A1 A0]
using Reset = uIO::PortD::Bit<4>;

// PORTE [- H - - - * - -]
using Halt = uIO::PortE::Bit<6>;

inline void configure_clock() {
  Clock::config_output(); // DDRC |= bit(6); //< set PC6 (OC3A) as output
  TCCR3B = bit(CS30) | bit(WGM32); //< no prescaling, CTC
  OCR3A = 1; // 4 MHz
}

inline void configure_halt() {
  // Set halt to active low input with pullup
  Halt::config_input_pullups(); // DDRE &= ~HALT_MASK; PORTE |= HALT_MASK;
}

void setup() {
  force_reset(true);
  configure_clock();
  configure_halt();
}

void force_reset(bool enable) {
  Reset::config_output(); // DDRD |= RESET_MASK;
  if (enable) {
    Reset::clear(); // PORTD &= ~RESET_MASK;
  } else {
    Reset::set(); // PORTD |= RESET_MASK;
  }
}

void enable_clock(bool enable) {
  if (enable) {
    TCCR3A = bit(COM3A0); //< toggle OC3A on compare
  } else {
    TCCR3A = bit(COM3A1); //< clear OC3A on compare (hold clock pin low)
  }
}

bool is_halted() {
  return Halt::is_clear(); // !(PINE & HALT_MASK);
}

#endif

} // namespace uDMA
