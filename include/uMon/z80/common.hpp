// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

namespace uMon {
namespace z80 {

constexpr const uint8_t PREFIX_IX = 0xDD;
constexpr const uint8_t PREFIX_IY = 0xFD;
constexpr const uint8_t PREFIX_ED = 0xED;
constexpr const uint8_t PREFIX_CB = 0xCB;

// ============================================================================
// Mnemonic Definitions
// ============================================================================

// Alphabetic index of assembly mnemonics
enum {
#define ITEM(x) MNE_##x,
#include "mnemonics.def"
#undef ITEM
  MNE_INVALID,
};

// Individual mnemonic strings in Flash memory
#define ITEM(x) const char MNE_STR_##x[] PROGMEM = #x;
#include "mnemonics.def"
#undef ITEM

// Flash memory table of mnemonic strings
const char* const MNE_STR[] PROGMEM = {
#define ITEM(x) MNE_STR_##x,
#include "mnemonics.def"
#undef ITEM
};

// ============================================================================
// ALU Encodings
// ============================================================================

#define ALU_LIST \
ITEM(ADD) ITEM(ADC) ITEM(SUB) ITEM(SBC) ITEM(AND) ITEM(XOR) ITEM(OR) ITEM(CP)

enum {
#define ITEM(x) ALU_##x,
ALU_LIST
#undef ITEM
};

// Mapping from ALU encoding to mnemonic
const uint8_t ALU_MNE[] = {
#define ITEM(x) MNE_##x,
ALU_LIST
#undef ITEM
};

#undef ALU_LIST

// ============================================================================
// CB-prefix OP Encodings
// ============================================================================

#define CB_LIST ITEM(BIT) ITEM(RES) ITEM(SET)

enum {
  CB_ROT, // Ops with additional 3-bit encoding
#define ITEM(x) CB_##x,
CB_LIST
#undef ITEM
};

// Mapping from CB op to mnemonic
const uint8_t CB_MNE[] = {
  MNE_INVALID,
#define ITEM(x) MNE_##x,
CB_LIST
#undef ITEM
};

#undef CB_LIST

// ============================================================================
// CB-prefix ROT Encodings
// ============================================================================

#define ROT_LIST \
ITEM(RLC) ITEM(RRC) ITEM(RL) ITEM(RR) ITEM(SLA) ITEM(SRA) ITEM(SL1) ITEM(SRL)

enum {
#define ITEM(x) ROT_##x,
ROT_LIST
#undef ITEM
};

// Mapping from ROT op to mnemonic
const uint8_t ROT_MNE[] = {
#define ITEM(x) MNE_##x,
ROT_LIST
#undef ITEM
};

#undef ROT_LIST

// ============================================================================
// Misc Encodings
// ============================================================================

#define MISC_LIST \
ITEM(RLCA) ITEM(RRCA) ITEM(RLA) ITEM(RRA) ITEM(DAA) ITEM(CPL) ITEM(SCF) ITEM(CCF)

enum {
#define ITEM(x) MISC_##x,
MISC_LIST
#undef ITEM
};

// Mapping from MISC op to mnemonic
const uint8_t MISC_MNE[] = {
#define ITEM(x) MNE_##x,
MISC_LIST
#undef ITEM
};

#undef MISC_LIST

// ============================================================================
// Token Definitions
// ============================================================================

// Alphabetic index of all registers, pairs, and conditions
enum {
  TOK_UNDEFINED,
#define ITEM(x) TOK_##x,
#include "tokens.def"
#undef ITEM
  TOK_INVALID,
  TOK_IMMEDIATE,
  TOK_MASK = 0x1F,
  TOK_BYTE = 0x20,
  TOK_DIGIT = 0x40,
  TOK_INDIRECT = 0x80,
  TOK_IMM_IND = TOK_IMMEDIATE | TOK_INDIRECT,
  TOK_BC_IND = TOK_BC | TOK_INDIRECT,
  TOK_DE_IND = TOK_DE | TOK_INDIRECT,
  TOK_HL_IND = TOK_HL | TOK_INDIRECT,
  TOK_IX_IND = TOK_IX | TOK_INDIRECT,
  TOK_IY_IND = TOK_IY | TOK_INDIRECT,
};

// Individual token strings in Flash memory
const char TOK_STR_UNDEFINED[] PROGMEM = "?";
#define ITEM(x) const char TOK_STR_##x[] PROGMEM = #x;
#include "tokens.def"
#undef ITEM

// Flash memory table of token strings
const char* const TOK_STR[] PROGMEM = {
  TOK_STR_UNDEFINED,
#define ITEM(x) TOK_STR_##x,
#include "tokens.def"
#undef ITEM
};

// ============================================================================
// Register Encodings
// ============================================================================

#define REG_LIST \
ITEM(B, TOK_B, TOK_B, TOK_B) \
ITEM(C, TOK_C, TOK_C, TOK_C) \
ITEM(D, TOK_D, TOK_D, TOK_D) \
ITEM(E, TOK_E, TOK_E, TOK_E) \
ITEM(H, TOK_H, TOK_IXH, TOK_IYH) \
ITEM(L, TOK_L, TOK_IXL, TOK_IYL) \
ITEM(M, TOK_HL_IND, TOK_IX_IND, TOK_IY_IND) \
ITEM(A, TOK_A, TOK_A, TOK_A)

enum {
#define ITEM(name, tok, tok_ix, tok_iy) REG_##name,
REG_LIST
#undef ITEM
  REG_INVALID,
};

// Mapping from reg encoding to token
const uint8_t REG_TOK[] = {
#define ITEM(name, tok, tok_ix, tok_iy) tok,
REG_LIST
#undef ITEM
};

// Mapping from reg encoding to token with IX prefix
const uint8_t REG_TOK_IX[] = {
#define ITEM(name, tok, tok_ix, tok_iy) tok_ix,
REG_LIST
#undef ITEM
};

// Mapping from reg encoding to token with IY prefix
const uint8_t REG_TOK_IY[] = {
#define ITEM(name, tok, tok_ix, tok_iy) tok_iy,
REG_LIST
#undef ITEM
};

#undef REG_LIST

// ============================================================================
// Register Pair Encodings
// ============================================================================

#define PAIR_LIST ITEM(BC) ITEM(DE) ITEM(HL) ITEM(SP)

enum {
#define ITEM(x) PAIR_##x,
PAIR_LIST
#undef ITEM
  PAIR_INVALID,
};

// Mapping from pair encoding to token
const uint8_t PAIR_TOK[] = {
#define ITEM(x) TOK_##x,
PAIR_LIST
#undef ITEM
};

#undef PAIR_LIST

// ============================================================================
// Branch Condition Encodings
// ============================================================================

#define COND_LIST \
ITEM(NZ) ITEM(Z) ITEM(NC) ITEM(C) ITEM(PO) ITEM(PE) ITEM(P) ITEM(M)

enum {
#define ITEM(x) COND_##x,
COND_LIST
#undef ITEM
  COND_INVALID,
};

// Mapping from cond encoding to token
const uint8_t COND_TOK[] = {
#define ITEM(x) TOK_##x,
COND_LIST
#undef ITEM
};

#undef COND_LIST

// ============================================================================
// Token Functions
// ============================================================================

// Data fields common to all operand types
struct Operand {
  uint8_t token;
  uint16_t value;

