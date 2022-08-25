// Copyright (c) 2021 Trevor Makes

#pragma once

#include "uIO.hpp"

#include <stdint.h>

namespace uDMA {

// Typical SRAM interface with active-low control lines
template <typename CHIP_SELECT, typename OUTPUT_ENABLE, typename WRITE_ENABLE>
struct Control {
  // Configure ports to drive control lines
  static inline void config_active() {
    CHIP_SELECT::set();
    OUTPUT_ENABLE::set();
    WRITE_ENABLE::set();
    CHIP_SELECT::config_output();
    OUTPUT_ENABLE::config_output();
    WRITE_ENABLE::config_output();
  }

  // Configure ports to float for external control
  static inline void config_float() {
    CHIP_SELECT::set();
    OUTPUT_ENABLE::set();
    WRITE_ENABLE::set();
    CHIP_SELECT::config_input_pullups();
    OUTPUT_ENABLE::config_input_pullups();
    WRITE_ENABLE::config_input_pullups();
  }

  // Set control lines for start of write sequence
  static inline void begin_write() {
    CHIP_SELECT::clear();
    WRITE_ENABLE::clear();
  }

  // Set control lines for end of write sequence
  static inline void end_write() {
    WRITE_ENABLE::set();
    CHIP_SELECT::set();
  }

  // Set control lines for start of read sequence
  static inline void begin_read() {
    CHIP_SELECT::clear();
    OUTPUT_ENABLE::clear();
  }

  // Set control lines for end of read sequence
  static inline void end_read() {
    OUTPUT_ENABLE::set();
    CHIP_SELECT::set();
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

  // TODO remove the string methods?
  static void write_string(ADDRESS_TYPE addr, const char* string) {
    for (;;) {
      const uint8_t data = *string++;
      write_byte(addr++, data);
      if (data == 0)
        break;
    }
  }

  static void read_string(ADDRESS_TYPE addr, char* string, uint8_t max_len) {
    for (uint8_t i = 0; i < max_len; ++i) {
      const uint8_t data = read_byte(addr + i);
      string[i] = data;
      if (data == 0)
        break;
    }
  }

  // Read bytes to buffer until null terminator or end of buffer
  template <uint8_t L>
  static void read_string(ADDRESS_TYPE addr, char (&buffer)[L]) {
    read_string(addr, buffer, L);
  }
};

// Perform initial configuration
void setup();

// Holds the Z80 in reset if true, otherwise resumes
void force_reset(bool enable);

// Resumes the clock output if true, otherwise pauses
void enable_clock(bool enable);

// Read the HALT signal from the Z80
bool is_halted();

} // namespace uDMA
