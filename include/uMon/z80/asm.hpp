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

template <typename API>
void write_op_word(uint16_t addr, uint8_t code, uint16_t value) {
  API::write_byte(addr, code);
  API::write_byte(addr + 1, value & 0xFF);
  API::write_byte(addr + 2, value >> 8);
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
  switch (prefix) {
  case PREFIX_IX: return index_of(REG_TOK_IX, token);
  case PREFIX_IY: return index_of(REG_TOK_IY, token);
  default: return index_of(REG_TOK, token);
  }
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
  // NOTE mne assumed to be valid
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
      if (reg == REG_M) {
        API::write_byte(addr + 2, src.value);
        return 3;
      } else {
        return 2;
      }
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
uint8_t encode_alu(uint16_t addr, uint8_t mne, Operand& op1, Operand& op2) {
  if (op2.token == TOK_INVALID) {
    return encode_alu_a<API>(addr, mne, op1);
  } else if (op1.token == TOK_A) {
    return encode_alu_a<API>(addr, mne, op2);
  } else {
    return encode_alu_hl<API>(addr, mne, op1, op2);
  }
}

template <typename API>
uint8_t encode_cb(uint16_t addr, uint8_t code, Operand& op) {
  // Validate register
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t reg = token_to_reg(op.token, prefix);
  if (reg == REG_INVALID || (prefix != 0 && reg != REG_M)) {
    print_operand_error<API>(op);
    return 0;
  }
  if (prefix != 0) {
    API::write_byte(addr, prefix);
    API::write_byte(addr + 1, PREFIX_CB);
    API::write_byte(addr + 2, op.value);
    API::write_byte(addr + 3, code | reg);
    return 4;
  } else {
    API::write_byte(addr, PREFIX_CB);
    API::write_byte(addr + 1, code | reg);
    return 2;
  }
}

template <typename API>
uint8_t encode_rot(uint16_t addr, uint8_t mne, Operand& op) {
  // NOTE mne assumed to be valid
  uint8_t code = index_of(ROT_MNE, mne) << 3;
  return encode_cb<API>(addr, code, op);
}

// Encode CB-prefix BIT, RES, and SET ops
template <typename API>
uint8_t encode_bit(uint16_t addr, uint8_t mne, Operand& op1, Operand& op2) {
  // Validate bit index
  if (op1.token != TOK_INTEGER || op1.value > 7) {
    print_operand_error<API>(op1);
    return 0;
  }
  uint8_t bit = op1.value << 3;
  // NOTE mne assumed to be valid
  uint8_t op = index_of(CB_MNE, mne) << 6;
  uint8_t code = op | bit;
  return encode_cb<API>(addr, code, op2);
}

template <typename API>
uint8_t encode_call_jp(uint16_t addr, bool is_call, Operand& op1, Operand& op2) {
  if (op2.token == TOK_INTEGER) {
    uint8_t cond = token_to_cond(op1.token);
    if (cond != COND_INVALID) {
      uint8_t code = (is_call ? 0304 : 0302) | (cond << 3);
      write_op_word<API>(addr, code, op2.value);
      return 3;
    }
  } else if (op2.token == TOK_INVALID) {
    if (op1.token == TOK_INTEGER) {
      uint8_t code = is_call ? 0315 : 0303;
      write_op_word<API>(addr, code, op1.value);
      return 3;
    } else if (!is_call) { // JP only
      uint8_t prefix = token_to_prefix(op1.token);
      uint8_t reg = token_to_reg(op1.token, prefix);
      if (reg == REG_M) {
        if (prefix != 0) API::write_byte(addr++, prefix);
        API::write_byte(addr, 0xE9); // JP (HL)
        return prefix != 0 ? 2 : 1;
      }
    }
  }
  print_operand_error<API>(op1);
  return 0;
}

template <typename API>
uint8_t encode_inc_dec(uint16_t addr, bool is_inc, Operand& op) {
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t reg = token_to_reg(op.token, prefix);
  uint8_t pair = token_to_pair(op.token, prefix);
  if (reg != REG_INVALID) {
    uint8_t code = is_inc ? 0004 : 0005;
    if (prefix != 0) {
      API::write_byte(addr, prefix);
      API::write_byte(addr + 1, code | (reg << 3));
      if (reg == REG_M) {
        API::write_byte(addr + 2, op.value);
        return 3;
      } else {
        return 2;
      }
    } else {
      API::write_byte(addr, code | (reg << 3));
      return 1;
    }
  } else if (pair != PAIR_INVALID) {
    if (prefix != 0) API::write_byte(addr++, prefix);
    uint8_t code = is_inc ? 0003 : 0013;
    API::write_byte(addr, code | (pair << 4));
    return prefix != 0 ? 2 : 1;
  } else {
    print_operand_error<API>(op);
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
uint8_t encode_im(uint16_t addr, Operand& op) {
  if (op.token == TOK_INTEGER && op.value < 3) {
    static constexpr const uint8_t IM[] = { 0x46, 0x56, 0x5E };
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, IM[op.value]);
    return 2;
  } else if (op.token == TOK_UNDEFINED) {
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0x4E);
    return 2;
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

template <typename API>
uint8_t encode_in_out(uint16_t addr, bool is_in, Operand& data, Operand& port) {
  if (data.token == TOK_A && port.token == (TOK_INTEGER | TOK_INDIRECT)) {
    API::write_byte(addr, is_in ? 0333 : 0323);
    API::write_byte(addr + 1, port.value);
    return 2;
  } else if (port.token == (TOK_C | TOK_INDIRECT)) {
    uint8_t reg = token_to_reg(data.token);
    if (reg == REG_INVALID || reg == REG_M) {
      print_operand_error<API>(data);
      return 0;
    }
    uint8_t code = (is_in ? 0100 : 0101) | reg << 3;
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, code);
    return 2;
  } else {
    print_operand_error<API>(port);
    return 0;
  }
}

template <typename API>
uint8_t encode_djnz_jr(uint16_t addr, uint8_t code, Operand& op) {
  int16_t disp = op.value - (addr + 2);
  if (op.token == TOK_INTEGER && disp >= -128 && disp <= 127) {
    API::write_byte(addr, code);
    API::write_byte(addr + 1, disp);
    return 2;
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

template <typename API>
uint8_t encode_jr(uint16_t addr, Operand& op1, Operand& op2) {
  if (op2.token == TOK_INVALID) {
    return encode_djnz_jr<API>(addr, 0x18, op1);
  } else {
    uint8_t cond = token_to_cond(op1.token);
    if (cond > 3) {
      print_operand_error<API>(op1);
      return 0;
    }
    uint8_t code = 0040 | cond << 3;
    return encode_djnz_jr<API>(addr, code, op2);
  }
}

template <typename API>
uint8_t encode_ld(uint16_t addr, Operand& dst, Operand& src) {
  // Special cases for destination A
  if (dst.token == TOK_A) {
    switch (src.token) {
    case TOK_I:
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, 0x57); // LD A,I
      return 2;
    case TOK_R:
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, 0x5F); // LD A,R
      return 2;
    case TOK_BC | TOK_INDIRECT:
      API::write_byte(addr, 0x0A); // LD A,(BC)
      return 1;
    case TOK_DE | TOK_INDIRECT:
      API::write_byte(addr, 0x1A); // LD A,(DE)
      return 1;
    case TOK_INTEGER | TOK_INDIRECT:
      API::write_byte(addr, 0x3A); // LD A,(nn)
      API::write_byte(addr + 1, src.value & 0xFF);
      API::write_byte(addr + 2, src.value >> 8);
      return 3;
    }
  }

  // Special cases for source A
  if (src.token == TOK_A) {
    switch (dst.token) {
    case TOK_I:
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, 0x47); // LD I,A
      return 2;
    case TOK_R:
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, 0x4F); // LD R,A
      return 2;
    case TOK_BC | TOK_INDIRECT:
      API::write_byte(addr, 0x02); // LD (BC),A
      return 1;
    case TOK_DE | TOK_INDIRECT:
      API::write_byte(addr, 0x12); // LD (DE),A
      return 1;
    case TOK_INTEGER | TOK_INDIRECT:
      API::write_byte(addr, 0x32); // LD (nn),A
      API::write_byte(addr + 1, dst.value & 0xFF);
      API::write_byte(addr + 2, dst.value >> 8);
      return 3;
    }
  }

  // Special cases for destination HL/IX/IY
  uint8_t dst_prefix = token_to_prefix(dst.token);
  uint8_t dst_pair = token_to_pair(dst.token, dst_prefix);
  if (dst_pair == PAIR_HL) {
    // Destination is HL/IX/IY
    uint8_t& prefix = dst_prefix;
    bool has_prefix = prefix != 0;
    if (src.token == (TOK_INTEGER | TOK_INDIRECT)) {
      // Source is address of 2-byte integer
      if (has_prefix) API::write_byte(addr++, prefix);
      API::write_byte(addr, 0x2A); // LD HL,(nn)
      API::write_byte(addr + 1, src.value & 0xFF);
      API::write_byte(addr + 2, src.value >> 8);
      return has_prefix ? 4 : 3;
    }
  }

  // Special cases for source HL/IX/IY
  uint8_t src_prefix = token_to_prefix(src.token);
  uint8_t src_pair = token_to_pair(src.token, src_prefix);
  if (src_pair == PAIR_HL) {
    // Source is HL/IX/IY
    uint8_t& prefix = src_prefix;
    bool has_prefix = prefix != 0;
    if (dst.token == (TOK_INTEGER | TOK_INDIRECT)) {
      // Desination is address of 2-byte integer
      if (has_prefix) API::write_byte(addr++, prefix);
      API::write_byte(addr, 0x22); // LD (nn),HL
      API::write_byte(addr + 1, dst.value & 0xFF);
      API::write_byte(addr + 2, dst.value >> 8);
      return has_prefix ? 4 : 3;
    } else if (dst.token == TOK_SP) {
      // Destination is SP
      if (has_prefix) API::write_byte(addr++, prefix);
      API::write_byte(addr, 0xF9); // LD SP,HL
      return has_prefix ? 2 : 1;
    }
  }

  // Catch-all cases for any register/pair
  uint8_t dst_reg = token_to_reg(dst.token, dst_prefix);
  if (dst_reg != REG_INVALID) {
    // Destination is any primary register
    uint8_t src_reg = token_to_reg(src.token, src_prefix);
    if (src_reg != REG_INVALID) {
      // Source is any primary register
      if (!(src_reg == REG_M && dst_prefix != 0)
        && !(dst_reg == REG_M && src_prefix != 0)
        && !(src_reg == REG_M && dst_reg == REG_M)
        && (src_prefix ^ dst_prefix) != (PREFIX_IX ^ PREFIX_IY)
        ) {
        uint8_t prefix = dst_prefix != 0 ? dst_prefix : src_prefix;
        bool has_prefix = prefix != 0;
        bool has_index = has_prefix && (dst_reg == REG_M || src_reg == REG_M);
        uint8_t disp = dst_reg == REG_M ? dst.value : src.value;
        uint8_t code = 0100 | dst_reg << 3 | src_reg;
        if (has_prefix) API::write_byte(addr++, prefix);
        API::write_byte(addr, code); // LD r,r
        if (has_index) API::write_byte(addr + 1, disp);
        return has_index ? 3 : has_prefix ? 2 : 1;
      }
    } else if (src.token == TOK_INTEGER) {
      // Source is 1-byte integer
      uint8_t& prefix = dst_prefix;
      bool has_prefix = prefix != 0;
      bool has_index = has_prefix && dst_reg == REG_M;
      uint8_t code = 0006 | dst_reg << 3;
      if (has_prefix) API::write_byte(addr++, prefix);
      API::write_byte(addr, code); // LD r,n
      if (has_index) API::write_byte(++addr, dst.value);
      API::write_byte(addr + 1, src.value & 0xFF);
      return has_index ? 4 : has_prefix ? 3 : 2;
    }
  } else if (dst_pair != PAIR_INVALID) {
    // Destination is any register pair
    uint8_t& prefix = dst_prefix;
    bool has_prefix = prefix != 0;
    if (src.token == TOK_INTEGER) {
      // Source is 2-byte integer
      uint8_t code = 0001 | dst_pair << 4;
      if (has_prefix) API::write_byte(addr++, prefix);
      API::write_byte(addr, code); // LD rr,nn
      API::write_byte(addr + 1, src.value & 0xFF);
      API::write_byte(addr + 2, src.value >> 8);
      return has_prefix ? 4 : 3;
    } else if (src.token == (TOK_INTEGER | TOK_INDIRECT)) {
      // Source is address of 2-byte integer
      // NOTE LD HL/IX/IY,(nn) already handled by special case; only BC/DE/SP
      uint8_t code = 0113 | dst_pair << 4;
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, code); // LD rr,(nn)
      API::write_byte(addr + 2, src.value & 0xFF);
      API::write_byte(addr + 3, src.value >> 8);
      return 4;
    }
  } else if (src_pair != PAIR_INVALID) {
    // Source is any register pair
    if (dst.token == (TOK_INTEGER | TOK_INDIRECT)) {
      // Destination is address of 2-byte integer
      // NOTE LD (nn),HL/IX/IY already handled by special case; only BC/DE/SP
      uint8_t code = 0103 | src_pair << 4;
      API::write_byte(addr, PREFIX_ED);
      API::write_byte(addr + 1, code); // LD (nn),rr
      API::write_byte(addr + 2, dst.value & 0xFF);
      API::write_byte(addr + 3, dst.value >> 8);
      return 4;
    }
  }

  print_operand_error<API>(src);
  return 0;
}

