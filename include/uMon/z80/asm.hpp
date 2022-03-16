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
uint8_t write_op(uint16_t addr, uint8_t code) {
  API::write_byte(addr, code);
  return 1;
}

template <typename API>
uint8_t write_op_byte(uint16_t addr, uint8_t code, uint8_t data) {
  API::write_byte(addr, code);
  API::write_byte(addr + 1, data);
  return 2;
}

template <typename API>
uint8_t write_prefix_op(uint16_t addr, uint8_t prefix, uint8_t code) {
  bool has_prefix = prefix != 0;
  if (has_prefix) API::write_byte(addr++, prefix);
  return has_prefix + write_op<API>(addr, code);
}

template <typename API>
uint8_t write_prefix_op_index(uint16_t addr, uint8_t prefix, uint8_t code, uint8_t index, bool has_index) {
  uint8_t size = write_prefix_op<API>(addr, prefix, code);
  if (has_index) API::write_byte(addr + size, index);
  return has_index + size;
}

template <typename API>
uint8_t write_op_word(uint16_t addr, uint8_t code, uint16_t data) {
  API::write_byte(addr, code);
  API::write_byte(addr + 1, data & 0xFF);
  API::write_byte(addr + 2, data >> 8);
  return 3;
}

template <typename API>
uint8_t write_prefix_op_word(uint16_t addr, uint8_t prefix, uint8_t code, uint16_t data) {
  bool has_prefix = prefix != 0;
  if (has_prefix) API::write_byte(addr++, prefix);
  return has_prefix + write_op_word<API>(addr, code, data);
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
    uint8_t code = 0306 | alu << 3;
    return write_op_byte<API>(addr, code, src.value);
  } else {
    // Validate source operand is register
    uint8_t prefix = token_to_prefix(src.token);
    uint8_t reg = token_to_reg(src.token, prefix);
    if (reg == REG_INVALID) {
      print_operand_error<API>(src);
      return 0;
    }
    bool has_index = prefix != 0 && reg == REG_M;
    uint8_t code = 0200 | (alu << 3) | reg;
    return write_prefix_op_index<API>(addr, prefix, code, src.value, has_index);
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
    uint8_t code = 0011 | (src_pair << 4); // ADD HL,rr
    return write_prefix_op<API>(addr, prefix, code);
  } else if (prefix == 0 && (mne == MNE_ADC || mne == MNE_SBC)) {
    uint8_t code = mne == MNE_ADC ? 0112 : 0102; // ADC/SBC HL,rr
    return write_prefix_op<API>(addr, PREFIX_ED, code | src_pair << 4);
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
    return write_prefix_op<API>(addr, PREFIX_CB, code | reg);
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
  uint8_t code = index_of(CB_MNE, mne) << 6;
  return encode_cb<API>(addr, code | bit, op2);
}

