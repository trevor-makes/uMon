// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uCLI.hpp"

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

template <typename API, uint8_t COL_SIZE = 16>
void impl_hex(uint16_t start, uint16_t end) {
  uint8_t row_data[COL_SIZE];
  for (uint16_t row = start; row < end; row += COL_SIZE) {
    API::read_bytes(row, row_data);

    // Print address
    fmt_hex16(API::print_char, row);
    API::print_char(':');

    // Print hex data
    for (uint8_t col = 0; col < COL_SIZE; ++col) {
      API::print_char(' ');
      if (col % 4 == 0) {
        API::print_char(' ');
      }
      fmt_hex8(API::print_char, row_data[col]);
    }

    // Print string data
    API::print_string("  \"");
    for (uint8_t col = 0; col < COL_SIZE; ++col) {
      char m = row_data[col];
      if (m < ' ' || m >= 0x7F) {
        m = '.'; // Display control and non-ASCII as dot
      }
      API::print_char(m);
    }
    API::print_string("\"\n");
  }
}

template <typename API>
void impl_memset(uint16_t start, uint16_t end, uint8_t fill) {
  for (uint16_t i = start; i < end; ++i) {
    API::write_byte(i, fill);
  }
}

template <typename API>
void impl_strcpy(uint16_t start, const char* str) {
  for (;;) {
    char c = *str++;
    if (c == '\0')
      break;
    API::write_byte(start++, c);
  }
}

template <typename API, uint8_t COL_SIZE = 16>
void cmd_hex(uCLI::Tokens args) {
  uint16_t start = args.has_next() ? parse_u32(args.next()) : 0;
  uint16_t end = args.has_next() ? parse_u32(args.next()) : start + COL_SIZE;
  impl_hex<API, COL_SIZE>(start, end);
}

template <typename API>
void cmd_set(uCLI::Tokens args) {
  const char* argv[3];
  bool are_str[3];
  uint8_t argc = args.get(argv, are_str);
  if (argc < 2) {
    API::print_string("loc str\n");
    API::print_string("loc (end) int\n");
    return;
  }
  uint16_t start = uMon::parse_u32(argv[0]);
  if (are_str[1]) {
    // Set mem[start:] to 2nd arg as string
    impl_strcpy<API>(start, argv[1]); // TODO should this just be an API function?
  } else if (argc == 2) {
    // Set mem[start] to 2nd arg as number
    uint8_t fill = uMon::parse_u32(argv[1]);
    impl_memset<API>(start, start + 1, fill);
  } else if (argc == 3) {
    // Set mem[start:end] to 3rd arg as number
    uint16_t end = uMon::parse_u32(argv[1]);
    uint8_t fill = uMon::parse_u32(argv[2]);
    impl_memset<API>(start, end, fill);
  }
}

} // namespace uMon
