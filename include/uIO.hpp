// Copyright (c) 2021 Trevor Makes

#pragma once

#include <avr/io.h>
#include <stdint.h>

#include "uIO/util.hpp"

namespace uIO {

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

// Virtual port that discards writes
template <typename T = uint8_t>
struct PortNull {
  using TYPE = T;
  static const TYPE MASK = 0;
  static inline void bitwise_xor(TYPE) {}
  static inline void bitwise_or(TYPE) {}
  static inline void bitwise_and(TYPE) {}
  static inline void write(TYPE) {}
  static inline void set() {}
  static inline void clear() {}
  static inline void flip() {}
  static inline TYPE read() { return 0; }
  static inline bool is_set() { return false; }
  static inline bool is_clear() { return true; }
  static inline void config_output() {}
  static inline void config_input() {}
  static inline void config_input_pullups() {}
};

template <typename PORT, uint8_t BITS>
struct RightShift : PORT {
  static_assert(BITS <= util::countr_zero(PORT::MASK), "RightShift would underflow masked bits");
  using TYPE = typename PORT::TYPE;
  static const TYPE MASK = PORT::MASK >> BITS;
  static inline void bitwise_xor(TYPE value) { PORT::bitwise_xor(value << BITS); }
  static inline void bitwise_or(TYPE value) { PORT::bitwise_or(value << BITS); }
  static inline void bitwise_and(TYPE value) { PORT::bitwise_and(value << BITS); }
  static inline void write(TYPE value) { PORT::write(value << BITS); }
  static inline TYPE read() { return PORT::read() >> BITS; }
};

template <typename PORT, uint8_t BITS>
struct LeftShift : PORT {
  static_assert(BITS <= util::countl_zero(PORT::MASK), "LeftShift would overflow masked bits");
  using TYPE = typename PORT::TYPE;
  static const TYPE MASK = PORT::MASK << BITS;
  static inline void bitwise_xor(TYPE value) { PORT::bitwise_xor(value >> BITS); }
  static inline void bitwise_or(TYPE value) { PORT::bitwise_or(value >> BITS); }
  static inline void bitwise_and(TYPE value) { PORT::bitwise_and(value >> BITS); }
  static inline void write(TYPE value) { PORT::write(value >> BITS); }
  static inline TYPE read() { return PORT::read() << BITS; }
};

template <typename PORT>
using RightAlign = RightShift<PORT, util::countr_zero(PORT::MASK)>;

template <typename Port1, typename Port2>
struct Overlay {
  static_assert(util::is_same<typename Port1::TYPE, typename Port2::TYPE>::value,
    "Overlain ports must have the same data type");
  static_assert((Port1::MASK & Port2::MASK) == 0,
    "Overlain ports must have non-overlapping masks");
  using TYPE = typename Port1::TYPE;
  static const TYPE MASK = Port1::MASK | Port2::MASK;

  // XOR value to both ports
  static inline void bitwise_xor(TYPE value) {
    Port1::bitwise_xor(value);
    Port2::bitwise_xor(value);
  }

  // OR value to both ports
  static inline void bitwise_or(TYPE value) {
    Port1::bitwise_or(value);
    Port2::bitwise_or(value);
  }

  // AND value to both ports
  static inline void bitwise_and(TYPE value) {
    Port1::bitwise_and(value);
    Port2::bitwise_and(value);
  }

  // Write value to both ports
  static inline void write(TYPE value) {
    Port1::write(value);
    Port2::write(value);
  }

  // Set bits in both ports
  static inline void set() {
    Port1::set();
    Port2::set();
  }

  // Clear bits in both ports
  static inline void clear() {
    Port1::clear();
    Port2::clear();
  }

  // Flip bits in both ports
  static inline void flip() {
    Port1::flip();
    Port2::flip();
  }

  // Read value from both ports
  static inline TYPE read() {
    return Port1::read() | Port2::read();
  }

  // Return true if both ports are set
  static inline bool is_set() {
    return Port1::is_set() && Port2::is_set();
  }

  // Return true if both ports are clear
  static inline bool is_clear() {
    return Port1::is_clear() && Port2::is_clear();
  }

  // Select write mode for both ports
  static inline void config_output() {
    Port1::config_output();
    Port2::config_output();
  }

  // Select read mode for both ports
  static inline void config_input() {
    Port1::config_input();
    Port2::config_input();
  }