template <typename API>
uint8_t encode_call_jp(uint16_t addr, bool is_call, Operand& op1, Operand& op2) {
  if (op2.token == TOK_INTEGER) {
    uint8_t cond = token_to_cond(op1.token);
    if (cond != COND_INVALID) {
      uint8_t code = (is_call ? 0304 : 0302) | (cond << 3);
      return write_op_word<API>(addr, code, op2.value);
    }
  } else if (op2.token == TOK_INVALID) {
    if (op1.token == TOK_INTEGER) {
      uint8_t code = is_call ? 0315 : 0303;
      return write_op_word<API>(addr, code, op1.value);
    } else if (!is_call) { // JP only
      uint8_t prefix = token_to_prefix(op1.token);
      uint8_t reg = token_to_reg(op1.token, prefix);
      if (reg == REG_M) { // (HL), (IX), (IY)
        return write_prefix_op<API>(addr, prefix, 0xE9); // JP (HL)
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
  bool has_prefix = prefix != 0;
  if (reg != REG_INVALID) {
    bool has_index = has_prefix && reg == REG_M;
    uint8_t code = (is_inc ? 0004 : 0005) | reg << 3;
    return write_prefix_op_index<API>(addr, prefix, code, op.value, has_index);
  } else if (pair != PAIR_INVALID) {
    uint8_t code = is_inc ? 0003 : 0013; // INC/DEC rr
    return write_prefix_op<API>(addr, prefix, code | pair << 4);
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
    return write_prefix_op<API>(addr, prefix, 0xE3); // EX (SP),HL
  } else if (op1.token == TOK_DE && op2.token == TOK_HL) {
    return write_op<API>(addr, 0xEB); // EX DE,HL
  } else if (op1.token == TOK_AF && (op2.token == TOK_AF || op2.token == TOK_INVALID)) {
    return write_op<API>(addr, 0x08); // EX AF,AF
  } else {
    print_operand_error<API>(op1);
    return 0;
  }
}

template <typename API>
uint8_t encode_im(uint16_t addr, Operand& op) {
  if (op.token == TOK_INTEGER && op.value < 3) {
    static constexpr const uint8_t IM[] = { 0x46, 0x56, 0x5E };
    return write_prefix_op<API>(addr, PREFIX_ED, IM[op.value]);
  } else if (op.token == TOK_UNDEFINED) {
    return write_prefix_op<API>(addr, PREFIX_ED, 0x4E);
  } else {
    print_operand_error<API>(op);
    return 0;
  }
}

template <typename API>
uint8_t encode_in_out(uint16_t addr, bool is_in, Operand& data, Operand& port) {
  if (data.token == TOK_A && port.token == (TOK_INTEGER | TOK_INDIRECT)) {
    uint8_t code = is_in ? 0333 : 0323; // IN A,(n) / OUT (n),A
    return write_op_byte<API>(addr, code, port.value);
  } else if (port.token == (TOK_C | TOK_INDIRECT)) {
    uint8_t reg = token_to_reg(data.token);
    if (reg == REG_INVALID || reg == REG_M) {
      print_operand_error<API>(data);
      return 0;
    }
    uint8_t code = (is_in ? 0100 : 0101) | reg << 3;
    return write_prefix_op<API>(addr, PREFIX_ED, code);
  } else {
    print_operand_error<API>(port);
    return 0;
  }
}

template <typename API>
uint8_t encode_djnz_jr(uint16_t addr, uint8_t code, Operand& op) {
  int16_t disp = op.value - (addr + 2);
  if (op.token == TOK_INTEGER && disp >= -128 && disp <= 127) {
    return write_op_byte<API>(addr, code, disp);
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
      return write_prefix_op<API>(addr, PREFIX_ED, 0x57); // LD A,I
    case TOK_R:
      return write_prefix_op<API>(addr, PREFIX_ED, 0x5F); // LD A,R
    case TOK_BC | TOK_INDIRECT:
      return write_op<API>(addr, 0x0A); // LD A,(BC)
    case TOK_DE | TOK_INDIRECT:
      return write_op<API>(addr, 0x1A); // LD A,(DE)
    case TOK_INTEGER | TOK_INDIRECT:
      return write_op_word<API>(addr, 0x3A, src.value); // LD A,(nn)
    }
  }

  // Special cases for source A
  if (src.token == TOK_A) {
    switch (dst.token) {
    case TOK_I:
      return write_prefix_op<API>(addr, PREFIX_ED, 0x47); // LD I,A
    case TOK_R:
      return write_prefix_op<API>(addr, PREFIX_ED, 0x4F); // LD R,A
    case TOK_BC | TOK_INDIRECT:
      return write_op<API>(addr, 0x02); // LD (BC),A
    case TOK_DE | TOK_INDIRECT:
      return write_op<API>(addr, 0x12); // LD (DE),A
    case TOK_INTEGER | TOK_INDIRECT:
      return write_op_word<API>(addr, 0x32, dst.value); // LD (nn),A
    }
  }

  // Special cases for destination HL/IX/IY
  uint8_t dst_prefix = token_to_prefix(dst.token);
  uint8_t dst_pair = token_to_pair(dst.token, dst_prefix);
  if (dst_pair == PAIR_HL) { // HL, IX, IY
    if (src.token == (TOK_INTEGER | TOK_INDIRECT)) { // LD HL,(nn)
      return write_prefix_op_word<API>(addr, dst_prefix, 0x2A, src.value);
    }
  }

  // Special cases for source HL/IX/IY
  uint8_t src_prefix = token_to_prefix(src.token);
  uint8_t src_pair = token_to_pair(src.token, src_prefix);
  if (src_pair == PAIR_HL) { // HL, IX, IY
    if (dst.token == (TOK_INTEGER | TOK_INDIRECT)) { // LD (nn),HL
      return write_prefix_op_word<API>(addr, src_prefix, 0x22, dst.value);
    } else if (dst.token == TOK_SP) {
      return write_prefix_op<API>(addr, src_prefix, 0xF9); // LD SP,HL
    }
  }

  // Catch-all cases for any register/pair
  uint8_t dst_reg = token_to_reg(dst.token, dst_prefix);
  if (dst_reg != REG_INVALID) {
    // Destination is any primary register
    uint8_t src_reg = token_to_reg(src.token, src_prefix);
    if (src_reg != REG_INVALID) {
      // Source is any primary register
      bool src_is_m = src_reg == REG_M;
      bool dst_is_m = dst_reg == REG_M;
      bool dst_in_src = token_to_reg(dst.token, src_prefix) != REG_INVALID;
      bool src_in_dst = token_to_reg(src.token, dst_prefix) != REG_INVALID;
      // - only one can be (HL/IX/IY) and other can't be IXH/IXL/IYH/IYL
      // - H/L, IXH/IXL, IYH/IYL can't be mixed; prefix affects both regs
      if ((src_is_m && !dst_is_m && dst_prefix == 0)
        || (dst_is_m && !src_is_m && src_prefix == 0)
        || (!src_is_m && !dst_is_m && (dst_in_src || src_in_dst))) {
        uint8_t prefix = dst_prefix | src_prefix;
        bool has_prefix = prefix != 0;
        bool has_index = has_prefix && (dst_is_m || src_is_m);
        uint8_t disp = dst_reg == REG_M ? dst.value : src.value;
        uint8_t code = 0100 | dst_reg << 3 | src_reg; // LD r,r
        return write_prefix_op_index<API>(addr, prefix, code, disp, has_index);
      }
    } else if (src.token == TOK_INTEGER) {
      // Source is 1-byte integer
      uint8_t& prefix = dst_prefix;
      bool has_prefix = prefix != 0;
      bool has_index = has_prefix && dst_reg == REG_M;
      uint8_t code = 0006 | dst_reg << 3; // LD r,n
      uint8_t size = write_prefix_op_index<API>(addr, prefix, code, dst.value, has_index);
      API::write_byte(addr + size, src.value & 0xFF);
      return size + 1;
    }
  } else if (dst_pair != PAIR_INVALID) {
    if (src.token == TOK_INTEGER) {
      uint8_t code = 0001 | dst_pair << 4; // LD rr,nn
      return write_prefix_op_word<API>(addr, dst_prefix, code, src.value);
    } else if (src.token == (TOK_INTEGER | TOK_INDIRECT)) {
      // NOTE LD HL/IX/IY,(nn) already handled by special case; only BC/DE/SP
      uint8_t code = 0113 | dst_pair << 4; // LD rr,(nn)
      return write_prefix_op_word<API>(addr, PREFIX_ED, code, src.value);
    }
  } else if (src_pair != PAIR_INVALID) {
    if (dst.token == (TOK_INTEGER | TOK_INDIRECT)) {
      // NOTE LD (nn),HL/IX/IY already handled by special case; only BC/DE/SP
      uint8_t code = 0103 | src_pair << 4; // LD (nn),rr
      return write_prefix_op_word<API>(addr, PREFIX_ED, code, dst.value);
    }
  }

  print_operand_error<API>(src);
  return 0;
}

template <typename API>
uint8_t encode_push_pop(uint16_t addr, bool is_push, Operand& op) {
  uint8_t code = is_push ? 0305 : 0301; // PUSH/POP rr
  uint8_t prefix = token_to_prefix(op.token);
  uint8_t pair = token_to_pair(op.token, prefix, true);
  if (pair == PAIR_INVALID) {
    print_operand_error<API>(op);
    return 0;
  }
  return write_prefix_op<API>(addr, prefix, code | pair << 4);
}

template <typename API>
uint8_t encode_ret(uint16_t addr, Operand& op) {
  if (op.token == TOK_INVALID) {
    return write_op<API>(addr, 0xC9); // RET
  } else {
    uint8_t cond = token_to_cond(op.token);
    if (cond == COND_INVALID) {
      print_operand_error<API>(op);
      return 0;
    }
    return write_op<API>(addr, 0300 | cond << 3); // RET cc
  }
}

template <typename API>
uint8_t encode_rst(uint16_t addr, Operand& op) {
  if (op.token == TOK_INTEGER && (op.value & 0307) == 0) {
    return write_op<API>(addr, 0307 | op.value);
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
    return write_op<API>(addr, 0x3F);
  case MNE_CPD:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xA9);
  case MNE_CPDR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xB9);
  case MNE_CPI:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xA1);
  case MNE_CPIR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xB1);
  case MNE_CPL:
    return write_op<API>(addr, 0x2F);
  case MNE_DAA:
    return write_op<API>(addr, 0x27);
  case MNE_DEC:
    return encode_inc_dec<API>(addr, false, op1);
  case MNE_DI:
    return write_op<API>(addr, 0xF3);
  case MNE_DJNZ:
    return encode_djnz_jr<API>(addr, 0x10, op1);
  case MNE_EI:
    return write_op<API>(addr, 0xFB);
  case MNE_EX:
    return encode_ex<API>(addr, op1, op2);
  case MNE_EXX:
    return write_op<API>(addr, 0xD9);
  case MNE_HALT:
    return write_op<API>(addr, 0x76);
  case MNE_IM:
    return encode_im<API>(addr, op1);
  case MNE_IN:
    return encode_in_out<API>(addr, true, op1, op2);
  case MNE_INC:
    return encode_inc_dec<API>(addr, true, op1);
  case MNE_IND:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xAA);
  case MNE_INDR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xBA);
  case MNE_INI:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xA2);
  case MNE_INIR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xB2);
  case MNE_JP:
    return encode_call_jp<API>(addr, false, op1, op2);
  case MNE_JR:
    return encode_jr<API>(addr, op1, op2);
  case MNE_LD:
    return encode_ld<API>(addr, op1, op2);
  case MNE_LDD:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xA8);
  case MNE_LDDR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xB8);
  case MNE_LDI:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xA0);
  case MNE_LDIR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xB0);
  case MNE_NEG:
    return write_prefix_op<API>(addr, PREFIX_ED, 0x44);
  case MNE_NOP:
    return write_op<API>(addr, 0x00);
  case MNE_OTDR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xBB);
  case MNE_OTIR:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xB3);
  case MNE_OUT:
    return encode_in_out<API>(addr, false, op2, op1);
  case MNE_OUTD:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xAB);
  case MNE_OUTI:
    return write_prefix_op<API>(addr, PREFIX_ED, 0xA3);
  case MNE_POP:
    return encode_push_pop<API>(addr, false, op1);
  case MNE_PUSH:
    return encode_push_pop<API>(addr, true, op1);
  case MNE_RET:
    return encode_ret<API>(addr, op1);
  case MNE_RETI:
    return write_prefix_op<API>(addr, PREFIX_ED, 0x4D);
  case MNE_RETN:
    return write_prefix_op<API>(addr, PREFIX_ED, 0x45);
  case MNE_RLA:
    return write_op<API>(addr, 0x17);
  case MNE_RLCA:
    return write_op<API>(addr, 0x07);
  case MNE_RLD:
    return write_prefix_op<API>(addr, PREFIX_ED, 0x6F);
  case MNE_RRA:
    return write_op<API>(addr, 0x1F);
  case MNE_RRCA:
    return write_op<API>(addr, 0x0F);
  case MNE_RRD:
    return write_prefix_op<API>(addr, PREFIX_ED, 0x67);
  case MNE_RST:
    return encode_rst<API>(addr, op1);
  case MNE_SCF:
    return write_op<API>(addr, 0x37);
  }
  return 0;
}

} // namespace z80
} // namespace uMon
