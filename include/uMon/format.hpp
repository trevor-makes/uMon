// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#ifdef AVR
#include <avr/pgmspace.h>
#else
#include "FakePgm.hpp"
#endif

#include <stdint.h>
#include <stdlib.h>

namespace uMon {

// Parse unsigned value from string, returning true on success
// Prints the string and returns false if invalid characters found
// Supports prefixes $ for hex, & for octal, and % for binary
template <typename T>
bool parse_unsigned(const char* str, T& result) {
  char* end;
  // Handle custom prefixes for non-decimal bases
  switch (str[0]) {
  case '$': result = strtoul(++str, &end, 16); break; // hex
  case '&': result = strtoul(++str, &end, 8); break; // octal
  case '%': result = strtoul(++str, &end, 2); break; // binary
  default:  result = strtoul(str, &end, 10); break; // decimal
  }
  // Return true if all characters were parsed
  return end != str && *end == '\0';
}

// Print single hex digit (or garbage if n > 15)
template <typename F>
void format_hex4(F&& print, uint8_t n) {
  print(n < 10 ? '0' + n : 'A' - 10 + n);
}

// Print 2 hex digits with leading zeroes
template <typename F>
void format_hex(F&& print, uint8_t n) {
  format_hex4(print, n >> 4);
  format_hex4(print, n & 0xF);
}

template <typename F>
void format_hex8(F&& print, uint8_t n) { format_hex(print, n); }

// Print 4 hex digits with leading zeroes
template <typename F>
void format_hex(F&& print, uint16_t n) {
  format_hex8(print, n >> 8);
  format_hex8(print, n & 0xFF);
}

template <typename F>
void format_hex16(F&& print, uint16_t n) { format_hex(print, n); }

// Print 8 hex digits with leading zeroes
template <typename F>
void format_hex(F&& print, uint32_t n) {
  format_hex16(print, n >> 16);
  format_hex16(print, n & 0xFFFF);
}

template <typename F>
void format_hex32(F&& print, uint32_t n) { format_hex(print, n); }

template <typename F>
void fmt_ascii(F&& print, uint8_t c) {
  if (c < ' ' || c >= 0x7F) {
    c = '.'; // Display control and non-ASCII as dot
  }
  print(c);
}

// Set CLI prompt to "[cmd] "
template <typename API, size_t N = 0>
void set_prompt(const char* cmd) {
  API::prompt_string(cmd);
  API::prompt_char(' ');
}

// Set CLI prompt to "[cmd] $[arg] ..."
template <typename API, bool First = true, typename T, typename... Var>
void set_prompt(const char* cmd, const T arg, const Var... var)
{
  // Prepend cmd
  if (First) {
    set_prompt<API>(cmd);
  }
  API::prompt_char('$');
  format_hex(API::prompt_char, arg);
  API::prompt_char(' ');
  // Append following args
  if (sizeof...(Var) > 0) {
    set_prompt<API, false, Var...>(cmd, var...);
  }
}

template <typename API>
void print_pgm_string(const char* str) {
  for (;;) {
    char c = pgm_read_byte(str++);
    if (c == '\0') return;
    API::print_char(c);
  }
}

// Print entry from PROGMEM string table
template <typename API>
void print_pgm_table(const char* const table[], uint8_t index) {
  char* str = (char*)pgm_read_ptr(table + index);
  print_pgm_string<API>(str);
}

// Find index of string in PROGMEM table
template <uint8_t N>
uint8_t pgm_bsearch(const char* const (&table)[N], const char* str) {
  char* res = (char*)bsearch(str, table, N, sizeof(table[0]),
    [](const void* key, const void* entry) {
      return strcasecmp_P((char*)key, (char*)pgm_read_ptr((const char* const*)entry));
    });
  return res == nullptr ? N : (res - (char*)table) / sizeof(table[0]);
}

#define uMON_FMT_ERROR(IS_ERR, LABEL, STRING, RET) \
  if (IS_ERR) { \
    const char* str = STRING; \
    API::print_string(LABEL); \
    if (*str != '\0') { \
      API::print_string(": "); \
      API::print_string(str); \
    } \
    API::print_string("?\n"); \
    RET; \
  }

#define uMON_EXPECT_UINT(TYPE, NAME, ARGS, RET) \
  TYPE NAME; \
  { \
    const char* str = ARGS.next(); \
    const bool is_err = !parse_unsigned(str, NAME); \
    uMON_FMT_ERROR(is_err, #NAME, str, RET) \
  }

#define uMON_OPTION_UINT(TYPE, NAME, DEFAULT, ARGS, RET) \
  TYPE NAME = DEFAULT; \
  if (ARGS.has_next()) { \
    const char* str = ARGS.next(); \
    const bool is_err = !parse_unsigned(str, NAME); \
    uMON_FMT_ERROR(is_err, #NAME, str, RET); \
  }

} // namespace uMon
