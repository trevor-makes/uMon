// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/format.hpp"
#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {

// Dump memory as hex/ascii from row to end, inclusive
template <typename API, uint8_t COL_SIZE = 16>
uint16_t impl_hex(uint16_t row, uint16_t end) {
  uint8_t row_data[COL_SIZE];
  for (;;) {
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
      fmt_ascii(API::print_char, row_data[col]);
    }
    API::print_string("\"\n");

    // Do while end does not overlap with row
    uint16_t next = row + COL_SIZE;
    if (uint16_t(end - row) < COL_SIZE) {
      return next;
    }
    row = next;
  }
}

// Write pattern from start to end, inclusive
template <typename API>
void impl_memset(uint16_t start, uint16_t end, uint8_t pattern) {
  do {
    API::write_byte(start, pattern);
  } while (start++ != end);
}

// Write string from start until null terminator
template <typename API>
uint16_t impl_strcpy(uint16_t start, const char* str) {
  for (;;) {
    char c = *str++;
    if (c == '\0') {
      return start;
    }
    API::write_byte(start++, c);
  }
}

template <typename API, uint8_t COL_SIZE = 16>
void cmd_hex(uCLI::Args args) {
  uint16_t start = args.has_next() ? parse_u32(args.next()) : 0;
  uint16_t size = args.has_next() ? parse_u32(args.next()) : COL_SIZE;
  uint16_t next = impl_hex<API, COL_SIZE>(start, start + size - 1);
  set_prompt<API>(args.command(), next);
}

template <typename API>
void cmd_set(uCLI::Args args) {
  const char* argv[3];
  bool are_str[3];
  uint8_t argc = args.get(argv, are_str);
  if (argc < 2) {
    API::print_string("loc str\n");
    API::print_string("loc (len) int\n");
    return;
  }
  uint16_t start = uMon::parse_u32(argv[0]);
  if (are_str[1]) {
    // Set mem[start:] to string
    uint16_t next = impl_strcpy<API>(start, argv[1]);
    set_prompt<API>(args.command(), next);
  } else if (argc == 2) {
    // Set mem[start] to pattern
    uint8_t pattern = uMon::parse_u32(argv[1]);
    impl_memset<API>(start, start, pattern);
    set_prompt<API>(args.command(), start + 1);
  } else if (argc >= 3) {
    // Set mem[start:start+size] to pattern
    uint16_t size = uMon::parse_u32(argv[1]);
    uint8_t pattern = uMon::parse_u32(argv[2]);
    impl_memset<API>(start, start + size - 1, pattern);
  }
}

} // namespace uMon
