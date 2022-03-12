// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "z80/asm.hpp"
#include "z80/dasm.hpp"
#include "uCLI.hpp"

namespace uMon {
namespace z80 {

template <typename API>
void parse_operand(uCLI::Tokens tokens, Operand& opr) {
  opr.token = TOK_INVALID;
  opr.value = 0;

  // Handle indirect operand surronded by parentheses
  bool is_indirect = false;
  if (tokens.peek_char() == '(') {
    is_indirect = true;
    tokens.split_at('(');
    tokens = tokens.split_at(')');

    // Split optional displacement following +/-
    uCLI::Tokens disp_tok = tokens;
    bool is_minus = false;
    disp_tok.split_at('+');
    if (!disp_tok.has_next()) {
      disp_tok = tokens;
      disp_tok.split_at('-');
      is_minus = true;
    }

    // Parse displacement and apply sign
    uMON_OPTION_UINT(uint16_t, disp, disp_tok, 0);
    opr.value = is_minus ? -disp : disp;
  }

  // Parse operand as char, number, or token
  bool is_string = tokens.is_string();
  auto opr_str = tokens.next();
  uint16_t value;
  if (is_string) {
    uMON_FMT_ERROR(strlen(opr_str) > 1, "char", opr_str);
    opr.token = TOK_INTEGER;
    opr.value = opr_str[0];
  } else if (uMon::parse_unsigned(opr_str, value)) {
    opr.token = TOK_INTEGER;
    opr.value = value;
  } else {
    opr.token = find_pgm_strtab(TOK_STR, opr_str);
    uMON_FMT_ERROR(opr.token == TOK_INVALID, "arg", opr_str);
  }

  if (is_indirect) {
    opr.token |= TOK_INDIRECT;
  }
}

template <typename API>
void cmd_asm(uCLI::Args args) {
  uMON_EXPECT_UINT(uint16_t, start, args);

  const char* mne_str = args.next();
  uint8_t mne = find_pgm_strtab(MNE_STR, mne_str);
  uMON_FMT_ERROR(mne == MNE_INVALID, "op", mne_str);

  // Parse up to 3 operands
  // NOTE only need 2 for documented ops; 3 for undocumented BIT/RES/SET
  uint8_t n_ops = 0;
  Operand operands[3];
  for (; n_ops < 3 && args.has_next(); ++n_ops) {
    parse_operand<API>(args.split_at(','), operands[n_ops]);
    if ((operands[n_ops].token & ~TOK_INDIRECT) == TOK_INVALID) {
      return;
    }
  }

  // TODO just print for now
  print_pgm_strtab<API>(MNE_STR, mne);
  API::print_char(' ');
  for (uint8_t i = 0; i < n_ops; ++i) {
    if (i != 0) API::print_char(',');
    print_operand<API>(operands[i]);
  }
  API::print_char('\n');

  uint8_t size = impl_asm<API>(start, mne);
  if (size > 0) {
    set_prompt<API>(args.command(), start + size);
  }
}

template <typename API>
void cmd_dasm(uCLI::Args args) {
  // Default size to one instruction if not provided
  uMON_EXPECT_UINT(uint16_t, start, args);
  uMON_OPTION_UINT(uint16_t, size, args, 1);
  uint16_t next = impl_dasm<API>(start, start + size - 1);
  set_prompt<API>(args.command(), next);
}

} // namespace z80
} // namespace uMon
