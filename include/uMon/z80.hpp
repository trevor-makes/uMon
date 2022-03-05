// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

// 8080/Z80 (and even x86!) opcodes are organized by octal groupings
// http://z80.info/decoding.htm is a great resource for decoding these

#pragma once

#include "uMon/format.hpp"
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

constexpr const char* ROT_STR[] = { "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL" };

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

// Print 1-byte immediate following opcode
template <typename API>
void print_imm_byte(uint16_t addr) {
  API::print_char('$');
  fmt_hex8(API::print_char, API::read_byte(addr + 1));
}

// Print 2-byte immediate following opcode
template <typename API>
void print_imm_word(uint16_t addr) {
  API::print_char('$');
  fmt_hex8(API::print_char, API::read_byte(addr + 2));
  fmt_hex8(API::print_char, API::read_byte(addr + 1));
}

// Format displacement following relative jump
template <typename API>
void print_disp(uint16_t addr) {
  API::print_char('$');
  const int8_t disp = API::read_byte(addr + 1);
  fmt_hex16(API::print_char, addr + 2 + disp);
}

// Decode IN/OUT (c): ED [01 --- 00-]
template <typename API>
uint16_t decode_in_out_c(uint16_t addr, uint8_t code) {
  const bool is_out = (code & 01) == 01;
  const uint8_t reg = (code & 070) >> 3;
  API::print_string(is_out ? "OUT (C)," : "IN ");
  // NOTE reg (HL) is undefined; OUT sends 0 and IN sets flags without storing
  API::print_string(reg == 6 ? "?" : REG_STR[reg]);
  if (!is_out) { API::print_string(",(C)"); }
  return addr + 1;
}

// Decode 16-bit ADC/SBC: ED [01 --- 010]
template <typename API>
uint16_t decode_hl_adc(uint16_t addr, uint8_t code) {
  const bool is_adc = (code & 010) == 010;
  const uint8_t pair = (code & 060) >> 4;
  API::print_string(is_adc ? "ADC" : "SBC");
  API::print_string(" HL,");
  API::print_string(PAIR_STR[pair]);
  return addr + 1;
}

// Decode 16-bit LD ind: ED [01 --- 011]
template <typename API>
uint16_t decode_ld_pair_ind(uint16_t addr, uint8_t code) {
  const bool is_load = (code & 010) == 010;
  const uint8_t pair = (code & 060) >> 4;
  API::print_string("LD ");
  if (is_load) {
    API::print_string(PAIR_STR[pair]);
    API::print_char(',');
  }
  API::print_char('(');
  print_imm_word<API>(addr);
  API::print_char(')');
  if (!is_load) {
    API::print_char(',');
    API::print_string(PAIR_STR[pair]);
  }
  return addr + 3;
}

// Decode LD I/R and RRD/RLD: ED [01 --- 111]
template <typename API>
uint16_t decode_ld_ir(uint16_t addr, uint8_t code) {
  const bool is_rot = (code & 040) == 040; // is RRD/RLD
  const bool is_load = (code & 020) == 020; // is LD A,I/R
  const bool is_rl = (code & 010) == 010; // is LD -R- or RLD
  if (is_rot) {
    if (is_load) {
      API::print_char('?');
    } else {
      API::print_string(is_rl ? "RLD" : "RRD");
    }
  } else {
    API::print_string("LD ");
    if (is_load) { API::print_string("A,"); }
    API::print_char(is_rl ? 'R' : 'I');
    if (!is_load) { API::print_string(",A"); }
  }
  return addr + 1;
}

// Decode block transfer ops: ED [10 1-- 0--]
template <typename API>
uint16_t decode_block_ops(uint16_t addr, uint8_t code) {
  static constexpr const char* OPS[] = { "LD", "CP", "IN", "OUT" };
  const uint8_t op = (code & 03);
  const bool is_rep = (code & 020) == 020;
  const bool is_dec = (code & 010) == 010;
  const bool is_ot = is_rep && op == 3;
  API::print_string(is_ot ? "OT" : OPS[op]);
  API::print_char(is_dec ? 'D' : 'I');
  if (is_rep) { API::print_char('R'); }
  return addr + 1;
}

