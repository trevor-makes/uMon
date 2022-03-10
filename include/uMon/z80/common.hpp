// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

namespace uMon {
namespace z80 {

enum Mnemonic {
  MNE_ADC,
  MNE_ADD,
  MNE_AND,
  MNE_BIT,
  MNE_CALL,
  MNE_CCF,
  MNE_CP,
  MNE_CPD,
  MNE_CPDR,
  MNE_CPI,
  MNE_CPIR,
  MNE_CPL,
  MNE_DAA,
  MNE_DEC,
  MNE_DI,
  MNE_DJNZ,
  MNE_EI,
  MNE_EX,
  MNE_EXX,
  MNE_HALT,
  MNE_IM,
  MNE_IN,
  MNE_INC,
  MNE_IND,
  MNE_INDR,
  MNE_INI,
  MNE_INIR,
  MNE_JP,
  MNE_JR,
  MNE_LD,
  MNE_LDD,
  MNE_LDDR,
  MNE_LDI,
  MNE_LDIR,
  MNE_NEG,
  MNE_NOP,
  MNE_OR,
  MNE_OTDR,
  MNE_OTIR,
  MNE_OUT,
  MNE_OUTD,
  MNE_OUTI,
  MNE_POP,
  MNE_PUSH,
  MNE_RES,
  MNE_RET,
  MNE_RETI,
  MNE_RETN,
  MNE_RL,
  MNE_RLA,
  MNE_RLC,
  MNE_RLCA,
  MNE_RLD,
  MNE_RR,
  MNE_RRA,
  MNE_RRC,
  MNE_RRCA,
  MNE_RRD,
  MNE_RST,
  MNE_SBC,
  MNE_SCF,
  MNE_SET,
  MNE_SL1, // Undocumented! Alt: SLL
  MNE_SLA,
  MNE_SRA,
  MNE_SRL,
  MNE_SUB,
  MNE_XOR,
  MNE_INVALID,
};

constexpr const char* MNE_STR[] = {
  "ADC",
  "ADD",
  "AND",
  "BIT",
  "CALL",
  "CCF",
  "CP",
  "CPD",
  "CPDR",
  "CPI",
  "CPIR",
  "CPL",
  "DAA",
  "DEC",
  "DI",
  "DJNZ",
  "EI",
  "EX",
  "EXX",
  "HALT",
  "IM",
  "IN",
  "INC",
  "IND",
  "INDR",
  "INI",
  "INIR",
  "JP",
  "JR",
  "LD",
  "LDD",
  "LDDR",
  "LDI",
  "LDIR",
  "NEG",
  "NOP",
  "OR",
  "OTDR",
  "OTIR",
  "OUT",
  "OUTD",
  "OUTI",
  "POP",
  "PUSH",
  "RES",
  "RET",
  "RETI",
  "RETN",
  "RL",
  "RLA",
  "RLC",
  "RLCA",
  "RLD",
  "RR",
  "RRA",
  "RRC",
  "RRCA",
  "RRD",
  "RST",
  "SBC",
  "SCF",
  "SET",
  "SL1", // Undocumented! Alt: SLL
  "SLA",
  "SRA",
  "SRL",
  "SUB",
  "XOR",
};

// All registers, pairs, and conditions
enum Token {
  TOK_A,
  TOK_AF,
  TOK_B,
  TOK_BC,
  TOK_C, // REG_C or COND_C
  TOK_D,
  TOK_DE,
  TOK_E,
  TOK_H,
  TOK_HL,
  TOK_I,
  TOK_IX,
  TOK_IXH,
  TOK_IXL,
  TOK_IY,
  TOK_IYH,
  TOK_IYL,
  TOK_L,
  TOK_M,
  TOK_NC,
  TOK_NZ,
  TOK_P,
  TOK_PE,
  TOK_PO,
  TOK_R,
  TOK_SP,
  TOK_Z,
  TOK_INVALID,
  TOK_INTEGER,
  TOK_INDIRECT = 0x80,
};

constexpr const char* TOK_STR[] = {
  "A",
  "AF",
  "B",
  "BC",
  "C", // REG_C or COND_C
  "D",
  "DE",
  "E",
  "H",
  "HL",
  "I",
  "IX",
  "IXH",
  "IXL",
  "IY",
  "IYH",
  "IYL",
  "L",
  "M",
  "NC",
  "NZ",
  "P",
  "PE",
  "PO",
  "R",
  "SP",
  "Z",
};

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
