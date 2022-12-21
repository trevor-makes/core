// Copyright (c) 2021 Trevor Makes

#pragma once

#include "core/util.hpp"

#include <stdint.h>

namespace core {
namespace io {

template <typename PIN>
struct ActiveLow {
  static inline void config_output() {
    disable();
    PIN::config_output();
  }
  static inline void config_float() {
    PIN::config_input();
  }
  static inline void enable() {
    PIN::clear();
  }
  static inline void disable() {
    PIN::set();
  }
};

template <typename PIN>
struct ActiveHigh {
  static inline void config_output() {
    disable();
    PIN::config_output();
  }
  static inline void config_float() {
    PIN::config_input();
  }
  static inline void enable() {
    PIN::set();
  }
  static inline void disable() {
    PIN::clear();
  }
};

// Typical SRAM interface with active-low control lines
template <typename OUTPUT_ENABLE, typename WRITE_ENABLE>
struct Control {
  // Configure ports to drive control lines
  static inline void config_output() {
    OUTPUT_ENABLE::config_output();
    WRITE_ENABLE::config_output();
  }

  // Configure ports to float for external control
  static inline void config_float() {
    OUTPUT_ENABLE::config_float();
    WRITE_ENABLE::config_float();
  }

  // Set control lines for start of write sequence
  static inline void begin_write() {
    WRITE_ENABLE::enable();
  }

  // Set control lines for end of write sequence
  static inline void end_write() {
    WRITE_ENABLE::disable();
  }

  // Set control lines for start of read sequence
  static inline void begin_read() {
    OUTPUT_ENABLE::enable();
  }

  // Set control lines for end of read sequence
  static inline void end_read() {
    OUTPUT_ENABLE::disable();
  }
};

template <typename DATA, typename LATCH_ENABLE/*, typename OUTPUT_ENABLE*/>
struct Latch {
  using TYPE = typename DATA::TYPE;
  static inline void config_output() {
    // TODO enable OUTPUT_ENABLE
    DATA::config_output();
    LATCH_ENABLE::config_output();
  }
  static inline void config_input() {
    // TODO disable OUTPUT_ENABLE
    // NOTE don't need to config DATA
    LATCH_ENABLE::config_float();
  }
  static inline void write(TYPE data) {
    DATA::write(data);
    LATCH_ENABLE::enable();
    LATCH_ENABLE::disable();
  }
};

// Derived type should define
// static void write_data(ADDRESS_TYPE addr, DATA_TYPE data)
// static DATA_TYPE read_data(ADDRESS_TYPE addr)
// static void config_float() [if used]
struct BaseBus {
  static void config_write() {}
  static void config_read() {}
  static void flush_write() {}
};

template <typename ADDRESS, typename DATA, typename CONTROL>
struct PortBus : BaseBus {
  using ADDRESS_TYPE = typename ADDRESS::TYPE;
  using DATA_TYPE = typename DATA::TYPE;

  // Configure ports for writing to memory
  static inline void config_write() {
    ADDRESS::config_output();
    DATA::config_output();
    CONTROL::config_active();
  }

  // Configure ports for reading from memory
  static inline void config_read() {
    ADDRESS::config_output();
    DATA::config_input();
    CONTROL::config_active();
  }

  // Configure ports to float for external control
  static inline void config_float() {
    ADDRESS::config_input();
    DATA::config_input();
    CONTROL::config_float();
  }

  static void write_data(ADDRESS_TYPE addr, DATA_TYPE data) {
    ADDRESS::write(addr);
    CONTROL::begin_write();
    DATA::write(data);
    CONTROL::end_write();
  }

  static DATA_TYPE read_data(ADDRESS_TYPE addr) {
    // Re-config data direction each read in case ADDRESS is latched from DATA
    // TODO add template to determine if DATA and ADDRESS have any overlapping bits; only do DATA::config_out/in if true
    DATA::config_output();
    ADDRESS::write(addr);
    DATA::config_input();
    CONTROL::begin_read();
    const DATA_TYPE data = DATA::read();
    CONTROL::end_read();
    return data;
  }
};

// Use CORE_ARRAY_BUS(array, address_t) to generate template parameters
template <typename DATA, typename ADDRESS, ADDRESS SIZE, DATA (&ARRAY)[SIZE]>
struct ArrayBus : BaseBus {
  using DATA_TYPE = DATA;
  using ADDRESS_TYPE = ADDRESS;
  static DATA_TYPE read_data(ADDRESS_TYPE addr) { return ARRAY[addr % SIZE]; }
  static void write_data(ADDRESS_TYPE addr, DATA_TYPE data) { ARRAY[addr % SIZE] = data; }
};

} // namespace io
} // namespace core

// Create a Bus-like read/write interface around an array with the given address type
#define CORE_ARRAY_BUS(ARRAY, ADDRESS) core::io::ArrayBus<core::util::remove_reference<decltype(ARRAY[0])>::type, ADDRESS, core::util::array_length(ARRAY), ARRAY>
