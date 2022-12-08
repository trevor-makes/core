// Copyright (c) 2021 Trevor Makes

#pragma once

#include "core/util.hpp"

#include <stdint.h>

namespace core {
namespace io {

template <typename PIN>
struct ActiveLow {
  static inline void config_active() {
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
  static inline void config_active() {
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
  static inline void config_active() {
    OUTPUT_ENABLE::config_active();
    WRITE_ENABLE::config_active();
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
    LATCH_ENABLE::config_active();
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

template <typename ADDRESS, typename DATA, typename CONTROL>
struct Bus {
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
    // Re-config direction each read in case ADDRESS is latched from DATA
    ADDRESS::config_output();
    ADDRESS::write(addr);
    DATA::config_input();
    CONTROL::begin_read();
    const DATA_TYPE data = DATA::read();
    CONTROL::end_read();
    return data;
  }

  // Derived type may use this to flush pending writes
  static void flush_write() {}
};

template <uint16_t SIZE, uint8_t (&ARRAY)[SIZE]>
struct ArrayBus {
  using DATA_TYPE = uint8_t;
  using ADDRESS_TYPE = uint16_t;
  static void config_read() {}
  static void config_write() {}
  static void config_float() {}
  static DATA_TYPE read_data(ADDRESS_TYPE addr) { return ARRAY[addr % SIZE]; }
  static void write_data(ADDRESS_TYPE addr, DATA_TYPE data) { ARRAY[addr % SIZE] = data; }
  static void flush_write() {}
};

} // namespace io
} // namespace core

#define CORE_ARRAY_BUS(ARRAY) core::io::ArrayBus<core::util::array_length(ARRAY), ARRAY>