  Operand(): token(TOK_INVALID), value(0) {}
  Operand(uint8_t token, uint16_t value): token(token), value(value) {}
};

// Maximum number of operands encoded by an instruction
constexpr const uint8_t MAX_OPERANDS = 2;

// Data fields common to all instruction types
struct Instruction {
  uint8_t mnemonic;
  Operand operands[MAX_OPERANDS];

  Instruction(): mnemonic(MNE_INVALID) {}
};

// Nicely format an instruction operand
template <typename API>
void print_operand(Operand& op) {
  const bool is_indirect = (op.token & TOK_INDIRECT) != 0;
  const bool is_byte = (op.token & TOK_BYTE) != 0;
  const bool is_digit = (op.token & TOK_DIGIT) != 0;
  const uint8_t token = op.token & TOK_MASK;
  if (is_indirect) API::print_char('(');
  if (token < TOK_INVALID) {
    print_pgm_table<API>(TOK_STR, token);
    if (op.value != 0) {
      int8_t value = op.value;
      API::print_char(value < 0 ? '-' : '+');
      API::print_char('$');
      fmt_hex8(API::print_char, value < 0 ? -value : value);
    }
  } else if (token == TOK_IMMEDIATE) {
    if (is_digit) {
      API::print_char('0' + op.value);
    } else if (is_byte) {
      API::print_char('$');
      fmt_hex8(API::print_char, op.value);
    } else {
      API::print_char('$');
      fmt_hex16(API::print_char, op.value);
    }
  } else {
    API::print_char('?');
  }
  if (is_indirect) API::print_char(')');
}

// Nicely format an instruction and its operands
template <typename API>
void print_instruction(Instruction& inst) {
  if (inst.mnemonic == MNE_INVALID) {
    API::print_char('?');
    return;
  }
  print_pgm_table<API>(MNE_STR, inst.mnemonic);
  for (uint8_t i = 0; i < MAX_OPERANDS; ++i) {
    Operand& op = inst.operands[i];
    if (op.token == TOK_INVALID) break;
    API::print_char(i == 0 ? ' ' : ',');
    print_operand<API>(op);
  }
}

} // namespace z80
} // namespace uMon
