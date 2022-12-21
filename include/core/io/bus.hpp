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
  static inline void config_float() { PIN::config_input(); }
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
  static inline void config_float() { PIN::config_input(); }
  static inline void enable() { PIN::set(); }
  static inline void disable() { PIN::clear(); }
  static inline bool is_enabled() { return PIN::is_set(); }
};

struct LogicNull {
  static inline void config_output() {}
  static inline void config_input() {}
  static inline void config_float() {}
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
  static inline void config_float() {
    OUTPUT_ENABLE::config_output();
    OUTPUT_ENABLE::disable();
  }
  static inline void write(TYPE data) {
    DATA::write(data);
    LATCH_ENABLE::enable();
    LATCH_ENABLE::disable();
  }
};

// Derived type should define
// static void write_bus(ADDRESS_TYPE addr, DATA_TYPE data)
// static DATA_TYPE read_bus(ADDRESS_TYPE addr)
// static void config_float() [if used]
struct BaseBus {
  static void config_write() {}
  static void config_read() {}
  static void flush_write() {}
};

// Use CORE_ARRAY_BUS(array, address_t) to generate template parameters
template <typename DATA, typename ADDRESS, ADDRESS SIZE, DATA (&ARRAY)[SIZE]>
struct ArrayBus : BaseBus {
  using DATA_TYPE = DATA;
  using ADDRESS_TYPE = ADDRESS;
  static DATA_TYPE read_bus(ADDRESS_TYPE addr) { return ARRAY[addr % SIZE]; }
  static void write_bus(ADDRESS_TYPE addr, DATA_TYPE data) { ARRAY[addr % SIZE] = data; }
};

} // namespace io
} // namespace core

// Create a Bus-like read/write interface around an array with the given address type
#define CORE_ARRAY_BUS(ARRAY, ADDRESS) core::io::ArrayBus<core::util::remove_reference<decltype(ARRAY[0])>::type, ADDRESS, core::util::array_length(ARRAY), ARRAY>