// Disassemble extended opcodes prefixed by $ED
template <typename API>
uint16_t dasm_ed(uint16_t addr) {
  const uint8_t code = API::read_byte(addr);
  if ((code & 0300) == 0100) {
    switch (code & 07) {
    case 0: case 1:
      return decode_in_out_c<API>(addr, code);
    case 2:
      return decode_hl_adc<API>(addr, code);
    case 3:
      return decode_ld_pair_ind<API>(addr, code);
    case 4:
      // NOTE only 0104/0x44 is documented, but the 2nd octal digit is ignored
      API::print_string("NEG");
      return addr + 1;
    case 5:
      // NOTE only 0x45 RETN is documented
      API::print_string(code == 0x4D ? "RETI" : "RETN");
      return addr + 1;
    case 6:
      // NOTE only 0x46, 0x56, 0x5E are documented; '?' sets an undefined mode
      static constexpr const char IM[] = { '0', '?', '1', '2' };
      API::print_string("IM ");
      API::print_char(IM[(code & 030) >> 3]);
      return addr + 1;
    case 7:
      return decode_ld_ir<API>(addr, code);
    }
  } else if ((code & 0344) == 0240) {
    return decode_block_ops<API>(addr, code);
  }
  API::print_char('?');
  return addr + 1;
}

// Disassemble extended opcodes prefixed by $CB
template <typename API>
uint16_t dasm_cb(uint16_t addr) {
  const uint8_t code = API::read_byte(addr);
  const uint8_t op = (code & 0300) >> 6;
  const uint8_t index = (code & 070) >> 3;
  const uint8_t reg = (code & 07);
  // Print opcode
  API::print_string(op == 0 ? ROT_STR[index] : CB_STR[op]);
  API::print_char(' ');
  // Print bit index (only for BIT/RES/SET)
  if (op != 0) {
    API::print_char('0' + index);
    API::print_char(',');
  }
  // Print register operand
  API::print_string(REG_STR[reg]);
  return addr + 1;
}

// Disassemble relative jumps: [00 --- 000]
template <typename API>
uint16_t decode_jr(uint16_t addr, uint8_t code) {
  switch (code & 070) {
  case 000:
    API::print_string("NOP");
    return addr + 1;
  case 010:
    API::print_string("EX AF");
    return addr + 1;
  case 020:
    // DJNZ e
    API::print_string("DJNZ ");
    print_disp<API>(addr);
    return addr + 2;
  case 030:
    // JR e
    API::print_string("JR ");
    print_disp<API>(addr);
    return addr + 2;
  default:
    // JR cc, e
    API::print_string("JR ");
    API::print_string(COND_STR[(code & 030) >> 3]);
    API::print_char(',');
    print_disp<API>(addr);
    return addr + 2;
  }
}

// Disassemble indirect loads: [00 --- 010]
template <typename API>
uint16_t decode_ld_ind(uint16_t addr, uint8_t code) {
  // Decode 070 bitfield
  const bool is_store = (code & 010) == 0; // A/HL is src instead of dst
  const bool use_hl = (code & 060) == 040; // Use HL instead of A
  const bool use_pair = (code & 040) == 0; // Use (BC/DE) instead of (nn)
  API::print_string("LD ");
  // Print A/HL first for loads
  if (!is_store) {
    API::print_string(use_hl ? "HL," : "A,");
  }
  // Print (BC/DE) or (nn)
  API::print_char('(');
  if (use_pair) {
    API::print_string(PAIR_STR[(code & 020) >> 4]);
  } else {
    print_imm_word<API>(addr);
  }
  API::print_char(')');
  // Print A/HL second for stores
  if (is_store) {
    API::print_string(use_hl ? ",HL" : ",A");
  }
  // Opcodes followed by (nn) consume 2 extra bytes
  if (use_pair) {
    return addr + 1;
  } else {
    return addr + 3;
  }
}

// Disassemble INC/DEC: [00 --- 011/100/101]
template <typename API>
uint16_t decode_inc_dec(uint16_t addr, uint8_t code) {
  const bool is_pair = (code & 04) == 0;
  const bool is_inc = is_pair ? (code & 010) == 0 : (code & 01) == 0;
  uint8_t reg = is_pair ? (code & 060) >> 4 : (code & 070) >> 3;
  API::print_string(is_inc ? "INC " : "DEC ");
  API::print_string(is_pair ? PAIR_STR[reg] : REG_STR[reg]);
  return addr + 1;
}

// Disassemble opcodes with leading octal digit 0
template <typename API>
uint16_t dasm_base_lo(uint16_t addr, uint8_t code) {
  switch (code & 07) {
  case 0:
    return decode_jr<API>(addr, code);
  case 1:
    if ((code & 010) == 0) {
      // LD rr, nn
      API::print_string("LD ");
      API::print_string(PAIR_STR[(code & 060) >> 4]);
      API::print_char(',');
      print_imm_word<API>(addr);
      return addr + 3;
    } else {
      // ADD HL, rr
      API::print_string("ADD HL,");
      API::print_string(PAIR_STR[(code & 060) >> 4]);
      return addr + 1;
    }
  case 2:
    return decode_ld_ind<API>(addr, code);
  case 6:
    // LD r, n
    API::print_string("LD ");
    API::print_string(REG_STR[(code & 070) >> 3]);
    API::print_char(',');
    print_imm_byte<API>(addr);
    return addr + 2;
  case 7:
    // Misc AF ops with no operands
    API::print_string(MISC_STR[(code & 070) >> 3]);
    return addr + 1;
  default:
    return decode_inc_dec<API>(addr, code);
  }
}

