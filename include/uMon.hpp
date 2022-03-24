// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/format.hpp"
#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {

// Dump memory as hex/ascii from row to end, inclusive
template <typename API, uint8_t COL_SIZE = 16, uint8_t MAX_ROWS = 24>
uint16_t impl_hex(uint16_t row, uint16_t end) {
  uint8_t row_data[COL_SIZE];
  for (uint8_t i = 0; i < MAX_ROWS; ++i) {
    API::read_bytes(row, row_data);

    // Print address
    format_hex16(API::print_char, row);
    API::print_char(':');

    // Print hex data
    for (uint8_t col = 0; col < COL_SIZE; ++col) {
      API::print_char(' ');
      if (col % 4 == 0) {
        API::print_char(' ');
      }
      format_hex8(API::print_char, row_data[col]);
    }

    // Print string data
    API::print_string("  \"");
    for (uint8_t col = 0; col < COL_SIZE; ++col) {
      fmt_ascii(API::print_char, row_data[col]);
    }
    API::print_string("\"\n");

    // Do while end does not overlap with row
    uint16_t prev = row;
    row += COL_SIZE;
    if (uint16_t(end - prev) < COL_SIZE) { break; }
  }
  return row;
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

// Copy [start, end] to [dest, dest+end-start] (end inclusive)
template <typename API>
void impl_memmove(uint16_t start, uint16_t end, uint16_t dest) {
  uint16_t delta = end - start;
  uint16_t dest_end = dest + delta;
  // Buses narrower than 16-bits introduce cases with ghosting (wrap-around).
  // This logic should work as long as start and dest are both within [0, 2^N),
  // where N is the actual bus width.
  // See [notes/memmove.png]
  bool a = dest <= end;
  bool b = dest_end < start;
  bool c = dest > start;
  if ((a && b) || (a && c) || (b && c)) {
    // Reverse copy from end to start
    for (uint16_t i = 0; i <= delta; ++i) {
      API::write_byte(dest_end - i, API::read_byte(end - i));
    }
  } else {
    // Forward copy from start to end
    for (uint16_t i = 0; i <= delta; ++i) {
      API::write_byte(dest + i, API::read_byte(start + i));
    }
  }
}

template <typename API, uint8_t COL_SIZE = 16, uint8_t MAX_ROWS = 24>
void cmd_hex(uCLI::Args args) {
  // Default size to one row if not provided
  uMON_EXPECT_UINT(uint16_t, start, args, return);
  uMON_OPTION_UINT(uint16_t, size, COL_SIZE, args, return);
  uint16_t end_incl = start + size - 1;
  uint16_t next = impl_hex<API, COL_SIZE, MAX_ROWS>(start, end_incl);
  uint16_t part = next - start;
  if (part < size) {
    set_prompt<API>(args.command(), next, size - part);
  }
}

template <typename API>
void cmd_set(uCLI::Args args) {
  uMON_EXPECT_UINT(uint16_t, start, args, return);
  do {
    if (args.is_string()) {
      start = impl_strcpy<API>(start, args.next());
    } else {
      uMON_EXPECT_UINT(uint8_t, data, args, return);
      API::write_byte(start++, data);
    }
  } while (args.has_next());
  set_prompt<API>(args.command(), start);
}

template <typename API>
void cmd_fill(uCLI::Args args) {
  uMON_EXPECT_UINT(uint16_t, start, args, return);
  uMON_EXPECT_UINT(uint16_t, size, args, return);
  uMON_EXPECT_UINT(uint8_t, pattern, args, return);
  impl_memset<API>(start, start + size - 1, pattern);
}

template <typename API>
void cmd_move(uCLI::Args args) {
  uMON_EXPECT_UINT(uint16_t, start, args, return);
  uMON_EXPECT_UINT(uint16_t, size, args, return);
  uMON_EXPECT_UINT(uint16_t, dest, args, return);
  impl_memmove<API>(start, start + size - 1, dest);
}

} // namespace uMon
