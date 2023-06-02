// Copyright (c) 2023 Trevor Makes

#pragma once

#include <stdint.h>

#define CORE_PORT(X) \
template <uint16_t MASK, bool B = (MASK == uint16_t(~0))> \
struct _Port##X; \
\
using Port##X = _Port##X<~0>;\
\
/* TODO maybe add functions for open-drain, high-Z, drive capability, etc */ \
template <uint16_t M> \
struct _PortBase##X { \
  using TYPE = uint16_t; \
  static const TYPE MASK = M; \
\
  /* Select bit within I/O port */ \
  template <uint8_t BIT> \
  using Bit = _Port##X<1 << BIT>; \
\
  /* Select masked region within I/O port */ \
  template <uint8_t SUBMASK> \
  using Mask = _Port##X<SUBMASK>; \
\
  template <uint16_t BITS = M, uint8_t I = 0> \
  static inline void set_pfs(uint32_t pfs) { \
    if constexpr (BITS & 1) { \
      R_PFS->PORT[X].PIN[I].PmnPFS = pfs; \
    } \
    if constexpr (BITS > 0) { \
      set_pfs<(BITS >> 1), I + 1>(pfs); \
    } \
  } \
\
  /* Configure port as output */ \
  static inline void config_output() { \
    /* R_PORT##X->PDR |= M; */ \
    set_pfs(bit(2)); /* set PIDR */ \
  } \
\
  /* Configure port as input */ \
  static inline void config_input() { \
    /* R_PORT##X->PDR &= TYPE(~M); */ \
    set_pfs(0); \
  } \
\
  /* Configure port as input with pullup registers */ \
  static inline void config_input_pullups() { \
    /* R_PORT##X->PDR &= TYPE(~M); */ \
    set_pfs(bit(4)); /* set PCR */ \
  } \
}; \
\
/* Unmasked port optimizations */ \
template <uint16_t MASK> \
struct _Port##X<MASK, true> : _PortBase##X<MASK> { \
  static_assert(MASK == uint16_t(~0), \
    "Unmasked port should have all MASK bits set"); \
\
  static inline void set() { bitwise_or(MASK); } \
  static inline void clear() { bitwise_and(0); } \
  static inline void write(uint16_t value) { R_PORT##X->PODR = value; } \
  static inline void bitwise_or(uint16_t value) { R_PORT##X->POSR = value; } \
  static inline void bitwise_and(uint16_t value) { R_PORT##X->PORR = ~value; } \
\
  static inline uint16_t read() { return R_PORT##X->PIDR; } \
  static inline bool is_clear() { return R_PORT##X->PIDR == 0; } \
  static inline bool is_set() { return R_PORT##X->PIDR == MASK; } \
}; \
\
/* Masked port */ \
template <uint16_t MASK> \
struct _Port##X<MASK, false> : _PortBase##X<MASK> { \
\
  static inline void set() { bitwise_or(MASK); } \
  static inline void clear() { bitwise_and(0); } \
  static inline void write(uint16_t value) { R_PORT##X->PODR = (R_PORT##X->PODR & uint16_t(~MASK)) | (value & MASK); } \
  static inline void bitwise_or(uint16_t value) { R_PORT##X->POSR = value & MASK; } \
  static inline void bitwise_and(uint16_t value) { R_PORT##X->PORR = ~value & MASK; } \
\
  static inline uint16_t read() { return R_PORT##X->PIDR & MASK; } \
  static inline bool is_clear() { return (R_PORT##X->PIDR & MASK) == 0; } \
  static inline bool is_set() { return (R_PORT##X->PIDR & MASK) == MASK; } \
};
