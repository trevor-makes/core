// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/labels.hpp"

namespace uMon {

template <typename T, uint8_t LBL_SIZE = 80>
struct Base {
  static uMon::Labels& get_labels() {
    return labels;
  }

  // static uANSI::StreamEx& get_stream()
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

  // static uCLI::CLI<>& get_cli()
  static void prompt_char(char c) { T::get_cli().prompt(c); }
  static void prompt_string(const char* str) { T::get_cli().prompt(str); }

  template <uint8_t N>
  static void read_bytes(uint16_t addr, uint8_t (&buf)[N]) {
    for (uint8_t i = 0; i < N; ++i) {
      buf[i] = T::read_byte(addr + i);
    }
  }

private:
  static uMon::LabelsOwner<LBL_SIZE> labels;
};

template <typename T, uint8_t N>
uMon::LabelsOwner<N> Base<T, N>::labels;

} // namespace uMon