  // Select read mode with pullups on both ports
  static inline void config_input_pullups() {
    Port1::config_input_pullups();
    Port2::config_input_pullups();
  }
};

template <typename ...>
struct WordExtend {};

// Double the word size of Port (upper half discards writes and reads as 0)
template <typename Port>
struct WordExtend<Port> : WordExtend<PortNull<typename Port::TYPE>, Port> {};

// Join three ports as one port with quadruple the word size
template <typename Port2, typename Port1, typename Port0>
struct WordExtend<Port2, Port1, Port0> :
  WordExtend<WordExtend<Port2>, WordExtend<Port1, Port0>> {};

// Join four ports as one port with quadruple the word size
template <typename Port3, typename Port2, typename Port1, typename Port0>
struct WordExtend<Port3, Port2, Port1, Port0> :
  WordExtend<WordExtend<Port3, Port2>, WordExtend<Port1, Port0>> {};

// Join two ports as one port with double the word size
template <typename PortMSB, typename PortLSB>
struct WordExtend<PortMSB, PortLSB> {
  static_assert(util::is_same<typename PortLSB::TYPE, typename PortMSB::TYPE>::value,
    "Can only extend pairs of same type");
  using TYPE = typename util::extend_unsigned<typename PortLSB::TYPE>::type;
  static constexpr uint8_t SHIFT = sizeof(typename PortLSB::TYPE) * 8;
  static const TYPE MASK = PortLSB::MASK | (TYPE(PortMSB::MASK) << SHIFT);

  // XOR extended value to high and low ports
  static inline void bitwise_xor(TYPE value) {
    PortMSB::bitwise_xor(value >> SHIFT);
    PortLSB::bitwise_xor(value);
  }

  // OR extended value to high and low ports
  static inline void bitwise_or(TYPE value) {
    PortMSB::bitwise_or(value >> SHIFT);
    PortLSB::bitwise_or(value);
  }

  // AND extended value to high and low ports
  static inline void bitwise_and(TYPE value) {
    PortMSB::bitwise_and(value >> SHIFT);
    PortLSB::bitwise_and(value);
  }

  // Write extended value to high and low ports
  static inline void write(TYPE value) {
    PortMSB::write(value >> SHIFT);
    PortLSB::write(value);
  }

  // Set bits in both ports
  static inline void set() {
    PortMSB::set();
    PortLSB::set();
  }

  // Clear bits in both ports
  static inline void clear() {
    PortMSB::clear();
    PortLSB::clear();
  }

  // Flip bits in both ports
  static inline void flip() {
    PortMSB::flip();
    PortLSB::flip();
  }

  // Read extended value from high and low ports
  static inline TYPE read() {
    return TYPE(PortLSB::read()) | (TYPE(PortMSB::read()) << SHIFT);
  }

  // Return true if both ports are set
  static inline bool is_set() {
    return PortLSB::is_set() && PortMSB::is_set();
  }

  // Return true if both ports are clear
  static inline bool is_clear() {
    return PortLSB::is_clear() && PortMSB::is_clear();
  }

  // Select write mode for both ports
  static inline void config_output() {
    PortMSB::config_output();
    PortLSB::config_output();
  }

  // Select read mode for both ports
  static inline void config_input() {
    PortMSB::config_input();
    PortLSB::config_input();
  }

  // Select read mode with pullups for both ports
  static inline void config_input_pullups() {
    PortMSB::config_input_pullups();
    PortLSB::config_input_pullups();
  }
};

// TODO use WordExtend when mask width exceeds type width
// TODO make variadic template 
template <typename PORT_MSB, typename PORT_LSB>
using BitExtend = Overlay<
  LeftShift<RightAlign<PORT_MSB>, util::mask_width(RightAlign<PORT_LSB>::MASK)>,
  RightAlign<PORT_LSB>>;

#define uIO_REG(REG) \
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
#define uIO_PORT(X) \
  uIO_REG(DDR##X) uIO_REG(PORT##X) uIO_REG(PIN##X) \
  using Port##X = Port<RegDDR##X, RegPORT##X, RegPIN##X>;

// TODO include other registers, or just I/O?
// TODO define masks for ports with less than 8 pins?
// TODO ^ macro to generate PinXN only for unmasked pins?
// ^^ maybe remove PinXN from port macro and just manually generate Arduino digital pin defs?
#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
uIO_PORT(B);
uIO_PORT(C);
uIO_PORT(D);
#elif defined(ARDUINO_AVR_MICRO)
uIO_PORT(B);
uIO_PORT(C);
uIO_PORT(D);
uIO_PORT(E);
uIO_PORT(F);
#endif

} // namespace uIO
