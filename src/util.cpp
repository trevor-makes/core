// Copyright (c) 2022 Trevor Makes

#include <stdint.h>

namespace util {

uint8_t reverse_bits(uint8_t b) {
  // https://stackoverflow.com/a/2602885
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

} // namespace util
