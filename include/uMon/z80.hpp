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
    opr.token = index_of_pgm_string(TOK_STR, opr_str);
    uMON_FMT_ERROR(opr.token == TOK_INVALID, "arg", opr_str);
  }

  if (is_indirect) {
    opr.token |= TOK_INDIRECT;
  }
}

template <typename API>
void cmd_asm(uCLI::Args args) {
  uMON_EXPECT_UINT(uint16_t, start, args);

  // Parse mnemonic
  Instruction inst;
  const char* mnemonic = args.next();
  inst.mnemonic = index_of_pgm_string(MNE_STR, mnemonic);
  uMON_FMT_ERROR(inst.mnemonic == MNE_INVALID, "op", mnemonic);

  // Parse operands
  for (Operand& op : inst.operands) {
    if (!args.has_next()) break;
    parse_operand<API>(args.split_at(','), op);
    if (op.token == TOK_INVALID) return;
  }
  // TODO error if unparsed operands remain?

  uint8_t size = impl_asm<API>(start, inst);
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
