// Copyright (c) 2023 Trevor Makes

#pragma once

#include <stdint.h>

namespace core {
namespace io {

// TODO need R_PFS->PORT[n]
template <uint32_t BASE, uint16_t MASK = uint16_t(~0),
  bool B = (MASK == uint16_t(~0))>
struct Port;

// Unmasked port optimizations
template <uint32_t BASE, uint16_t M>
struct Port<BASE, M, true> {
  using TYPE = uint16_t;
  static const TYPE MASK = M;
  static_assert(MASK == uint16_t(~0),
    "Unmasked port should have all MASK bits set");

  // Select bit within I/O port
  template <uint8_t BIT>
  using Bit = Port<BASE, 1 << BIT>;

  // Select masked region within I/O port
  template <uint8_t SUBMASK>
  using Mask = Port<BASE, SUBMASK>;

  // Configure port as output
  static inline void config_output() {
    ((R_PORT0_Type*)BASE)->PDR = MASK;
  }

  // Configure port as input
  static inline void config_input() {
    ((R_PORT0_Type*)BASE)->PDR = 0; //< select read mode
    // TODO need to use R_PFS to disable pullups
  }

  // Configure port as input with pullup registers
  static inline void config_input_pullups() {
    // TODO need to use R_PFS to disable pullups
    ((R_PORT0_Type*)BASE)->PDR = 0; //< select read mode
  }

  static inline void set() { bitwise_or(MASK); }
  static inline void clear() { bitwise_and(0); }
  static inline void write(uint16_t value) { ((R_PORT0_Type*)BASE)->PODR = value; }
  static inline void bitwise_or(uint16_t value) { ((R_PORT0_Type*)BASE)->POSR = value; }
  static inline void bitwise_and(uint16_t value) { ((R_PORT0_Type*)BASE)->PORR = ~value; }

  static inline uint16_t read() { return ((R_PORT0_Type*)BASE)->PIDR; }
  static inline bool is_clear() { return ((R_PORT0_Type*)BASE)->PIDR == 0; }
  static inline bool is_set() { return ((R_PORT0_Type*)BASE)->PIDR == MASK; }
};

// Masked port
template <uint32_t BASE, uint16_t M>
struct Port<BASE, M, false> {
  using TYPE = uint16_t;
  static const TYPE MASK = M;

  // Select bit within I/O port
  template <uint8_t BIT>
  using Bit = Port<BASE, 1 << BIT>;

  // Select masked region within I/O port
  template <uint8_t SUBMASK>
  using Mask = Port<BASE, SUBMASK>;

  // Configure port as output
  static inline void config_output() {
    ((R_PORT0_Type*)BASE)->PDR |= MASK;
  }

  // Configure port as input
  static inline void config_input() {
    ((R_PORT0_Type*)BASE)->PDR &= TYPE(~MASK); //< select read mode
    // TODO need to use R_PFS to disable pullups
  }

  // Configure port as input with pullup registers
  static inline void config_input_pullups() {
    // TODO need to use R_PFS to disable pullups
    ((R_PORT0_Type*)BASE)->PDR &= TYPE(~MASK); //< select read mode
  }

  static inline void set() { bitwise_or(MASK); }
  static inline void clear() { bitwise_and(0); }
  static inline void write(uint16_t value) { ((R_PORT0_Type*)BASE)->PODR = (((R_PORT0_Type*)BASE)->PODR & uint16_t(~MASK)) | (value & MASK); }
  static inline void bitwise_or(uint16_t value) { ((R_PORT0_Type*)BASE)->POSR = value & MASK; }
  static inline void bitwise_and(uint16_t value) { ((R_PORT0_Type*)BASE)->PORR = ~value & MASK; }

  static inline uint16_t read() { return ((R_PORT0_Type*)BASE)->PIDR & MASK; }
  static inline bool is_clear() { return (((R_PORT0_Type*)BASE)->PIDR & MASK) == 0; }
  static inline bool is_set() { return (((R_PORT0_Type*)BASE)->PIDR & MASK) == MASK; }
};

} // namespace io
} // namespace core
