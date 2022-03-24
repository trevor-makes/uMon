// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#ifdef AVR
#include <avr/pgmspace.h>
#else
#include <strings.h>
#include <stdlib.h>
char pgm_read_byte(const char* ptr) {
  return *ptr;
}
const char* pgm_read_ptr(const char* const* ptr) {
  return *ptr;
}
#define strcasecmp_P strcasecmp
#define PROGMEM
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

template <typename F>
void fmt_ascii(F&& print, uint8_t c) {
  if (c < ' ' || c >= 0x7F) {
    c = '.'; // Display control and non-ASCII as dot
  }
  print(c);
}

// Set CLI prompt to "[cmd] $[addr] "
template <typename API>
void set_prompt(const char* cmd, uint16_t addr) {
  API::prompt_string(cmd);
  API::prompt_string(" $");
  fmt_hex16(API::prompt_char, addr);
  API::prompt_char(' ');
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
