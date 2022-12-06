// Copyright (c) 2022 Trevor Makes

#pragma once

#include "labels.hpp"

namespace core {
namespace mon {

// Default implementations for mon API
// struct API : Base<API> { 
//   struct BUS { ... };  or  using BUS = ...;
//   static StreamEx& get_stream() { ... }
//   static CLI<>& get_cli() { ... }
// };
template <typename T, uint8_t LBL_SIZE = 80>
struct Base {
  static Labels& get_labels() {
    return labels;
  }

  static void print_char(char c) { T::get_stream().print(c); }
  static void print_string(const char* str) { T::get_stream().print(str); }
  static void newline() { T::get_stream().println(); }

  static char input_char() {
    char c;
    do {
      c = T::get_stream().read();
    } while (c == -1);
    T::get_stream().print(c);
    return c;
  }

  static void prompt_char(char c) { T::get_cli().prompt(c); }
  static void prompt_string(const char* str) { T::get_cli().prompt(str); }

private:
  static LabelsOwner<LBL_SIZE> labels;
};

template <typename T, uint8_t N>
LabelsOwner<N> Base<T, N>::labels;

} // namespace mon
} // namespace core
