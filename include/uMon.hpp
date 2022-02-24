// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include <stdint.h>

namespace uMon {

uint32_t parse_u32(const char* str);

// Print single hex digit (or garbage if n > 15)
template <typename F>
void fmt_hex4(F&& print, uint8_t n) {
  print(n < 10 ? '0' + n : 'A' - 10 + n);
}

// Print 2 hex digits with leading zeroes
template <typename F>
void fmt_hex8(F&& print, uint8_t n) {
  fmt_hex4(print, n >> 4);
  fmt_hex4(print, n & 0xF);
}

// Print 4 hex digits with leading zeroes
template <typename F>
void fmt_hex16(F&& print, uint16_t n) {
  fmt_hex8(print, n >> 8);
  fmt_hex8(print, n & 0xFF);
}

// Print 8 hex digits with leading zeroes
template <typename F>
void fmt_hex32(F&& print, uint32_t n) {
  fmt_hex16(print, n >> 16);
  fmt_hex16(print, n & 0xFFFF);
}

} // namespace uMon
