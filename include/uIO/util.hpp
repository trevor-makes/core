// Copyright (c) 2022 Trevor Makes

#pragma once

#include <stdint.h>

namespace util {

// Convert type <volatile T&> to T
template <typename>
struct remove_volatile_reference;

template <typename T>
struct remove_volatile_reference<volatile T&> {
  using type = T;
};

// Value is true only when types are equal
template <typename TYPE1, typename TYPE2>
struct is_same {
  static constexpr const bool value = false;
};

template <typename TYPE>
struct is_same<TYPE, TYPE> {
  static constexpr const bool value = true;
};

static_assert(is_same<volatile float&, float>::value == false);
static_assert(is_same<remove_volatile_reference<volatile float&>::type, float>::value == true);

// Get unsigned type with doubled word size
template <typename>
struct extend_unsigned;

template <>
struct extend_unsigned<uint8_t> {
  using type = uint16_t;
};

template <>
struct extend_unsigned<uint16_t> {
  using type = uint32_t;
};

template <>
struct extend_unsigned<uint32_t> {
  using type = uint64_t;
};

template <typename T>
constexpr T less_one_until_zero(T N) { return N > 0 ? N - 1 : 0; }

//http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
template <uint8_t N, typename T, typename R = uint8_t, T S = T(1) << N, T P = ((T(1) << S) - 1)>
constexpr R ilog2_impl(T v) {
  return N > 0 ? (v > P) << N | ilog2_impl<less_one_until_zero(N)>(v >> ((v > P) << N)) : (v > P);
}

// Return integer binary logarithm
// (same as 0-indexed position of the highest set bit)
template <typename T, typename R = uint8_t>
constexpr R ilog2(T v) {
  return ilog2_impl<2 + ilog2_impl<1>(sizeof(T))>(v);
}

static_assert(ilog2(0x00) == 0);
static_assert(ilog2(0x01) == 0);
static_assert(ilog2(0x02) == 1);
static_assert(ilog2(0x03) == 1);
static_assert(ilog2(0x04) == 2);
static_assert(ilog2(0x07) == 2);
static_assert(ilog2(0x08) == 3);
static_assert(ilog2(0x0F) == 3);
static_assert(ilog2(0x10) == 4);
static_assert(ilog2(0x1F) == 4);
static_assert(ilog2(0x20) == 5);
static_assert(ilog2(0x3F) == 5);
static_assert(ilog2(0x40) == 6);
static_assert(ilog2(0x7F) == 6);
static_assert(ilog2(0x80) == 7);
static_assert(ilog2(0xFF) == 7);
static_assert(ilog2(0xFFFF) == 15);
static_assert(ilog2(0xFFFFFF) == 23);
static_assert(ilog2(0xFFFFFFFF) == 31);
static_assert(ilog2(0xFFFFFFFFFFFF) == 47);
static_assert(ilog2(0xFFFFFFFFFFFFFFFF) == 63);

// Return number of bits needed to represent value
// (same as 1-indexed position of the highest set bit)
template <typename T, typename R = uint8_t>
constexpr R mask_width(T v) {
  return v > 0 ? ilog2(v) + 1 : 0;
}

static_assert(mask_width(0) == 0);
static_assert(mask_width(1) == 1);
static_assert(mask_width(2) == 2);
static_assert(mask_width(4) == 3);
static_assert(mask_width(15) == 4);
static_assert(mask_width(31) == 5);

// Return number of consecutive zero bits starting from lsb
// NOTE std::countr_zero added to <bit> in c++20
//http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel
template <typename T>
constexpr uint8_t countr_zero(T v) {
  return sizeof(T) * 8
    - ((v & -v) ? 1 : 0)
    - ((v & -v & 0x00000000FFFFFFFF) && sizeof(T) > 4 ? 32 : 0)
    - ((v & -v & 0x0000FFFF0000FFFF) && sizeof(T) > 2 ? 16 : 0)
    - ((v & -v & 0x00FF00FF00FF00FF) && sizeof(T) > 1 ? 8 : 0)
    - ((v & -v & 0x0F0F0F0F0F0F0F0F) ? 4 : 0)
    - ((v & -v & 0x3333333333333333) ? 2 : 0)
    - ((v & -v & 0x5555555555555555) ? 1 : 0);
}

static_assert(countr_zero(uint8_t(0)) == 8);
static_assert(countr_zero(uint16_t(0)) == 16);
static_assert(countr_zero(uint32_t(0)) == 32);
static_assert(countr_zero(uint64_t(0)) == 64);
static_assert(countr_zero(1) == 0);
static_assert(countr_zero(0x10) == 4);
static_assert(countr_zero(0x100) == 8);
static_assert(countr_zero(0x1000) == 12);
static_assert(countr_zero(0x8000) == 15);
static_assert(countr_zero(0x80000000) == 31);
static_assert(countr_zero(0x8000000000000000) == 63);
static_assert(countr_zero(0xF0F0F0F0) == 4);

// Return number of consecutive zero bits starting from msb
// NOTE std::countl_zero added to <bit> in c++20
template <typename T, typename R = uint8_t>
constexpr R countl_zero(T v) {
  return sizeof(T) * 8 - mask_width(v);
}

static_assert(countl_zero<uint16_t>(0x0000) == 16);
static_assert(countl_zero<uint16_t>(0x0001) == 15);
static_assert(countl_zero<uint16_t>(0x7FFF) == 1);
static_assert(countl_zero<uint16_t>(0x8000) == 0);

static_assert(countl_zero(uint32_t(0)) == 32);
static_assert(countl_zero(uint32_t(1)) == 31);
static_assert(countl_zero(uint32_t(0x80000000)) == 0);
static_assert(countl_zero(uint32_t(0xF0F0F0F0)) == 0);
static_assert(countl_zero(uint32_t(0x0F0F0F0F)) == 4);

} // namespace util
