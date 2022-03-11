// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

namespace uMon {
namespace z80 {

#define MNEMONICS \
  ITEM(ADC)  ITEM(ADD)  ITEM(AND)  ITEM(BIT)  ITEM(CALL) ITEM(CCF)  ITEM(CP)   \
  ITEM(CPD)  ITEM(CPDR) ITEM(CPI)  ITEM(CPIR) ITEM(CPL)  ITEM(DAA)  ITEM(DEC)  \
  ITEM(DI)   ITEM(DJNZ) ITEM(EI)   ITEM(EX)   ITEM(EXX)  ITEM(HALT) ITEM(IM)   \
  ITEM(IN)   ITEM(INC)  ITEM(IND)  ITEM(INDR) ITEM(INI)  ITEM(INIR) ITEM(JP)   \
  ITEM(JR)   ITEM(LD)   ITEM(LDD)  ITEM(LDDR) ITEM(LDI)  ITEM(LDIR) ITEM(NEG)  \
  ITEM(NOP)  ITEM(OR)   ITEM(OTDR) ITEM(OTIR) ITEM(OUT)  ITEM(OUTD) ITEM(OUTI) \
  ITEM(POP)  ITEM(PUSH) ITEM(RES)  ITEM(RET)  ITEM(RETI) ITEM(RETN) ITEM(RL)   \
  ITEM(RLA)  ITEM(RLC)  ITEM(RLCA) ITEM(RLD)  ITEM(RR)   ITEM(RRA)  ITEM(RRC)  \
  ITEM(RRCA) ITEM(RRD)  ITEM(RST)  ITEM(SBC)  ITEM(SCF)  ITEM(SET)  ITEM(SL1)  \
  ITEM(SLA)  ITEM(SRA)  ITEM(SRL)  ITEM(SUB)  ITEM(XOR)

enum Mnemonic {
#define ITEM(x) MNE_##x,
MNEMONICS
#undef ITEM
  MNE_INVALID,
};

#define ITEM(x) const char MNE_STR_##x[] PROGMEM = #x;
MNEMONICS
#undef ITEM

const char* const MNE_STR[] PROGMEM = {
#define ITEM(x) MNE_STR_##x,
MNEMONICS
#undef ITEM
};

#undef MNEMONICS

#define TOKENS \
  ITEM(A)   ITEM(AF)  ITEM(B)   ITEM(BC)  ITEM(C)   ITEM(D)   ITEM(DE)  \
  ITEM(E)   ITEM(H)   ITEM(HL)  ITEM(I)   ITEM(IX)  ITEM(IXH) ITEM(IXL) \
  ITEM(IY)  ITEM(IYH) ITEM(IYL) ITEM(L)   ITEM(M)   ITEM(NC)  ITEM(NZ)  \
  ITEM(P)   ITEM(PE)  ITEM(PO)  ITEM(R)   ITEM(SP)  ITEM(Z)

// All registers, pairs, and conditions
enum Token {
#define ITEM(x) TOK_##x,
TOKENS
#undef ITEM
  TOK_INVALID,
  TOK_INTEGER,
  TOK_INDIRECT = 0x80,
};

#define ITEM(x) const char TOK_STR_##x[] PROGMEM = #x;
TOKENS
#undef ITEM

const char* const TOK_STR[] PROGMEM = {
#define ITEM(x) TOK_STR_##x,
TOKENS
#undef ITEM
};

#undef TOKENS

// Register operand 3-bit encodings
enum Reg {
  REG_B = 0,
  REG_C = 1,
  REG_D = 2,
  REG_E = 3,
  REG_H = 4,
  REG_L = 5,
  REG_M = 6, // (HL), memory at address pointed to by HL
  REG_A = 7,
};

constexpr const char* REG_STR[] = { "B", "C", "D", "E", "H", "L", "(HL)", "A" };

// Register pair operand 2-bit encodings
enum Pair {
  PAIR_BC = 0,
  PAIR_DE = 1,
  PAIR_HL = 2,
  PAIR_SP = 3, // id "SP" is poisoned by AVR macro definition...
  PAIR_AF = 3, // alternate meaning for PUSH/POP
};

constexpr const char* PAIR_STR[] = { "BC", "DE", "HL", "SP" };

// Branch condition 3-bit encodings
// xx- = [Z, C, P/V, S], --x = [clear, set]
enum Cond {
  COND_NZ = 0, //   Z = 0 : non-zero or not equal
  COND_Z  = 1, //   Z = 1 : zero or equal
  COND_NC = 2, //   C = 0 : no overflow or carry clear
  COND_C  = 3, //   C = 1 : unsigned overflow or carry set
  COND_PO = 4, // P/V = 0 : odd parity or no overflow
  COND_PE = 5, // P/V = 1 : even parity or signed overflow
  COND_P  = 6, //   S = 0 : positive or high bit clear
  COND_M  = 7, //   S = 1 : negative or high bit set
};

constexpr const char* COND_STR[] = { "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" };

// ALU operation 3-bit encodings
enum ALU {
  ALU_ADD = 0,
  ALU_ADC = 1, // id "ADC" is poisoned by AVR macro definition...
  ALU_SUB = 2,
  ALU_SBC = 3,
  ALU_AND = 4,
  ALU_XOR = 5,
  ALU_OR  = 6,
  ALU_CP  = 7,
};

constexpr const char* ALU_STR[] = { "ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP" };

// CB prefix bit ops 2-bit encoding
enum CB {
  CB_ROT = 0,
  CB_BIT = 1,
  CB_RES = 2,
  CB_SET = 3,
};

constexpr const char* CB_STR[] = { "", "BIT", "RES", "SET" };

// Rotate/shift operation 3-bit encodings
enum Rot {
  ROT_RLC = 0,
  ROT_RRC = 1,
  ROT_RL  = 2,
  ROT_RR  = 3,
  ROT_SLA = 4,
  ROT_SRA = 5,
  ROT_SLL = 6, // NOTE illegal/undefined opcode; should use SLA instead
  ROT_SRL = 7,
};

constexpr const char* ROT_STR[] = { "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SL1", "SRL" };

// Misc AF ops 3-bit encoding
enum Misc {
  MISC_RLCA = 0,
  MISC_RRCA = 1,
  MISC_RLA = 2,
  MISC_RRA = 3,
  MISC_DAA = 4,
  MISC_CPL = 5,
  MISC_SCF = 6,
  MISC_CCF = 7,
};

constexpr const char* MISC_STR[] = { "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF" };

} // namespace z80
} // namespace uMon
