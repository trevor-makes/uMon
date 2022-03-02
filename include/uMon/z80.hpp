// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

// 8080/Z80 (and even x86!) opcodes are organized by octal groupings
// http://z80.info/decoding.htm is a great resource for decoding these

#pragma once

#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {
namespace z80 {

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
constexpr const char* PAIR_ALT_STR[] = { "BC", "DE", "HL", "AF" };

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

constexpr const char* ROT_STR[] = { "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL" };

// Format 1-byte immediate following opcode
template <typename API>
void fmt_imm(uint16_t addr) {
  API::print_char('$');
  fmt_hex8(API::print_char, API::read_byte(addr + 1));
}

// Format 2-byte immediate following opcode
template <typename API>
void fmt_imm2(uint16_t addr) {
  API::print_char('$');
  fmt_hex8(API::print_char, API::read_byte(addr + 2));
  fmt_hex8(API::print_char, API::read_byte(addr + 1));
}

// Format displacement following relative jump
template <typename API>
void fmt_disp(uint16_t addr) {
  API::print_char('$');
  int8_t disp = API::read_byte(addr + 1);
  fmt_hex16(API::print_char, addr + 2 + disp);
}

// Disassemble extended opcodes prefixed by $CB
template <typename API>
uint16_t dasm_cb(uint16_t addr) {
  uint8_t code = API::read_byte(addr);
  switch (code & 0300) {
  case 0000:
    API::print_string(ROT_STR[(code & 070) >> 3]);
    API::print_char(' ');
    API::print_string(REG_STR[(code & 07)]);
    return addr + 1;
  case 0100:
    API::print_string("BIT ");
    break;
  case 0200:
    API::print_string("RES ");
    break;
  case 0300:
    API::print_string("SET ");
    break;
  }
  API::print_char('0' + ((code & 070) >> 3));
  API::print_char(',');
  API::print_string(REG_STR[(code & 07)]);
  return addr + 1;
}

// Disassemble opcodes with leading octal digit 0
template <typename API>
uint16_t dasm_lo(uint16_t addr, uint8_t code) {
  switch (code & 07) {
  case 0:
    if ((code & 040) == 0) {
      switch (code & 030) {
      case 000:
        API::print_string("NOP");
        return addr + 1;
      case 010:
        API::print_string("EX AF");
        return addr + 1;
      case 020:
        API::print_string("DJNZ ");
        fmt_disp<API>(addr);
        return addr + 2;
      case 030:
        API::print_string("JR ");
        fmt_disp<API>(addr);
        return addr + 2;
      }
    } else {
      API::print_string("JR ");
      API::print_string(COND_STR[(code & 030) >> 3]);
      API::print_char(',');
      fmt_disp<API>(addr);
      return addr + 2;
    }
  case 1:
    if ((code & 010) == 0) {
      API::print_string("LD ");
      API::print_string(PAIR_STR[(code & 060) >> 4]);
      API::print_char(',');
      fmt_imm2<API>(addr);
      return addr + 3;
    } else {
      API::print_string("ADD HL,");
      API::print_string(PAIR_STR[(code & 060) >> 4]);
      return addr + 1;
    }
  case 2:
    break;
  case 3:
    if ((code & 010) == 0) {
      API::print_string("INC ");
    } else {
      API::print_string("DEC ");
    }
    API::print_string(PAIR_STR[(code & 060) >> 4]);
    return addr + 1;
  case 4:
    API::print_string("INC ");
    API::print_string(REG_STR[(code & 070) >> 3]);
    return addr + 1;
  case 5:
    API::print_string("DEC ");
    API::print_string(REG_STR[(code & 070) >> 3]);
    return addr + 1;
  case 6:
    API::print_string("LD ");
    API::print_string(REG_STR[(code & 070) >> 3]);
    API::print_char(',');
    fmt_imm<API>(addr);
    return addr + 2;
  case 7:
    break;
  }
  return addr + 1;
}

// Disassemble opcodes with leading octal digit 3
template <typename API>
uint16_t dasm_hi(uint16_t addr, uint8_t code) {
  switch (code & 07) {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  case 4:
    break;
  case 5:
    break;
  case 6:
    break;
  case 7:
    break;
  }
  return addr + 1;
}

template <typename API>
uint16_t dasm_base(uint16_t addr) {
  uint8_t code = API::read_byte(addr);
  switch (code) {
  case 0x76:
    API::print_string("HALT");
    return addr + 1;
  case 0xCB:
    return dasm_cb<API>(addr + 1);
  case 0xDB:
  case 0xEB:
  case 0xFB:
    // TODO handle prefix bytes
    return addr + 1;
  }
  switch (code & 0300) {
  case 0000:
    return dasm_lo<API>(addr, code);
  case 0100:
    API::print_string("LD ");
    API::print_string(REG_STR[(code & 070) >> 3]);
    API::print_char(',');
    API::print_string(REG_STR[(code & 07)]);
    return addr + 1;
  case 0200:
    API::print_string(ALU_STR[(code & 070) >> 3]);
    API::print_string(" A,");
    API::print_string(REG_STR[(code & 07)]);
    return addr + 1;
  case 0300:
    return dasm_hi<API>(addr, code);
  }
  return addr + 1;
}

template <typename API>
void impl_dasm(uint16_t addr, uint16_t end) {
  for (;;) {
    uint16_t next = dasm_base<API>(addr);
    API::print_char('\n');
    // Do while end does not overlap with opcode
    if (uint16_t(end - addr) < uint16_t(next - addr)) break;
    addr = next;
  }
}

template <typename API>
void cmd_dasm(uCLI::Tokens args) {
  uint16_t start = args.has_next() ? parse_u32(args.next()) : 0;
  uint16_t size = args.has_next() ? parse_u32(args.next()) : 1;
  impl_dasm<API>(start, start + size - 1);
}

} // namespace z80
} // namespace uMon
