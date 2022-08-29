// Copyright (c) 2021 Trevor Makes

#include "core/cli.hpp"
#include "core/serial.hpp"
#include "core/util.hpp"

namespace core {
namespace cli {

bool Cursor::try_left() {
  if (cursor_ > 0) {
    --cursor_;
    return true;
  } else {
    return false;
  }
}

bool Cursor::try_right() {
  if (cursor_ < length_) {
    ++cursor_;
    return true;
  } else {
    return false;
  }
}

uint8_t Cursor::seek_home() {
  uint8_t spaces = cursor_;
  cursor_ = 0;
  return spaces;
}

uint8_t Cursor::seek_end() {
  uint8_t spaces = length_ - cursor_;
  cursor_ = length_;
  return spaces;
}

uint8_t Cursor::try_insert(const char* input, uint8_t size) {
  // Limit size to space available in Cursor
  size = util::min(size, uint8_t(limit_ - length_));

  // Limit size to null terminator in input
  for (uint8_t i = 0; i < size; ++i) {
    if (input[i] == '\0') {
      size = i;
    }
  }

  if (size > 0) {
    // Move what follows the cursor and copy input into hole
    memmove(buffer_ + cursor_ + size, buffer_ + cursor_, length_ - cursor_);
    memcpy(buffer_ + cursor_, input, size);
    cursor_ += size;
    length_ += size;
    buffer_[length_] = '\0';
  }

  return size;
}

bool Cursor::try_insert(char input) {
  // Reject if buffer is full or input is null
  if (length_ >= limit_ || input == '\0') {
    return false;
  }

  // Shift following characters right
  for (uint8_t i = length_; i > cursor_; --i) {
    buffer_[i] = buffer_[i - 1];
  }

  // Insert and advance cursor
  buffer_[cursor_] = input;
  ++cursor_;
  ++length_;
  buffer_[length_] = '\0';
  return true;
}

bool Cursor::try_delete() {
  if (cursor_ == 0) {
    return false;
  }

  // Shift following characters left
  for (uint8_t i = cursor_; i < length_; ++i) {
    buffer_[i - 1] = buffer_[i];
  }

  // Backspace cursor
  --cursor_;
  --length_;
  buffer_[length_] = '\0';
  return true;
}

void History::push(const Cursor& cursor) {
  if (size_ == 0) {
    return;
  }

  // Limit entry size to absolute size of history buffer (excluding prefix)
  uint8_t size = util::min(cursor.length(), uint8_t(size_ - 1));
  uint8_t available = size_ - (size + 1);

  // Determine how many old entries will be overwritten
  uint8_t old_size = 0;
  for (uint8_t entry = 0; entry < entries_; ++entry) {
    uint8_t entry_size = 1 + buffer_[old_size]; // prefix byte plus size of entry
    if (old_size + entry_size > available) {
      // Drop this and any remaining entries
      entries_ = entry;
      break;
    }
    old_size += entry_size;
  }

  // Shift old entries back and copy new entry at beginning
  memmove(buffer_ + size + 1, buffer_, old_size);
  memcpy(buffer_ + 1, cursor.contents(), size);
  buffer_[0] = size;
  ++entries_;

  reset_index();
}

void History::copy_entry(uint8_t entry, Cursor& cursor) {
  if (entry >= entries_) {
    return;
  }

  // Skip forward `entry` list entries
  uint8_t index = 0;
  for (; entry > 0; --entry) {
    index += 1 + buffer_[index]; // skip prefix byte plus size of entry
  }

  // Copy entry into cursor
  cursor.clear();
  uint8_t size = buffer_[index];
  const char* offset = buffer_ + index + 1;
  cursor.try_insert(offset, size);
}

void History::copy_prev(Cursor& cursor) {
  if (index_ < entries_) {
    copy_entry(index_, cursor);
    ++index_;
  }
}

void History::copy_next(Cursor& cursor) {
  if (index_ > 0) {
    --index_;
    if (index_ > 0) {
      copy_entry(index_ - 1, cursor);
    }
  }
}

// Move cursor far left and delete line
inline void clear_line(serial::StreamEx& stream, Cursor& cursor) {
  stream.cursor_left(cursor.seek_home());
  stream.delete_char(cursor.length());
  cursor.clear();
}

bool try_read(serial::StreamEx& stream, Cursor& cursor, History& history) {
  using serial::StreamEx;
  int input = stream.read();
  switch (input) {
  case -1:
    break;
  case StreamEx::KEY_LEFT:
    if (cursor.try_left()) {
      stream.cursor_left();
    }
    break;
  case StreamEx::KEY_RIGHT:
    if (cursor.try_right()) {
      stream.cursor_right();
    }
    break;
  case StreamEx::KEY_HOME:
    // Move cursor far left
    stream.cursor_left(cursor.seek_home());
    break;
  case StreamEx::KEY_END:
    // Move cursor far right
    stream.cursor_right(cursor.seek_end());
    break;
  case StreamEx::KEY_UP:
    if (history.has_prev()) {
      clear_line(stream, cursor);
      history.copy_prev(cursor);
      stream.print(cursor.contents());
    }
    break;
  case StreamEx::KEY_DOWN:
    clear_line(stream, cursor);
    if (history.has_next()) {
      history.copy_next(cursor);
      stream.print(cursor.contents());
    }
    break;
  case '\x08': // ASCII backspace
  case '\x7F': // ASCII delete (not ANSI delete \e[3~)
    if (cursor.try_delete()) {
      stream.cursor_left();
      stream.delete_char();
    }
    break;
  case '\n': // NOTE StreamEx transforms \r and \r\n to \n
    if (cursor.length() > 0) {
      // Exit loop and execute command if line is not empty
      history.push(cursor);
      return true;
    }
    break;
  default:
    // Ignore other non-printable ASCII chars and uANSI input sequences
    // NOTE UTF-8 multi-byte encodings in [0x80, 0xFF] should be ok
    if (input < 0x20 || input > 0xFF) {
      break;
    }
    // Echo inserted character to stream
    if (cursor.try_insert(input)) {
      if (!cursor.at_eol()) {
        stream.insert_char();
      }
      stream.write(input);
      // Reset history index on edit
      history.reset_index();
    }
  }
  return false;
}

void Tokens::trim_left(char padding) {
  while (*next_ == padding) {
    ++next_;
  }
}

Tokens Tokens::split_at(char separator) {
  Tokens prev = next_;
  // Scan until end of string
  while (*next_ != '\0') {
    // Replace separator with null and return
    if (*next_ == separator) {
      *next_++ = '\0';
      break;
    }
    ++next_;
  }
  trim_left(' ');
  return prev;
}

const char* Tokens::next() {
  Tokens token;
  trim_left(' ');
  char c = peek_char();
  if (c == '\"' || c == '\'') {
    ++next_; // skip past open quote
    token = split_at(c);
  } else {
    token = split_at(' ');
  }
  return token.next_;
}

} // namespace cli
} // namespace core
