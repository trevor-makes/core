// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#ifdef ENV_NATIVE
#include "FakePgm.hpp"
#else
#include <avr/pgmspace.h>
#endif

#include <stdint.h>
#include <stdlib.h>

namespace uMon {

// Parse unsigned value from string, returning true on success
// Prints the string and returns false if invalid characters found
// Supports prefixes $ for hex, & for octal, and % for binary
template <typename T>
bool parse_unsigned(T& result, const char* str) {
  char* end;
  // Handle custom prefixes for non-decimal bases
  switch (str[0]) {
  case '$': result = strtoul(++str, &end, 16); break; // hex
  case '&': result = strtoul(++str, &end, 8); break; // octal
  case '%': result = strtoul(++str, &end, 2); break; // binary
  default:  result = strtoul(str, &end, 10); break; // decimal
  }
  // Return true if all characters were parsed
  return end != str && *end == '\0';
}

template <typename API, uint8_t N, typename T>
bool input_hex(T& result) {
  // Read input into buffer
  char buf[N + 1];
  for (uint8_t i = 0; i < N; ++i) {
    buf[i] = API::input_char();
  }
  buf[N] = '\0';
  // Parse buffer as hex
  char* end;
  result = strtoul(buf, &end, 16);
  // Return true if all characters were parsed
  return end == &buf[N];
}

// Print single hex digit (or garbage if n > 15)
template <typename F>
void format_hex4(F&& print, uint8_t n) {
  print(n < 10 ? '0' + n : 'A' - 10 + n);
}

// Print 2 hex digits with leading zeroes
template <typename F>
void format_hex(F&& print, uint8_t n) {
  format_hex4(print, n >> 4);
  format_hex4(print, n & 0xF);
}

template <typename F>
void format_hex8(F&& print, uint8_t n) { format_hex(print, n); }

// Print 4 hex digits with leading zeroes
template <typename F>
void format_hex(F&& print, uint16_t n) {
  format_hex8(print, n >> 8);
  format_hex8(print, n & 0xFF);
}

template <typename F>
void format_hex16(F&& print, uint16_t n) { format_hex(print, n); }

// Print 8 hex digits with leading zeroes
template <typename F>
void format_hex(F&& print, uint32_t n) {
  format_hex16(print, n >> 16);
  format_hex16(print, n & 0xFFFF);
}

template <typename F>
void format_hex32(F&& print, uint32_t n) { format_hex(print, n); }

template <typename F>
void format_ascii(F&& print, uint8_t c) {
  if (c < ' ' || c >= 0x7F) {
    c = '.'; // Display control and non-ASCII as dot
  }
  print(c);
}

// Set CLI prompt to "[cmd] "
template <typename API, bool First = true>
void set_prompt(const char* cmd) {
  // NOTE compiler requires having `First` to match `set_prompt<API, false, Var...>`
  // when `Var...` is empty, even though `if (sizeof...(Var) > 0)` prevents this
  // from happening in practice
  if (First) {
    API::prompt_string(cmd);
    API::prompt_char(' ');
  }
}

// Set CLI prompt to "[cmd] $[arg] ..."
template <typename API, bool First = true, typename T, typename... Var>
void set_prompt(const char* cmd, const T arg, const Var... var)
{
  // Print command before first arg only
  if (First) {
    set_prompt<API>(cmd);
  }

  // Print current arg as 8-, 16-, or 32-bit hex
  API::prompt_char('$');
  format_hex(API::prompt_char, arg);
  API::prompt_char(' ');

  // Append following args recursively (with First set to false)
  if (sizeof...(Var) > 0) {
    set_prompt<API, false, Var...>(cmd, var...);
  }
}

template <typename API>
void print_pgm_string(const char* str) {
  for (;;) {
    char c = pgm_read_byte(str++);
    if (c == '\0') return;
    API::print_char(c);
  }
}

// Print entry from PROGMEM string table
template <typename API>
void print_pgm_table(const char* const table[], uint8_t index) {
  char* str = (char*)pgm_read_ptr(table + index);
  print_pgm_string<API>(str);
}

// Find index of string in PROGMEM table
template <uint8_t N>
uint8_t pgm_bsearch(const char* const (&table)[N], const char* str) {
  char* res = (char*)bsearch(str, table, N, sizeof(table[0]),
    [](const void* key, const void* entry) {
      return strcasecmp_P((char*)key, (char*)pgm_read_ptr((const char* const*)entry));
    });
  return res == nullptr ? N : (res - (char*)table) / sizeof(table[0]);
}

#define uMON_FMT_ERROR(API, IS_ERR, LABEL, STRING, FAIL) \
  if (IS_ERR) { \
    const char* str = STRING; \
    API::print_string(LABEL); \
    if (*str != '\0') { \
      API::print_string(": "); \
      API::print_string(str); \
    } \
    API::print_char('?'); \
    API::newline(); \
    FAIL; \
  }

#define uMON_EXPECT_ADDR(API, TYPE, NAME, ARGS, FAIL) \
  TYPE NAME; \
  { \
    const char* str = ARGS.next(); \
    if ( API::get_labels().get_addr(str, NAME)) {} \
    else if (parse_unsigned(NAME, str)) {} \
    else { uMON_FMT_ERROR(API, true, #NAME, str, FAIL) } \
  }

#define uMON_EXPECT_UINT(API, TYPE, NAME, ARGS, FAIL) \
  TYPE NAME; \
  { \
    const char* str = ARGS.next(); \
    const bool is_err = !parse_unsigned(NAME, str); \
    uMON_FMT_ERROR(API, is_err, #NAME, str, FAIL) \
  }

#define uMON_OPTION_UINT(API, TYPE, NAME, DEFAULT, ARGS, FAIL) \
  TYPE NAME = DEFAULT; \
  if (ARGS.has_next()) { \
    const char* str = ARGS.next(); \
    const bool is_err = !parse_unsigned(NAME, str); \
    uMON_FMT_ERROR(API, is_err, #NAME, str, FAIL); \
  }

#define uMON_INPUT_HEX8(API, NAME, FAIL) \
  uint8_t NAME; \
  if (!input_hex<API, 2>(NAME)) FAIL;

#define uMON_INPUT_HEX16(API, NAME, FAIL) \
  uint16_t NAME; \
  if (!input_hex<API, 4>(NAME)) FAIL;

} // namespace uMon
