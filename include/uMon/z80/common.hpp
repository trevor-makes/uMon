// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

namespace uMon {
namespace z80 {

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
#define ITEM(x) TOK_##x,
#include "tokens.def"
#undef ITEM
  TOK_INVALID,
  TOK_INTEGER,
  TOK_MASK = 0x1F,
  TOK_BYTE = 0x20,
  TOK_DIGIT = 0x40,
  TOK_INDIRECT = 0x80,
};

// Individual token strings in Flash memory
#define ITEM(x) const char TOK_STR_##x[] PROGMEM = #x;
#include "tokens.def"
#undef ITEM

// Flash memory table of token strings
const char* const TOK_STR[] PROGMEM = {
#define ITEM(x) TOK_STR_##x,
#include "tokens.def"
#undef ITEM
};

// ============================================================================
// Register Encodings
// ============================================================================

#define REG_LIST \
ITEM(B, TOK_B) ITEM(C, TOK_C) ITEM(D, TOK_D) ITEM(E, TOK_E) ITEM(H, TOK_H) \
ITEM(L, TOK_L) ITEM(M, TOK_INDIRECT | TOK_HL) ITEM(A, TOK_A)

enum {
#define ITEM(name, token) REG_##name,
REG_LIST
#undef ITEM
};

// Mapping from reg encoding to token
const uint8_t REG_TOK[] = {
#define ITEM(name, token) token,
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
};

// Maximum number of operands encoded by an instruction
constexpr const uint8_t MAX_OPERANDS = 2;

// Data fields common to all instruction types
struct Instruction {
  uint8_t mnemonic;
  Operand operands[MAX_OPERANDS];
};

template <typename API>
void print_token(uint8_t token) {
  const bool is_indirect = (token & TOK_INDIRECT) != 0;
  if (is_indirect) API::print_char('(');
  print_pgm_strtab<API>(TOK_STR, token & ~TOK_INDIRECT);
  if (is_indirect) API::print_char(')');
}

// Nicely format an instruction operand
template <typename API>
void print_operand(Operand& op) {
  const bool is_indirect = (op.token & TOK_INDIRECT) != 0;
  const bool is_byte = (op.token & TOK_BYTE) != 0;
  const bool is_digit = (op.token & TOK_DIGIT) != 0;
  const uint8_t token = op.token & TOK_MASK;
  if (is_indirect) API::print_char('(');
  if (token < TOK_INVALID) {
    print_pgm_strtab<API>(TOK_STR, token);
    if (op.value != 0) {
      int8_t value = op.value;
      API::print_char(value < 0 ? '-' : '+');
      API::print_char('$');
      fmt_hex8(API::print_char, value < 0 ? -value : value);
    }
  } else if (token == TOK_INTEGER) {
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
  print_pgm_strtab<API>(MNE_STR, inst.mnemonic);
  for (uint8_t i = 0; i < MAX_OPERANDS; ++i) {
    Operand& op = inst.operands[i];
    if (op.token == TOK_INVALID) break;
    API::print_char(i == 0 ? ' ' : ',');
    print_operand<API>(op);
  }
}

} // namespace z80
} // namespace uMon
