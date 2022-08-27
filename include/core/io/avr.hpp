// Copyright (c) 2022 Trevor Makes

#pragma once

// TODO this isn't strictly needed here; should user include it instead?
#include <avr/io.h>

#include "core/util.hpp"

#define CORE_REG(REG) \
  using TYPE_##REG = util::remove_volatile_reference<decltype((REG))>::type; \
  \
  template <TYPE_##REG MASK, \
    bool B = (MASK == TYPE_##REG(~0))> \
  struct RegMask##REG; \
  \
  using Reg##REG = RegMask##REG<TYPE_##REG(~0)>; \
  \
  template <TYPE_##REG M> \
  struct RegBase##REG { \
    using TYPE = TYPE_##REG; \
    static const TYPE MASK = M; \
    /* Select single bit within register */ \
    template <uint8_t BIT> \
    using Bit = RegMask##REG<1 << BIT>; \
    /* Select masked subfield within register */ \
    template <TYPE SUBMASK> \
    using Mask = RegMask##REG<SUBMASK>; \
  }; \
  \
  /* Unmasked register operations */ \
  template <TYPE_##REG MASK> \
  struct RegMask##REG<MASK, true> : RegBase##REG<MASK> { \
    static_assert(MASK == TYPE_##REG(~0), \
      "Unmasked register should have all MASK bits set"); \
    struct Input { \
      /* Read from I/O register; emits [IN] */ \
      static inline TYPE_##REG read() { return (REG); } \
      /* Test if bits are clear; `if (clear)` may emit [SBIS] */ \
      static inline bool is_clear() { return (REG) == 0; } \
      /* Test if bits are set; `if (set)` may emit [SBIC] */ \
      static inline bool is_set() { return (REG) == MASK; } \
    }; \
    static constexpr auto read = &Input::read; \
    static constexpr auto is_clear = &Input::is_clear; \
    static constexpr auto is_set = &Input::is_set; \
    struct Output { \
      /* Set bits; emits [(LDI) OUT] */ \
      static inline void set() { (REG) = MASK; } \
      /* Clear bits; emits [IN ANDI OUT] or [CBI] */ \
      static inline void clear() { (REG) = TYPE_##REG(~MASK); } \
      /* Write to I/O register; emits [(LDI) OUT] */ \
      static inline void write(TYPE_##REG value) { (REG) = value; } \
      /* Apply bitwise OR; emits [IN, OR, OUT] */ \
      static inline void bitwise_or(TYPE_##REG value) { (REG) |= value; } \
      /* Apply bitwise AND; emits [IN, AND, OUT] */ \
      static inline void bitwise_and(TYPE_##REG value) { (REG) &= value; } \
    }; \
    static constexpr auto set = &Output::set; \
    static constexpr auto clear = &Output::clear; \
    static constexpr auto write = &Output::write; \
    static constexpr auto bitwise_or = &Output::bitwise_or; \
    static constexpr auto bitwise_and = &Output::bitwise_and; \
  }; \
  \
  /* Masked register operations */ \
  template <TYPE_##REG MASK> \
  struct RegMask##REG<MASK, false> : RegBase##REG<MASK> { \
    static_assert(MASK != 0 && MASK != TYPE_##REG(~0), \
      "Masked register should have some but not all MASK bits set"); \
    struct Input { \
      /* Read from I/O register; emits IN, ANDI */ \
      static inline TYPE_##REG read() { return (REG) & MASK; } \
      /* Test if bits are clear; `if (clear)` may emit [SBIS] */ \
      static inline bool is_clear() { return ((REG) & MASK) == 0; } \
      /* Test if bits are set; `if (set)` may emit [SBIC] */ \
      static inline bool is_set() { return ((REG) & MASK) == MASK; } \
    }; \
    static constexpr auto read = &Input::read; \
    static constexpr auto is_clear = &Input::is_clear; \
    static constexpr auto is_set = &Input::is_set; \
    struct Output { \
      /* Set bits; emits [IN, ORI, OUT] or [SBI] */ \
      static inline void set() { (REG) |= MASK; } \
      /* Clear bits; emits [IN ANDI OUT] or [CBI] */ \
      static inline void clear() { (REG) &= TYPE_##REG(~MASK); } \
      /* Write to I/O register; emits IN, ANDI, (ANDI,) OR, OUT */ \
      static inline void write(TYPE_##REG value) { (REG) = ((REG) & TYPE_##REG(~MASK)) | (value & MASK); } \
      /* Apply bitwise OR; emits to IN, (ANDI,) OR, OUT */ \
      static inline void bitwise_or(TYPE_##REG value) { (REG) |= value & MASK; } \
      /* Apply bitwise AND; emits to IN, (ORI,) AND, OUT */ \
      static inline void bitwise_and(TYPE_##REG value) { (REG) &= value | TYPE_##REG(~MASK); } \
    }; \
    static constexpr auto set = &Output::set; \
    static constexpr auto clear = &Output::clear; \
    static constexpr auto write = &Output::write; \
    static constexpr auto bitwise_or = &Output::bitwise_or; \
    static constexpr auto bitwise_and = &Output::bitwise_and; \
  };

// Define Reg[DDR#X, PORT#X, PIN#X], Port#X
#define CORE_PORT(X) \
  CORE_REG(DDR##X) CORE_REG(PORT##X) CORE_REG(PIN##X) \
  using Port##X = core::io::Port<RegDDR##X, RegPORT##X, RegPIN##X>;

namespace core {
namespace io {

template <typename DDR, typename PORT, typename PIN>
struct Port : PORT::Output, PIN::Input {
  static_assert((DDR::MASK == PORT::MASK) && (PORT::MASK == PIN::MASK),
    "Parameters DDR, PORT, and PIN should have the same masks");
  using TYPE = typename PORT::TYPE;
  static const TYPE MASK = PORT::MASK;

  // Select bit within I/O port
  template <uint8_t BIT>
  using Bit = Port<
    typename DDR::template Bit<BIT>,
    typename PORT::template Bit<BIT>,
    typename PIN::template Bit<BIT>>;

  // Select masked region within I/O port
  template <uint8_t MASK>
  using Mask = Port<
    typename DDR::template Mask<MASK>,
    typename PORT::template Mask<MASK>,
    typename PIN::template Mask<MASK>>;

  // Invert output bits
  static inline void flip() {
    PIN::set(); //< Set bits in PIN to flip bits in PORT
  }

  // XOR output register with value
  static inline void bitwise_xor(TYPE value) {
    PIN::write(value); //< Set bits in PIN to flip (xor) bits in PORT
  }

  // Configure port as output
  static inline void config_output() {
    DDR::set(); //< Set bits in DDR to select write mode
  }

  // Configure port as input
  static inline void config_input() {
    PORT::clear(); //< Clear bits in PORT to disable pullups
    DDR::clear(); //< Clear bits in DDR to select read mode
  }

  // Configure port as input with pullup registers
  static inline void config_input_pullups() {
    PORT::set(); //< Set bits in PORT to enable pullups
    DDR::clear(); //< Clear bits in DDR to select read mode
  }
};

} // namespace io
} // namespace core
