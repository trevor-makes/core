// Copyright (c) 2021 Trevor Makes

#pragma once

#ifdef ENV_NATIVE
#include "FakeStream.hpp"
#else
#include <Arduino.h>
#endif

#include <stdint.h>

// Arduino pollutes all namespaces by defining DEFAULT in the preprocessor
#ifdef DEFAULT
constexpr uint8_t DEFAULT_BUT_DONT_POLLUTE = DEFAULT;
#undef DEFAULT
// Redefine DEFAULT as a constant in the default namespace only
constexpr uint8_t DEFAULT = DEFAULT_BUT_DONT_POLLUTE;
#endif

namespace core {
namespace serial {

// Styles supported by `set_style`
enum class Style : uint8_t {
  DEFAULT   = 0,
  BOLD      = 1,
  FAINT     = 2,
  ITALIC    = 3,
  UNDERLINE = 4,
  BLINK     = 5,
  INVERSE   = 7,
};

// Colors supported by `set_foreground` and `set_background`
enum class Color : uint8_t {
  BLACK   = 0,
  RED     = 1,
  GREEN   = 2,
  YELLOW  = 3,
  BLUE    = 4,
  MAGENTA = 5,
  CYAN    = 6,
  WHITE   = 7,
  DEFAULT = 9,
};

class StreamEx : public Stream {
  Stream& stream_;
  int peek_ = -1;
  enum class State : uint8_t {
    RESET,
    ESCAPE, // preceding input was "\e"
    CSI, // preceding input was "\e["
    EMIT_CSI, // spit out unhandled CSI
    CR, // preceding input was "\r"
  } state_ = State::RESET;

public:
  // Extended key codes returned by `read`
  static constexpr int KEY_UP    = 0x100;
  static constexpr int KEY_DOWN  = 0x101;
  static constexpr int KEY_RIGHT = 0x102;
  static constexpr int KEY_LEFT  = 0x103;
  static constexpr int KEY_END   = 0x104;
  static constexpr int KEY_HOME  = 0x105;

  StreamEx(Stream& stream): stream_{stream} {}

  // Make type non-copyable
  StreamEx(const StreamEx&) = delete;
  StreamEx& operator=(const StreamEx&) = delete;

  // Virtual methods from Stream
  int peek(void) override;
  int read(void) override;
  int available(void) override { return peek() != -1; }

  // Virtual methods from Print
  int availableForWrite(void) override { return stream_.availableForWrite(); }
  void flush(void) override { stream_.flush(); }
  size_t write(uint8_t c) override { return stream_.write(c); }

  // Expose non-virtual methods from Print, as done by HardwareSerial
  using Print::write;

  void save_cursor();
  void restore_cursor();

  void get_cursor(uint8_t& row, uint8_t& col); //< Get the current cursor position
  void get_size(uint8_t& row, uint8_t& col); //< Get the bottom-right-most position

  // Move the cursor to (`row`, `col`)
  void set_cursor(uint8_t row, uint8_t col);

  void cursor_up(uint8_t spaces = 1); //< Move the cursor up, optionally by multiple `spaces`
  void cursor_down(uint8_t spaces = 1); //< Move the cursor down, optionally by multiple `spaces`
  void cursor_right(uint8_t spaces = 1); //< Move the cursor right, optionally by multiple `spaces`
  void cursor_left(uint8_t spaces = 1); //< Move the cursor left, optionally by multiple `spaces`

  void hide_cursor(); //< Hide the cursor
  void show_cursor(); //< Show the cursor

  // Erase all text and formatting
  void clear_screen();

  void insert_char(uint8_t count = 1); //< Insert at cursor, shifting the rest of the line right
  void delete_char(uint8_t count = 1); //< Delete at cursor, shifting the rest of the line left
  void erase_char(uint8_t count = 1); //< Erase at cursor without shifting the rest of the line

  // Set the text style
  void set_style(Style style);

  // Set the text color
  void set_foreground(Color color) {
    set_style(Style(30 + uint8_t(color)));
  }

  // Set the background color
  void set_background(Color color) {
    set_style(Style(40 + uint8_t(color)));
  }
};

} // namespace serial
} // namespace core
