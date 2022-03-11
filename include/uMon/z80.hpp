// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "z80/asm.hpp"
#include "z80/dasm.hpp"
#include "uCLI.hpp"

namespace uMon {
namespace z80 {

template <typename API>
void cmd_asm(uCLI::Args args) {
  uMON_EXPECT_UINT(uint16_t, start, args);

  const char* mne_str = args.next();
  uint8_t mne = find_progmem(MNE_STR, mne_str);
  uMON_FMT_ERROR(mne == MNE_INVALID, "op", mne_str);

  // TODO just print for now
  print_progmem<API>(MNE_STR, mne);
  API::print_char(' ');

  // Parse up to 3 operands
  // NOTE only need 2 for documented ops; 3 for 
  uint8_t n_ops = 0;
  Operand operands[3];
  for (; n_ops < 3 && args.has_next(); ++n_ops) {
    uCLI::Tokens opr_tok = args.split_at(',');

    operands[n_ops].token = 0;
    operands[n_ops].value = 0;

    // Handle indirect operand surronded by parentheses
    if (opr_tok.peek_char() == '(') {
      operands[n_ops].token = TOK_INDIRECT;
      opr_tok.split_at('(');
      opr_tok = opr_tok.split_at(')');

      // Split optional displacement following +/-
      uCLI::Tokens disp_tok = opr_tok;
      bool is_minus = false;
      disp_tok.split_at('+');
      if (!disp_tok.has_next()) {
        disp_tok = opr_tok;
        disp_tok.split_at('-');
        is_minus = true;
      }

      // Parse displacement and apply sign
      uMON_OPTION_UINT(uint16_t, disp, disp_tok, 0);
      if (disp != 0) {
        operands[n_ops].value = is_minus ? -disp : disp;
      }
    }

    // Parse operand as char, number, or token
    bool is_string = opr_tok.is_string();
    auto opr_str = opr_tok.next();
    uint16_t value;
    if (is_string) {
      uMON_FMT_ERROR(strlen(opr_str) > 1, "char", opr_str);
      operands[n_ops].token |= TOK_INTEGER;
      operands[n_ops].value = opr_str[0];
    } else if (uMon::parse_unsigned(opr_str, value)) {
      operands[n_ops].token |= TOK_INTEGER;
      operands[n_ops].value = value;
    } else {
      uint8_t token = find_progmem(TOK_STR, opr_str);
      uMON_FMT_ERROR(token == TOK_INVALID, "arg", opr_str);
      operands[n_ops].token |= token;
    }

    // TODO just print for now
    uint8_t token = operands[n_ops].token;
    value = operands[n_ops].value;
    if ((token & TOK_INDIRECT) != 0) {
      API::print_char('*');
      token &= ~TOK_INDIRECT;
    }
    if (token < TOK_INVALID) {
      print_progmem<API>(TOK_STR, token);
      if (value != 0) {
        API::print_char('+');
        fmt_hex16(API::print_char, value);
      }
    } else if (token == TOK_INTEGER) {
      fmt_hex16(API::print_char, value);
    } else {
      API::print_char('?');
    }
    API::print_char(',');
  }

  // TODO just print for now
  API::print_char('\n');

  uint16_t next = /* TODO */ start;
  set_prompt<API>(args.command(), next);
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
