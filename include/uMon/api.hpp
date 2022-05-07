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

  // static uCLI::CLI<>& get_cli()
  static void prompt_char(char c) { T::get_cli().insert(c); }
  static void prompt_string(const char* str) { T::get_cli().insert(str); }

  template <uint8_t N>
  static void read_bytes(uint16_t addr, uint8_t (&buf)[N]) {
    for (uint8_t i = 0; i < N; ++i) {
      buf[i] = T::read_byte(addr + i);
    }
  }

private:
  static char buffer[LBL_SIZE];
  static uMon::Labels labels;
};

template <typename T, uint8_t N>
char Base<T, N>::buffer[N];

template <typename T, uint8_t N>
uMon::Labels Base<T, N>::labels(Base<T, N>::buffer);

} // namespace uMon
