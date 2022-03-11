// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/z80/common.hpp"
#include "uMon/format.hpp"
#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {
namespace z80 {

struct Operand {
  uint8_t token;
  uint16_t value;
};

uint8_t parse_mnemonic(const char* str) {
  for (uint8_t i = 0; i < MNE_INVALID; ++i) {
    if (strcasecmp_P(str, (char*)pgm_read_ptr(MNE_STR + i)) == 0) {
      return i;
    }
  }
  return MNE_INVALID;
}

uint8_t parse_token(const char* str) {
  for (uint8_t i = 0; i < TOK_INVALID; ++i) {
    if (strcasecmp_P(str, (char*)pgm_read_ptr(TOK_STR + i)) == 0) {
      return i;
    }
  }
  return TOK_INVALID;
}

} // namespace z80
} // namespace uMon