template <typename API>
uint8_t encode_push_pop(uint16_t addr, bool is_push, Operand& op) {
  uint8_t code = is_push ? 0305 : 0301;
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t pair = token_to_pair(op.token, prefix, true);
  if (pair == PAIR_INVALID) {
    print_operand_error<API>(op);
    return 0;
  }
  if (prefix != 0) API::write_byte(addr++, prefix);
  API::write_byte(addr, code | pair << 4);
  return prefix != 0 ? 2 : 1;
}

template <typename API>
uint8_t encode_ret(uint16_t addr, Operand& op) {
  if (op.token == TOK_INVALID) {
    API::write_byte(addr, 0xC9);
    return 1;
  } else {
    uint8_t cond = token_to_cond(op.token);
    if (cond == COND_INVALID) {
      print_operand_error<API>(op);
      return 0;
    }
    API::write_byte(addr, 0300 | (cond << 3));
    return 1;
  }
}

template <typename API>
uint8_t encode_rst(uint16_t addr, Operand& op) {
  if (op.token == TOK_INTEGER && (op.value & 0307) == 0) {
    API::write_byte(addr, 0307 | op.value);
    return 1;
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

template <typename API>
uint8_t impl_asm(uint16_t addr, Instruction inst) {
  Operand& op1 = inst.operands[0];
  Operand& op2 = inst.operands[1];
  switch (inst.mnemonic) {
  case MNE_ADC: case MNE_ADD: case MNE_SBC: case MNE_SUB:
  case MNE_AND: case MNE_CP:  case MNE_OR:  case MNE_XOR:
    return encode_alu<API>(addr, inst.mnemonic, op1, op2);
  case MNE_RLC: case MNE_RRC: case MNE_RL:  case MNE_RR:
  case MNE_SLA: case MNE_SRA: case MNE_SL1: case MNE_SRL:
    return encode_rot<API>(addr, inst.mnemonic, op1);
  case MNE_BIT: case MNE_RES: case MNE_SET:
    return encode_bit<API>(addr, inst.mnemonic, op1, op2);
  case MNE_CALL:
    return encode_call_jp<API>(addr, true, op1, op2);
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
  case MNE_DEC:
    return encode_inc_dec<API>(addr, false, op1);
  case MNE_DI:
    API::write_byte(addr, 0xF3);
    return 1;
  case MNE_DJNZ:
    return encode_djnz_jr<API>(addr, 0x10, op1);
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
    return encode_im<API>(addr, op1);
  case MNE_IN:
    return encode_in_out<API>(addr, true, op1, op2);
  case MNE_INC:
    return encode_inc_dec<API>(addr, true, op1);
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
  case MNE_JP:
    return encode_call_jp<API>(addr, false, op1, op2);
  case MNE_JR:
    return encode_jr<API>(addr, op1, op2);
  case MNE_LD:
    return encode_ld<API>(addr, op1, op2);
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
  case MNE_OUT:
    return encode_in_out<API>(addr, false, op2, op1);
  case MNE_OUTD:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xAB);
    return 2;
  case MNE_OUTI:
    API::write_byte(addr, PREFIX_ED);
    API::write_byte(addr + 1, 0xA3);
    return 2;
  case MNE_POP:
    return encode_push_pop<API>(addr, false, op1);
  case MNE_PUSH:
    return encode_push_pop<API>(addr, true, op1);
  case MNE_RET:
    return encode_ret<API>(addr, op1);
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
    return encode_rst<API>(addr, op1);
  case MNE_SCF:
    API::write_byte(addr, 0x37);
    return 1;
  }
  return 0;
}

} // namespace z80
} // namespace uMon
