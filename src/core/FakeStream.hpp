#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Minimal copy of Arduino Print class for native unit testing
class Print {
public:
  virtual int availableForWrite(void) = 0;
  virtual void flush(void) = 0;
  virtual size_t write(uint8_t c) = 0;
  size_t write(const char *str) {
    if (str == NULL) return 0;
    return write((const uint8_t *)str, strlen(str));
  }
  virtual size_t write(const uint8_t *buffer, size_t size) = 0;
  size_t write(const char *buffer, size_t size) {
    return write((const uint8_t *)buffer, size);
  }
  size_t print(const char *str) { return write(str); }
  size_t print(char c) { return write(c); }
  size_t println() { return write("\r\n"); }
  size_t println(const char* str) { return write(str) + println(); }
  size_t println(char c) { return write(c) + println(); }
};

// Minimal copy of Arduino Stream class for native unit testing
class Stream : public Print {
public:
  virtual int peek(void) = 0;
  virtual int read(void) = 0;
  virtual int available(void) = 0;
};