// Disassemble opcodes with leading octal digit 3
template <typename API>
uint16_t dasm_base_hi(uint16_t addr, uint8_t code) {
  switch (code & 07) {
  case 0:
    // RET cc
    API::print_string("RET ");
    API::print_string(COND_STR[(code & 070) >> 3]);
    return addr + 1;
  case 1:
    if ((code & 010) == 0) {
      API::print_string("POP ");
      API::print_string(PAIR_ALT_STR[(code & 060) >> 4]);
      return addr + 1;
    } else {
      // TODO JP/LD will need to handle DD/FD prefix
      static constexpr const char* OPS[] = { "RET", "EXX", "JP (HL)", "LD SP,HL" };
      API::print_string(OPS[(code & 060) >> 4]);
      return addr + 1;
    }
  case 2:
    // JP cc, nn
    API::print_string("JP ");
    API::print_string(COND_STR[(code & 070) >> 3]);
    API::print_char(',');
    print_imm_word<API>(addr);
    return addr + 3;
  case 3:
    switch (code & 070) {
    case 000:
      // JP (nn)
      API::print_string("JP ");
      print_imm_word<API>(addr);
      return addr + 3;
    case 010:
      // $CB prefix currently handled elsewhere
      break;
    case 020:
      // OUT (n),A
      API::print_string("OUT (");
      print_imm_byte<API>(addr);
      API::print_string("),A");
      return addr + 2;
    case 030:
      // IN A,(n)
      API::print_string("IN A,(");
      print_imm_byte<API>(addr);
      API::print_char(')');
      return addr + 2;
    case 040:
      API::print_string("EX (SP),HL");
      return addr + 1;
    case 050:
      API::print_string("EX DE,HL");
      return addr + 1;
    case 060:
      API::print_string("DI");
      return addr + 1;
    case 070:
      API::print_string("EI");
      return addr + 1;
    }
    break;
  case 4:
    // CALL cc, nn
    API::print_string("CALL ");
    API::print_string(COND_STR[(code & 070) >> 3]);
    API::print_char(',');
    print_imm_word<API>(addr);
    return addr + 3;
  case 5:
    if ((code & 010) == 0) {
      API::print_string("PUSH ");
      API::print_string(PAIR_ALT_STR[(code & 060) >> 4]);
      return addr + 1;
    } else {
      // NOTE $DD, $ED, $FD should have already been handled
      API::print_string("CALL ");
      print_imm_word<API>(addr);
      return addr + 3;
    }
  case 6:
    // [ALU op] A, n
    API::print_string(ALU_STR[(code & 070) >> 3]);
    API::print_string(" A,");
    print_imm_byte<API>(addr);
    return addr + 2;
  case 7:
    // RST zp
    API::print_string("RST $");
    fmt_hex8(API::print_char, code & 070);
    return addr + 1;
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
  case 0xED:
    return dasm_ed<API>(addr + 1);
  case 0xDD:
  case 0xFD:
    // TODO handle IX/IY prefix bytes
    return addr + 1;
  }
  switch (code & 0300) {
  case 0000:
    return dasm_base_lo<API>(addr, code);
  case 0100:
    // LD r, r
    API::print_string("LD ");
    API::print_string(REG_STR[(code & 070) >> 3]);
    API::print_char(',');
    API::print_string(REG_STR[(code & 07)]);
    return addr + 1;
  case 0200:
    // [ALU op] A, r
    API::print_string(ALU_STR[(code & 070) >> 3]);
    API::print_string(" A,");
    API::print_string(REG_STR[(code & 07)]);
    return addr + 1;
  case 0300:
    return dasm_base_hi<API>(addr, code);
  }
  return addr + 1;
}

template <typename API>
uint16_t impl_dasm(uint16_t addr, uint16_t end) {
  for (;;) {
    // Print "addr:  opcode"
    fmt_hex16(API::print_char, addr);
    API::print_string(":  ");
    uint16_t next = dasm_base<API>(addr);
    API::print_char('\n');
    // Do while end does not overlap with opcode
    if (uint16_t(end - addr) < uint16_t(next - addr)) {
      return next;
    }
    addr = next;
  }
}

template <typename API>
void cmd_dasm(uCLI::Args args) {
  uint16_t start = args.has_next() ? parse_u32(args.next()) : 0;
  uint16_t size = args.has_next() ? parse_u32(args.next()) : 1;
  uint16_t next = impl_dasm<API>(start, start + size - 1);
  set_prompt<API>(args.command(), next);
}

} // namespace z80
} // namespace uMon
