// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "uMon/z80/common.hpp"
#include "uMon/format.hpp"
#include "uCLI.hpp"

#include <stdint.h>

namespace uMon {
namespace z80 {

template <typename API>
void print_operand_error(Operand& op) {
  print_operand<API>(op);
  API::print_string("?\n");
}

template <typename T, uint8_t N>
uint8_t index_of(const T (&table)[N], T value) {
  for (uint8_t i = 0; i < N; ++i) {
    if (table[i] == value) {
      return i;
    }
  }
  return N;
}

uint8_t token_to_reg(uint8_t token, uint8_t prefix = 0) {
  if (prefix == PREFIX_IX) {
    switch (token) {
    case TOK_IXH: return REG_H;
    case TOK_IXL: return REG_L;
    case TOK_IX | TOK_INDIRECT: return REG_M;
    case TOK_H: case TOK_L: return REG_INVALID;
    }
  } else if (prefix == PREFIX_IY) {
    switch (token) {
    case TOK_IYH: return REG_H;
    case TOK_IYL: return REG_L;
    case TOK_IY | TOK_INDIRECT: return REG_M;
    case TOK_H: case TOK_L: return REG_INVALID;
    }
  }
  return index_of(REG_TOK, token);
}

uint8_t token_to_pair(uint8_t token, uint8_t prefix = 0, bool use_af = false) {
  if (prefix == PREFIX_IX) {
    if (token == TOK_IX) return PAIR_HL;
    if (token == TOK_HL) return PAIR_INVALID;
  } else if (prefix == PREFIX_IY) {
    if (token == TOK_IY) return PAIR_HL;
    if (token == TOK_HL) return PAIR_INVALID;
  }
  if (use_af) {
    if (token == TOK_AF) return PAIR_SP;
    if (token == TOK_SP) return PAIR_INVALID;
  }
  return index_of(PAIR_TOK, token);
}

uint8_t token_to_cond(uint8_t token) {
  return index_of(COND_TOK, token);
}

uint8_t token_to_prefix(uint8_t token) {
  switch (token & TOK_MASK) {
  case TOK_IX: case TOK_IXH: case TOK_IXL:
    return PREFIX_IX;
  case TOK_IY: case TOK_IYH: case TOK_IYL:
    return PREFIX_IY;
  default:
    return 0;
  }
}

template <typename API>
uint8_t encode_alu_a(uint16_t addr, uint8_t mne, Operand& src) {
  uint8_t alu = index_of(ALU_MNE, mne);
  if (src.token == TOK_INTEGER) {
    API::write_byte(addr, 0306 | (alu << 3));
    API::write_byte(addr + 1, src.value);
    return 2;
  } else {
    // Validate source operand is register
    uint8_t prefix = token_to_prefix(src.token);
    uint8_t reg = token_to_reg(src.token, prefix);
    uint8_t code = 0200 | (alu << 3) | reg;
    if (reg == REG_INVALID) {
      print_operand_error<API>(src);
      return 0;
    } else if (prefix != 0) {
      // Write [prefix, code, displacement]
      API::write_byte(addr, prefix);
      API::write_byte(addr + 1, code);
      API::write_byte(addr + 2, src.value);
      return 3;
    } else {
      API::write_byte(addr, code);
      return 1;
    }
  }
}

template <typename API>
uint8_t encode_alu_hl(uint16_t addr, uint8_t mne, Operand& dst, Operand& src) {
  // Validate dst operand is HL/IX/IY pair
  uint8_t prefix = token_to_prefix(dst.token);
  if (token_to_pair(dst.token, prefix) != PAIR_HL) {
    print_operand_error<API>(dst);
    return 0;
  }
  // Make sure src and dst prefixes match (can't mix HL/IX/IY)
  uint8_t src_prefix = token_to_prefix(src.token);
  if (src_prefix != 0 && src_prefix != prefix) {
    print_operand_error<API>(src);
    return 0;
  }
  // Validate src operand is pair
  uint8_t src_pair = token_to_pair(src.token, prefix);
  if (src_pair == PAIR_INVALID) {
    print_operand_error<API>(src);
    return 0;
  }
  // Handle ADD, ADC, SBC
  if (mne == MNE_ADD) {
    if (prefix != 0) API::write_byte(addr++, prefix);
    API::write_byte(addr, 0011 | (src_pair << 4));
    return prefix != 0 ? 2 : 1;
  } else if (prefix == 0 && (mne == MNE_ADC || mne == MNE_SBC)) {
    uint8_t code = mne == MNE_ADC ? 0112 : 0102;
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, code | (src_pair << 4));
    return 2;
  } else {
    print_operand_error<API>(dst);
    return 0;
  }
}

template <typename API>
uint8_t encode_ex(uint16_t addr, Operand& op1, Operand& op2) {
  if (op1.token == (TOK_SP | TOK_INDIRECT)) {
    uint8_t prefix = token_to_prefix(op2.token);
    if (token_to_pair(op2.token, prefix) != PAIR_HL) {
      print_operand_error<API>(op2);
      return 0;
    }
    if (prefix != 0) API::write_byte(addr++, prefix);
    API::write_byte(addr, 0xE3);
    return prefix != 0 ? 2 : 1;
  } else if (op1.token == TOK_DE && op2.token == TOK_HL) {
    API::write_byte(addr, 0xEB);
    return 1;
  } else if (op1.token == TOK_AF && (op2.token == TOK_AF || op2.token == TOK_INVALID)) {
    API::write_byte(addr, 0x08);
    return 1;
  } else {
    print_operand_error<API>(op1);
    return 0;
  }
}

template <typename API>
uint8_t impl_asm(uint16_t addr, Instruction inst) {
  Operand& op1 = inst.operands[0];
  Operand& op2 = inst.operands[1];
  switch (inst.mnemonic) {
  case MNE_ADC:
  case MNE_ADD:
  case MNE_AND:
  case MNE_CP:
  case MNE_OR:
  case MNE_SBC:
  case MNE_SUB:
  case MNE_XOR:
    if (op2.token == TOK_INVALID) {
      return encode_alu_a<API>(addr, inst.mnemonic, op1);
    } else if (op1.token == TOK_A) {
      return encode_alu_a<API>(addr, inst.mnemonic, op2);
    } else {
      return encode_alu_hl<API>(addr, inst.mnemonic, op1, op2);
    }
  case MNE_CCF:
    API::write_byte(addr, 0x3F);
    return 1;
  case MNE_CPD:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA9);
    return 2;
  case MNE_CPDR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xB9);
    return 2;
  case MNE_CPI:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA1);
    return 2;
  case MNE_CPIR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xB1);
    return 2;
  case MNE_CPL:
    API::write_byte(addr, 0x2F);
    return 1;
  case MNE_DAA:
    API::write_byte(addr, 0x27);
    return 1;
  case MNE_DI:
    API::write_byte(addr, 0xF3);
    return 1;
  case MNE_DJNZ:
    if (op1.token == TOK_INTEGER) {
      API::write_byte(addr, 0x10);
      // TODO validate if range in [-128, 127]
      API::write_byte(addr + 1, op1.value - (addr + 2));
      return 2;
    }
    break;
  case MNE_EI:
    API::write_byte(addr, 0xFB);
    return 1;
  case MNE_EX:
    return encode_ex<API>(addr, op1, op2);
  case MNE_EXX:
    API::write_byte(addr, 0xD9);
    return 1;
  case MNE_HALT:
    API::write_byte(addr, 0x76);
    return 1;
  case MNE_IM:
    if (op1.token == TOK_INTEGER && op1.value < 3) {
      static constexpr const uint8_t IM[] = { 0x46, 0x56, 0x5E };
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, IM[op1.value]);
      return 2;
    } else if (op1.token == TOK_UNDEFINED) {
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, 0x4E);
      return 2;
    } else {
      print_operand_error<API>(op1);
      return 0;
    }
  case MNE_IND:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xAA);
    return 2;
  case MNE_INDR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xBA);
    return 2;
  case MNE_INI:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA2);
    return 2;
  case MNE_INIR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xB2);
    return 2;
  case MNE_LDD:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA8);
    return 2;
  case MNE_LDDR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xB8);
    return 2;
  case MNE_LDI:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA0);
    return 2;
  case MNE_LDIR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xB0);
    return 2;
  case MNE_NEG:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0x44);
    return 2;
  case MNE_NOP:
    API::write_byte(addr, 0x00);
    return 1;
  case MNE_OTDR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xBB);
    return 2;
  case MNE_OTIR:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xB3);
    return 2;
  case MNE_OUTD:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xAB);
    return 2;
  case MNE_OUTI:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA3);
    return 2;
  case MNE_RETI:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0x4D);
    return 2;
  case MNE_RETN:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0x45);
    return 2;
  case MNE_RLA:
    API::write_byte(addr, 0x17);
    return 1;
  case MNE_RLCA:
    API::write_byte(addr, 0x07);
    return 1;
  case MNE_RLD:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0x6F);
    return 2;
  case MNE_RRA:
    API::write_byte(addr, 0x1F);
    return 1;
  case MNE_RRCA:
    API::write_byte(addr, 0x0F);
    return 1;
  case MNE_RRD:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0x67);
    return 2;
  case MNE_RST:
    if (op1.token == TOK_INTEGER && (op1.value & 0307) == 0) {
      API::write_byte(addr, 0307 | op1.value);
      return 1;
    } else {
      print_operand_error<API>(op1);
      return 0;
    }
  case MNE_SCF:
    API::write_byte(addr, 0x37);
    return 1;
  }
  return 0;
}

} // namespace z80
} // namespace uMon
