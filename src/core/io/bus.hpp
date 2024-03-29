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
  static inline void config_input() { PIN::config_input(); }
  static inline void enable() { PIN::clear(); }
  static inline void disable() { PIN::set(); }
  static inline bool is_enabled() { return PIN::is_clear(); }
};

template <typename PIN>
struct ActiveHigh {
  static inline void config_output() {
    disable();
    PIN::config_output();
  }
  static inline void config_input() { PIN::config_input(); }
  static inline void enable() { PIN::set(); }
  static inline void disable() { PIN::clear(); }
  static inline bool is_enabled() { return PIN::is_set(); }
};

struct LogicNull {
  static inline void config_output() {}
  static inline void config_input() {}
  static inline void enable() {}
  static inline void disable() {}
  static inline bool is_enabled() { return false; }
};

template <typename DATA, typename LATCH_ENABLE, typename OUTPUT_ENABLE = LogicNull>
struct Latch {
  using TYPE = typename DATA::TYPE;
  static inline void config_output() {
    OUTPUT_ENABLE::config_output();
    OUTPUT_ENABLE::enable();
    DATA::config_output();
    LATCH_ENABLE::config_output();
  }
  static inline void config_input() {
    OUTPUT_ENABLE::config_output();
    OUTPUT_ENABLE::disable();
  }
  static inline void write(TYPE data) {
    DATA::write(data);
    LATCH_ENABLE::enable();
    LATCH_ENABLE::disable();
  }
};

// Derived types should define the following:
// typename DATA_TYPE
// typename ADDRESS_TYPE
// static void write_bus(ADDRESS_TYPE addr, DATA_TYPE data)
// static DATA_TYPE read_bus(ADDRESS_TYPE addr)
// static void config_write()
// static void config_read()
// static void config_float()
struct BaseBus {
  // Derived types can override to flush delayed writes (e.g. EEPROM page mode)
  static void flush_write() {}
};

// Parallel computer bus for interfacing with external devices
// See read_bus comments on overriding if necessary
template <typename ADDRESS, typename DATA, typename RE, typename WE>
struct PortBus : BaseBus {
  using ADDRESS_TYPE = typename ADDRESS::TYPE;
  using DATA_TYPE = typename DATA::TYPE;

  static void config_write() {
    ADDRESS::config_output();
    DATA::config_output();
    RE::config_output();
    WE::config_output();
  }

  static void config_read() {
    ADDRESS::config_output();
    DATA::config_input();
    RE::config_output();
    WE::config_output();
  }

  static void config_float() {
    ADDRESS::config_input();
    DATA::config_input();
    RE::config_input();
    WE::config_input();
  }

  static void write_bus(ADDRESS_TYPE addr, DATA_TYPE data) {
    ADDRESS::write(addr);
    WE::enable();
    DATA::write(data);
    WE::disable();
  }

  // This function can be overridden if:
  // - ADDRESS is latched from DATA; use config_output/input as commented
  // - tOE longer than 70ns results in corrupted read value
  static DATA_TYPE read_bus(ADDRESS_TYPE addr) {
    //DATA::config_output();
    ADDRESS::write(addr);
    //DATA::config_input();
    RE::enable();
    // Need at least one cycle delay for AVR read latency
    // Second delay cycle has been sufficient for SRAM/EEPROM with tOE up to 70ns
    util::nop<2>(); // insert two NOP delay cycles
    const DATA_TYPE data = DATA::read();
    RE::disable();
    return data;
  }
};

// Use CORE_ARRAY_BUS(array, address_t) to generate template parameters
template <typename DATA, typename ADDRESS, ADDRESS SIZE, DATA (&ARRAY)[SIZE]>
struct ArrayBus : BaseBus {
  using DATA_TYPE = DATA;
  using ADDRESS_TYPE = ADDRESS;
  static void config_write() {}
  static void config_read() {}
  static void config_float() {}
  static DATA_TYPE read_bus(ADDRESS_TYPE addr) { return ARRAY[addr % SIZE]; }
  static void write_bus(ADDRESS_TYPE addr, DATA_TYPE data) { ARRAY[addr % SIZE] = data; }
};

} // namespace io
} // namespace core

// Create a Bus-like read/write interface around an array with the given address type
#define CORE_ARRAY_BUS(ARRAY, ADDRESS) core::io::ArrayBus<core::util::remove_reference<decltype(ARRAY[0])>::type, ADDRESS, core::util::array_length(ARRAY), ARRAY>
