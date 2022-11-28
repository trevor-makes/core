// Copyright (c) 2021 Trevor Makes

#pragma once

#include <stdint.h>

namespace core {
namespace io {

// Typical SRAM interface with active-low control lines
template <typename OUTPUT_ENABLE, typename WRITE_ENABLE>
struct Control {
  // Configure ports to drive control lines
  static inline void config_active() {
    OUTPUT_ENABLE::set();
    WRITE_ENABLE::set();
    OUTPUT_ENABLE::config_output();
    WRITE_ENABLE::config_output();
  }

  // Configure ports to float for external control
  static inline void config_float() {
    OUTPUT_ENABLE::config_input_pullups();
    WRITE_ENABLE::config_input_pullups();
  }

  // Set control lines for start of write sequence
  static inline void begin_write() {
    WRITE_ENABLE::clear();
  }

  // Set control lines for end of write sequence
  static inline void end_write() {
    WRITE_ENABLE::set();
  }

  // Set control lines for start of read sequence
  static inline void begin_read() {
    OUTPUT_ENABLE::clear();
  }

  // Set control lines for end of read sequence
  static inline void end_read() {
    OUTPUT_ENABLE::set();
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

  // TODO rename *_byte to *_data or just read/write
  static void write_byte(ADDRESS_TYPE addr, DATA_TYPE data) {
    ADDRESS::write(addr);
    CONTROL::begin_write();
    DATA::write(data);
    CONTROL::end_write();
  }

  static DATA_TYPE read_byte(ADDRESS_TYPE addr) {
    ADDRESS::write(addr);
    CONTROL::begin_read();
    const DATA_TYPE data = DATA::read();
    CONTROL::end_read();
    return data;
  }
};

} // namespace io
} // namespace core
