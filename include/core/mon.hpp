// Copyright (c) 2022 Trevor Makes

#pragma once

#include "mon/api.hpp"
#include "mon/format.hpp"
#include "core/cli.hpp"

#include <stdint.h>
#include <ctype.h>

namespace core {
namespace mon {

// Dump memory as hex/ascii from row to end, inclusive
template <typename API, uint8_t COL_SIZE = 16, uint8_t MAX_ROWS = 24>
uint16_t impl_hex(uint16_t row, uint16_t end) {
  API::BUS::config_read();

  uint8_t row_data[COL_SIZE];
  for (uint8_t i = 0; i < MAX_ROWS; ++i) {
    // Read row into temp buffer
    for (uint8_t j = 0; j < COL_SIZE; ++j) {
      row_data[j] = API::BUS::read_byte(row + j);
    }

    // Print address
    API::print_char(' ');
    format_hex16(API::print_char, row);

    // Print hex data
    for (uint8_t col = 0; col < COL_SIZE; ++col) {
      API::print_char(' ');
      if (col % 4 == 0) {
        API::print_char(' ');
      }
      format_hex8(API::print_char, row_data[col]);
    }

    // Print string data
    API::print_string("  \"");
    for (uint8_t col = 0; col < COL_SIZE; ++col) {
      format_ascii(API::print_char, row_data[col]);
    }
    API::print_char('\"');
    API::newline();

    // Do while end does not overlap with row
    uint16_t prev = row;
    row += COL_SIZE;
    if (uint16_t(end - prev) < COL_SIZE) { break; }
  }
  return row;
}

// Write pattern from start to end, inclusive
template <typename API>
void impl_memset(uint16_t start, uint16_t end, uint8_t pattern) {
  API::BUS::config_write();
  do {
    API::BUS::write_byte(start, pattern);
  } while (start++ != end);
  API::BUS::flush_write();
}

// Write string from start until null terminator
template <typename API>
uint16_t impl_strcpy(uint16_t start, const char* str) {
  API::BUS::config_write();
  for (;;) {
    char c = *str++;
    if (c == '\0') {
      return start;
    }
    API::BUS::write_byte(start++, c);
  }
  API::BUS::flush_write();
}

// Copy [start, end] to [dest, dest+end-start] (end inclusive)
template <typename API>
void impl_memmove(uint16_t start, uint16_t end, uint16_t dest) {
  uint16_t delta = end - start;
  uint16_t dest_end = dest + delta;
  // Buses narrower than 16-bits introduce cases with ghosting (wrap-around).
  // This logic should work as long as start and dest are both within [0, 2^N),
  // where N is the actual bus width.
  // See [notes/memmove.png]
  bool a = dest <= end;
  bool b = dest_end < start;
  bool c = dest > start;
  if ((a && b) || (a && c) || (b && c)) {
    // Reverse copy from end to start
    for (uint16_t i = 0; i <= delta; ++i) {
      API::BUS::config_read();
      auto data = API::BUS::read_byte(end - i);
      API::BUS::config_write();
      API::BUS::write_byte(dest_end - i, data);
    }
  } else {
    // Forward copy from start to end
    for (uint16_t i = 0; i <= delta; ++i) {
      API::BUS::config_read();
      auto data = API::BUS::read_byte(start + i);
      API::BUS::config_write();
      API::BUS::write_byte(dest + i, data);
    }
  }
  API::BUS::flush_write();
}

// Print memory range in IHX format
template <typename API, uint8_t REC_SIZE = 32>
void impl_export(uint16_t start, uint16_t size) {
  API::BUS::config_read();
  while (size > 0) {
    uint8_t rec_size = size > REC_SIZE ? REC_SIZE : size;
    size -= rec_size;
    // Print record header
    API::print_char(':');
    format_hex8(API::print_char, rec_size);
    format_hex16(API::print_char, start);
    format_hex8(API::print_char, 0);
    // Print data and checksum
    uint8_t checksum = rec_size + (start >> 8) + (start & 0xFF);
    while (rec_size-- > 0) {
      uint8_t data = API::BUS::read_byte(start++);
      format_hex8(API::print_char, data);
      checksum += data;
    }
    format_hex8(API::print_char, -checksum);
    API::newline();
  }
  // Print end-of-file record
  API::print_string(":00000001FF");
  API::newline();
}

// Read one line of IHX data plus checksum
template <typename API>
bool read_ihx_data(uint8_t rec_size, uint16_t address, uint8_t checksum) {
  // Read record data
  for (uint8_t i = 0; i < rec_size; ++i) {
    CORE_INPUT_HEX8(API, data, return false);
    API::BUS::write_byte(address + i, data);
    checksum += data;
  }
  // Validate checksum
  CORE_INPUT_HEX8(API, data, return false);
  return uint8_t(checksum + data) == 0;
}

// Read serial data from IHX format into memory
template <typename API>
bool impl_import() {
  API::BUS::config_write();
  bool success = false;
  for (;;) {
    // Discard whitespace while looking for start of record (:)
    char c;
    do { c = API::input_char(); } while (isspace(c));
    if (c != ':') break;
    // Parse record header and data
    CORE_INPUT_HEX8(API, rec_size, break);
    CORE_INPUT_HEX16(API, address, break);
    CORE_INPUT_HEX8(API, rec_type, break);
    uint8_t checksum = rec_size + (address >> 8) + (address & 0xFF) + rec_type;
    if (!read_ihx_data<API>(rec_size, address, checksum)) break;
    // Exit successfully if record type is not data (00)
    if (rec_type > 0) {
      success = true;
      break;
    }
  }
  API::BUS::flush_write();
  return success;
}

template <typename API>
void cmd_import(cli::Args) {
  if (!impl_import<API>()) {
    API::print_char('?');
  }
  API::newline();
}

template <typename API, uint8_t COL_SIZE = 16, uint8_t MAX_ROWS = 24>
void cmd_hex(cli::Args args) {
  // Default size to one row if not provided
  CORE_EXPECT_ADDR(API, uint16_t, start, args, return);
  CORE_OPTION_UINT(API, uint16_t, size, COL_SIZE, args, return);
  uint16_t end_incl = start + size - 1;
  uint16_t next = impl_hex<API, COL_SIZE, MAX_ROWS>(start, end_incl);
  uint16_t part = next - start;
  if (part < size) {
    set_prompt<API>(args.command(), next, size - part);
  }
}

template <typename API>
void cmd_set(cli::Args args) {
  CORE_EXPECT_ADDR(API, uint16_t, start, args, return);
  do {
    if (args.is_string()) {
      start = impl_strcpy<API>(start, args.next());
    } else {
      CORE_EXPECT_UINT(API, uint8_t, data, args, return);
      impl_memset<API>(start, start, data);
      ++start;
    }
  } while (args.has_next());
  set_prompt<API>(args.command(), start);
}

template <typename API>
void cmd_fill(cli::Args args) {
  CORE_EXPECT_ADDR(API, uint16_t, start, args, return);
  CORE_EXPECT_UINT(API, uint16_t, size, args, return);
  CORE_EXPECT_UINT(API, uint8_t, pattern, args, return);
  impl_memset<API>(start, start + size - 1, pattern);
}

template <typename API>
void cmd_move(cli::Args args) {
  CORE_EXPECT_ADDR(API, uint16_t, start, args, return);
  CORE_EXPECT_UINT(API, uint16_t, size, args, return);
  CORE_EXPECT_ADDR(API, uint16_t, dest, args, return);
  impl_memmove<API>(start, start + size - 1, dest);
}

template <typename API, uint8_t REC_SIZE = 32>
void cmd_export(cli::Args args) {
  CORE_EXPECT_ADDR(API, uint16_t, start, args, return);
  CORE_EXPECT_UINT(API, uint16_t, size, args, return);
  impl_export<API, REC_SIZE>(start, size);
}

template <typename API>
void cmd_label(cli::Args args) {
  auto& labels = API::get_labels();
  if (args.has_next()) {
    const char* name = args.next();
    if (args.has_next()) {
      // Set label to integer argument
      CORE_EXPECT_UINT(API, uint16_t, addr, args, return);
      if (labels.set_label(name, addr) == false) {
        API::print_string("full");
        API::newline();
      }
    } else {
      // Remove label
      bool fail = !labels.remove_label(name);
      CORE_FMT_ERROR(API, fail, "name", name, return);
    }
  } else {
    // Print list of all labels
    uint16_t addr;
    const char* name;
    for (uint8_t i = 0; i < labels.entries(); ++i) {
      labels.get_index(i, name, addr);
      API::print_string(name);
      API::print_string(": $");
      format_hex16(API::print_char, addr);
      API::newline();
    }
  }
}

} // namespace mon
} // namespace core
