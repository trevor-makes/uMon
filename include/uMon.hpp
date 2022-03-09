// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/format.hpp"
#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {

// Dump memory as hex/ascii from row to end, inclusive
template <typename API, uint8_t COL_SIZE = 16>
void impl_hex(uint16_t row, uint16_t end) {
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
    if (uint16_t(end - row) < COL_SIZE) { return; }
    row += COL_SIZE;
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
  // Default size to one row if not provided
  uint16_t start, size = COL_SIZE;
  if (!parse_unsigned<API>(args.next(), start)) { return; }
  if (args.has_next() && !parse_unsigned<API>(args.next(), size)) { return; }
  impl_hex<API, COL_SIZE>(start, start + size - 1);
}

template <typename API>
void cmd_set(uCLI::Args args) {
  uint16_t addr;
  uint8_t data;
  if (!parse_unsigned<API>(args.next(), addr)) { return; }
  do {
    if (args.is_string()) {
      addr = impl_strcpy<API>(addr, args.next());
    } else {
      if (!parse_unsigned<API>(args.next(), data)) { return; }
      API::write_byte(addr++, data);
    }
  } while (args.has_next());
  set_prompt<API>(args.command(), addr);
}

template <typename API>
void cmd_fill(uCLI::Args args) {
  uint16_t start, size;
  uint8_t pattern;
  if (!parse_unsigned<API>(args.next(), start)) { return; }
  if (!parse_unsigned<API>(args.next(), size)) { return; }
  if (!parse_unsigned<API>(args.next(), pattern)) { return; }
  impl_memset<API>(start, start + size - 1, pattern);
}

} // namespace uMon
