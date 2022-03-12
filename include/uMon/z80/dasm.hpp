// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

// 8080/Z80 (and even x86!) opcodes are organized by octal groupings
// http://z80.info/decoding.htm is a great resource for decoding these

#pragma once

#include "uMon/z80/common.hpp"
#include "uMon/format.hpp"

#include <stdint.h>

namespace uMon {
namespace z80 {

// Print given code as hex followed by '?'
template <typename API>
void print_error(uint8_t prefix, uint8_t code) {
  API::print_char('$');
  fmt_hex8(API::print_char, prefix);
  fmt_hex8(API::print_char, code);
  API::print_char('?');
}

// Print 1-byte immediate located at addr
template <typename API>
void print_imm_byte(uint16_t addr) {
  API::print_char('$');
  fmt_hex8(API::print_char, API::read_byte(addr));
}

// Print 2-byte immediate with LSB at addr and MSB at addr + 1
template <typename API>
void print_imm_word(uint16_t addr) {
  API::print_char('$');
  fmt_hex8(API::print_char, API::read_byte(addr + 1));
  fmt_hex8(API::print_char, API::read_byte(addr));
}

// Format displacement following relative jump
template <typename API>
void print_disp(uint16_t addr) {
  API::print_char('$');
  const int8_t disp = API::read_byte(addr + 1);
  fmt_hex16(API::print_char, addr + 2 + disp);
}

// Print IX/IY given prefix
template <typename API>
void print_index_reg(uint8_t prefix) {
  print_pgm_string<API>(prefix == 0xDD ? TOK_STR_IX : TOK_STR_IY);
}

// Print (IX/IY+disp) given address of displacement byte
template <typename API>
void print_index_ind(uint16_t addr, uint8_t prefix) {
  const int8_t disp = API::read_byte(addr);
  API::print_char('(');
  print_index_reg<API>(prefix);
  if (disp != 0) {
    // Print absolute displacement in hex
    API::print_string(disp < 0 ? "-$" : "+$");
    fmt_hex8(API::print_char, disp < 0 ? -disp : disp);
  }
  API::print_char(')');
}

// Print reg, optionally with IX/IY prefix, or (HL)
// (IX/IY+disp) should be handled with print_index_ind instead
template <typename API>
void print_reg(uint8_t reg, uint8_t prefix) {
  if (prefix != 0 && (reg == REG_L || reg == REG_H)) {
    print_index_reg<API>(prefix);
  }
  print_token<API>(REG_TOK[reg]);
}

// Print pair, replacing HL with IX/IY if prefixed, and SP with AF if flagged
template <typename API>
void print_pair(uint8_t pair, uint8_t prefix, bool use_af = false) {
  const bool has_prefix = prefix != 0;
  if (has_prefix && pair == PAIR_HL) {
    print_index_reg<API>(prefix);
  } else if (use_af && pair == PAIR_SP) {
    print_pgm_string<API>(TOK_STR_AF);
  } else {
    print_token<API>(PAIR_TOK[pair]);
  }
}

// Decode IN/OUT (c): ED [01 --- 00-]
template <typename API>
uint16_t decode_in_out_c(uint16_t addr, uint8_t code) {
  const bool is_out = (code & 01) == 01;
  const uint8_t reg = (code & 070) >> 3;
  print_pgm_string<API>(is_out ? MNE_STR_OUT : MNE_STR_IN);
  API::print_char(' ');
  if (is_out) { API::print_string("(C),"); }
  // NOTE reg (HL) is undefined; OUT sends 0 and IN sets flags without storing
  if (reg == REG_M) {
    API::print_char('?');
  } else {
    print_token<API>(REG_TOK[reg]);
  }
  if (!is_out) { API::print_string(",(C)"); }
  return addr + 1;
}

// Decode 16-bit ADC/SBC: ED [01 --- 010]
template <typename API>
uint16_t decode_hl_adc(uint16_t addr, uint8_t code) {
  const bool is_adc = (code & 010) == 010;
  const uint8_t pair = (code & 060) >> 4;
  print_pgm_string<API>(is_adc ? MNE_STR_ADC : MNE_STR_SBC);
  API::print_char(' ');
  print_pgm_string<API>(TOK_STR_HL);
  API::print_char(',');
  print_token<API>(PAIR_TOK[pair]);
  return addr + 1;
}

// Decode 16-bit LD ind: ED [01 --- 011]
template <typename API>
uint16_t decode_ld_pair_ind(uint16_t addr, uint8_t code) {
  const bool is_load = (code & 010) == 010;
  const uint8_t pair = (code & 060) >> 4;
  print_pgm_string<API>(MNE_STR_LD);
  API::print_char(' ');
  if (is_load) {
    print_token<API>(PAIR_TOK[pair]);
    API::print_char(',');
  }
  API::print_char('(');
  print_imm_word<API>(addr + 1);
  API::print_char(')');
  if (!is_load) {
    API::print_char(',');
    print_token<API>(PAIR_TOK[pair]);
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
      print_error<API>(0xED, code);
    } else {
      print_pgm_string<API>(is_rl ? MNE_STR_RLD : MNE_STR_RRD);
    }
  } else {
    print_pgm_string<API>(MNE_STR_LD);
    API::print_char(' ');
    if (is_load) { API::print_string("A,"); }
    API::print_char(is_rl ? 'R' : 'I');
    if (!is_load) { API::print_string(",A"); }
  }
  return addr + 1;
}

// Decode block transfer ops: ED [10 1-- 0--]
template <typename API>
uint16_t decode_block_ops(uint16_t addr, uint8_t code) {
  static constexpr const uint8_t OPS[4][4] = {
    { MNE_LDI, MNE_LDD, MNE_LDIR, MNE_LDDR },
    { MNE_CPI, MNE_CPD, MNE_CPIR, MNE_CPDR },
    { MNE_INI, MNE_IND, MNE_INIR, MNE_INDR },
    { MNE_OUTI, MNE_OUTD, MNE_OTIR, MNE_OTDR },
  };
  const uint8_t op = (code & 03);
  const uint8_t var = (code & 030) >> 3;
  print_pgm_strtab<API>(MNE_STR, OPS[op][var]);
  return addr + 1;
}

// Disassemble extended opcodes prefixed by $ED
template <typename API>
uint16_t decode_ed(uint16_t addr) {
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
      print_pgm_string<API>(MNE_STR_NEG);
      return addr + 1;
    case 5:
      // NOTE only 0x45 RETN is documented
      print_pgm_string<API>(code == 0x4D ? MNE_STR_RETI : MNE_STR_RETN);
      return addr + 1;
    case 6:
      // NOTE only 0x46, 0x56, 0x5E are documented; '?' sets an undefined mode
      static constexpr const char IM[] = { '0', '?', '1', '2' };
      print_pgm_string<API>(MNE_STR_IM);
      API::print_char(' ');
      API::print_char(IM[(code & 030) >> 3]);
      return addr + 1;
    case 7:
      return decode_ld_ir<API>(addr, code);
    }
  } else if ((code & 0344) == 0240) {
    return decode_block_ops<API>(addr, code);
  }
  print_error<API>(0xED, code);
  return addr + 1;
}

// Disassemble extended opcodes prefixed by $CB
template <typename API>
uint16_t decode_cb(uint16_t addr, uint8_t prefix) {
  const bool has_prefix = prefix != 0;
  // If prefixed, index displacement byte comes before opcode
  const uint8_t code = API::read_byte(has_prefix ? addr + 1 : addr);
  const uint8_t op = (code & 0300) >> 6;
  const uint8_t index = (code & 070) >> 3;
  const uint8_t reg = (code & 07);
  // Print opcode
  const uint8_t mne = op == CB_ROT ? ROT_MNE[index] : CB_MNE[op];
  print_pgm_strtab<API>(MNE_STR, mne);
  API::print_char(' ');
  // Print bit index (only for BIT/RES/SET)
  if (op != CB_ROT) {
    API::print_char('0' + index);
    API::print_char(',');
  }
  if (has_prefix) {
    if (op != CB_BIT && reg != REG_M) {
      // NOTE operand other than (HL) is undocumented
      // (IX/IY) is still used, but result also copied to reg
      print_token<API>(REG_TOK[reg]);
      API::print_char(',');
    }
    // Print (IX/IY+disp)
    print_index_ind<API>(addr, prefix);
    return addr + 2;
  } else {
    // Print register operand
    print_token<API>(REG_TOK[reg]);
    return addr + 1;
  }
}

// Disassemble relative jumps: [00 --- 000]
template <typename API>
uint16_t decode_jr(uint16_t addr, uint8_t code) {
  switch (code & 070) {
  case 000:
    print_pgm_string<API>(MNE_STR_NOP);
    return addr + 1;
  case 010:
    print_pgm_string<API>(MNE_STR_EX);
    API::print_char(' ');
    print_pgm_string<API>(TOK_STR_AF);
    return addr + 1;
  case 020:
    // DJNZ e
    print_pgm_string<API>(MNE_STR_DJNZ);
    API::print_char(' ');
    print_disp<API>(addr);
    return addr + 2;
  case 030:
    // JR e
    print_pgm_string<API>(MNE_STR_JR);
    API::print_char(' ');
    print_disp<API>(addr);
    return addr + 2;
  default:
    // JR cc, e
    print_pgm_string<API>(MNE_STR_JR);
    API::print_char(' ');
    print_token<API>(COND_TOK[(code & 030) >> 3]);
    API::print_char(',');
    print_disp<API>(addr);
    return addr + 2;
  }
}

// Disassemble LD/ADD pair: [00 --- 001]
template <typename API>
uint16_t decode_ld_add_pair(uint16_t addr, uint8_t code, uint8_t prefix) {
  const bool is_load = (code & 010) == 0;
  const uint16_t pair = (code & 060) >> 4;
  if (is_load) {
    // LD rr, nn
    print_pgm_string<API>(MNE_STR_LD);
    API::print_char(' ');
    print_pair<API>(pair, prefix);
    API::print_char(',');
    print_imm_word<API>(addr + 1);
    return addr + 3;
  } else {
    // ADD HL, rr
    print_pgm_string<API>(MNE_STR_ADD);
    API::print_char(' ');
    print_pair<API>(PAIR_HL, prefix);
    API::print_char(',');
    print_pair<API>(pair, prefix);
    return addr + 1;
  }
}

// Print HL/IX/IY or A
template <typename API>
void print_hl_or_a(bool use_hl, uint8_t prefix) {
  if (use_hl) {
    print_pair<API>(PAIR_HL, prefix);
  } else {
    API::print_char('A');
  }
}

// Disassemble indirect loads: [00 --- 010]
template <typename API>
uint16_t decode_ld_ind(uint16_t addr, uint8_t code, uint8_t prefix) {
  // Decode 070 bitfield
  const bool is_store = (code & 010) == 0; // A/HL is src instead of dst
  const bool use_hl = (code & 060) == 040; // Use HL instead of A
  const bool use_pair = (code & 040) == 0; // Use (BC/DE) instead of (nn)
    print_pgm_string<API>(MNE_STR_LD);
    API::print_char(' ');
  // Print A/HL/IX/IY first for loads
  if (!is_store) {
    print_hl_or_a<API>(use_hl, prefix);
    API::print_char(',');
  }
  // Print (BC/DE) or (nn)
  API::print_char('(');
  if (use_pair) {
    print_token<API>(PAIR_TOK[(code & 020) >> 4]);
  } else {
    print_imm_word<API>(addr + 1);
  }
  API::print_char(')');
  // Print A/HL/IX/IY second for stores
  if (is_store) {
    API::print_char(',');
    print_hl_or_a<API>(use_hl, prefix);
  }
  // Opcodes followed by (nn) consume 2 extra bytes
  if (use_pair) {
    return addr + 1;
  } else {
    return addr + 3;
  }
}

// Disassemble LD r, n: ([ix/iy]) [00 r 110] ([d]) [n]
template <typename API>
uint16_t decode_ld_reg_imm(uint16_t addr, uint8_t code, uint8_t prefix) {
  const uint8_t reg = (code & 070) >> 3;
  const bool has_prefix = prefix != 0;
    print_pgm_string<API>(MNE_STR_LD);
    API::print_char(' ');
  if (has_prefix && reg == REG_M) {
    print_index_ind<API>(addr + 1, prefix);
    API::print_char(',');
    print_imm_byte<API>(addr + 2);
    return addr + 3;
  } else {
    print_reg<API>(reg, prefix);
    API::print_char(',');
    print_imm_byte<API>(addr + 1);
    return addr + 2;
  }
}

// Disassemble INC/DEC: [00 --- 011/100/101]
template <typename API>
uint16_t decode_inc_dec(uint16_t addr, uint8_t code, uint8_t prefix) {
  const bool is_pair = (code & 04) == 0;
  const bool is_inc = is_pair ? (code & 010) == 0 : (code & 01) == 0;
  print_pgm_string<API>(is_inc ? MNE_STR_INC : MNE_STR_DEC);
  API::print_char(' ');
  if (is_pair) {
    const uint8_t pair = (code & 060) >> 4;
    print_pair<API>(pair, prefix);
    return addr + 1;
  } else {
    const bool has_prefix = prefix != 0;
    const uint8_t reg = (code & 070) >> 3;
    if (has_prefix && reg == REG_M) {
      // Replace (HL) with (IX/IY+disp)
      print_index_ind<API>(addr + 1, prefix);
      return addr + 2;
    } else {
      print_reg<API>(reg, prefix);
      return addr + 1;
    }
  }
}

// Decode LD r, r: [01 --- ---]
template <typename API>
uint16_t decode_ld_reg_reg(uint16_t addr, uint8_t code, uint8_t prefix) {
  // Replace LD (HL),(HL) with HALT
  if (code == 0x76) {
    print_pgm_string<API>(MNE_STR_HALT);
    return addr + 1;
  }
  const uint8_t dest = (code & 070) >> 3;
  const uint8_t src = (code & 07);
  // If (HL) used, replace with (IX/IY+disp)
  // Otherwise, replace H/L with IXH/IXL
  // NOTE the latter effect is undocumented!
  const bool has_prefix = prefix != 0;
  const bool has_dest_index = has_prefix && dest == REG_M;
  const bool has_src_index = has_prefix && src == REG_M;
  const bool has_index = has_dest_index || has_src_index;
  print_pgm_string<API>(MNE_STR_LD);
  API::print_char(' ');
  // Print destination register
  if (has_dest_index) {
    print_index_ind<API>(addr + 1, prefix);
  } else {
    print_reg<API>(dest, has_index ? 0 : prefix);
  }
  API::print_char(',');
  // Print source register
  if (has_src_index) {
    print_index_ind<API>(addr + 1, prefix);
  } else {
    print_reg<API>(src, has_index ? 0 : prefix);
  }
  // Skip displacement byte if (IX/IY+disp) is used
  return has_index ? addr + 2 : addr + 1;
}

// Decode [ALU op] A, r: [10 --- ---]
template <typename API>
uint16_t decode_alu_a_reg(uint16_t addr, uint8_t code, uint8_t prefix) {
  const uint8_t op = (code & 070) >> 3;
  const uint8_t reg = code & 07;
  const bool has_prefix = prefix != 0;
  // Print [op] A,
  print_pgm_strtab<API>(MNE_STR, ALU_MNE[op]);
  API::print_string(" A,");
  // Print operand reg
  if (has_prefix && reg == REG_M) {
    print_index_ind<API>(addr + 1, prefix);
    return addr + 2;
  } else {
    print_reg<API>(reg, prefix);
    return addr + 1;
  }
}

// Decode conditional RET/JP/CALL
template <typename API>
uint16_t decode_jp_cond(uint16_t addr, uint8_t code) {
  static constexpr const uint8_t OPS[] = { MNE_RET, MNE_JP, MNE_CALL };
  const uint8_t op = (code & 06) >> 1;
  const uint8_t cond = (code & 070) >> 3;
  print_pgm_strtab<API>(MNE_STR, OPS[op]);
  API::print_char(' ');
  print_token<API>(COND_TOK[cond]);
  if (op != 0) {
    API::print_char(',');
    print_imm_word<API>(addr + 1);
    return addr + 3;
  } else {
    return addr + 1;
  }
}

// Decode PUSH/POP/CALL/RET and misc
template <typename API>
uint16_t decode_push_pop(uint16_t addr, uint8_t code, uint8_t prefix) {
  const bool is_push = (code & 04) == 04;
  switch (code & 070) {
  case 010:
    if (is_push) {
      print_pgm_string<API>(MNE_STR_CALL);
      API::print_char(' ');
      print_imm_word<API>(addr + 1);
      return addr + 3;
    } else {
      print_pgm_string<API>(MNE_STR_RET);
      return addr + 1;
    }
  case 030:
    print_pgm_string<API>(MNE_STR_EXX);
    return addr + 1;
  case 050:
    print_pgm_string<API>(MNE_STR_JP);
    API::print_string(" (");
    print_pair<API>(PAIR_HL, prefix);
    API::print_char(')');
    return addr + 1;
  case 070:
    print_pgm_string<API>(MNE_STR_LD);
    API::print_char(' ');
    print_pgm_string<API>(TOK_STR_SP);
    API::print_char(',');
    print_pair<API>(PAIR_HL, prefix);
    return addr + 1;
  default:
    print_pgm_string<API>(is_push ? MNE_STR_PUSH : MNE_STR_POP);
    API::print_char(' ');
    print_pair<API>((code & 060) >> 4, prefix, true);
    return addr + 1;
  }
}

template <typename API>
uint16_t decode_misc_hi(uint16_t addr, uint8_t code, uint8_t prefix) {
  switch (code & 070) {
  case 000:
    // JP (nn)
    print_pgm_string<API>(MNE_STR_JP);
    API::print_char(' ');
    print_imm_word<API>(addr + 1);
    return addr + 3;
  case 010:
    return decode_cb<API>(addr + 1, prefix);
  case 020:
    // OUT (n),A
    print_pgm_string<API>(MNE_STR_OUT);
    API::print_string(" (");
    print_imm_byte<API>(addr + 1);
    API::print_string("),A");
    return addr + 2;
  case 030:
    // IN A,(n)
    print_pgm_string<API>(MNE_STR_IN);
    API::print_string(" A,(");
    print_imm_byte<API>(addr + 1);
    API::print_char(')');
    return addr + 2;
  case 040:
    print_pgm_string<API>(MNE_STR_EX);
    API::print_char(' ');
    print_token<API>(TOK_SP + TOK_INDIRECT);
    API::print_char(',');
    print_pair<API>(PAIR_HL, prefix);
    return addr + 1;
  case 050:
    // NOTE EX DE,HL unaffected by prefix
    print_pgm_string<API>(MNE_STR_EX);
    API::print_char(' ');
    print_pgm_string<API>(TOK_STR_DE);
    API::print_char(',');
    print_pgm_string<API>(TOK_STR_HL);
    return addr + 1;
  case 060:
    print_pgm_string<API>(MNE_STR_DI);
    return addr + 1;
  default: // 070
    print_pgm_string<API>(MNE_STR_EI);
    return addr + 1;
  }
}

template <typename API>
uint16_t decode_base(uint16_t addr, uint8_t prefix = 0) {
  uint8_t code = API::read_byte(addr);
  // Handle prefix codes
  if (code == 0xDD || code == 0xED || code == 0xFD) {
    if (prefix != 0) {
      // Discard old prefix and start over
      print_error<API>(prefix, code);
      return addr;
    } else {
      if (code == 0xED) {
        return decode_ed<API>(addr + 1);
      } else {
        return decode_base<API>(addr + 1, code);
      }
    }
  }
  // Decode by leading octal digit
  switch (code & 0300) {
  case 0000:
    // Decode by trailing octal digit
    switch (code & 07) {
    case 0:
      return decode_jr<API>(addr, code);
    case 1:
      return decode_ld_add_pair<API>(addr, code, prefix);
    case 2:
      return decode_ld_ind<API>(addr, code, prefix);
    case 6:
      return decode_ld_reg_imm<API>(addr, code, prefix);
    case 7:
      // Misc AF ops with no operands
      print_pgm_strtab<API>(MNE_STR, MISC_MNE[(code & 070) >> 3]);
      return addr + 1;
    default: // 3, 4, 5
      return decode_inc_dec<API>(addr, code, prefix);
    }
  case 0100:
    return decode_ld_reg_reg<API>(addr, code, prefix);
  case 0200:
    return decode_alu_a_reg<API>(addr, code, prefix);
  default: // 0300
    // Decode by trailing octal digit
    switch (code & 07) {
    case 3:
      return decode_misc_hi<API>(addr, code, prefix);
    case 6:
      print_pgm_strtab<API>(MNE_STR, ALU_MNE[(code & 070) >> 3]);
      API::print_string(" A,");
      print_imm_byte<API>(addr + 1);
      return addr + 2;
    case 7:
      print_pgm_string<API>(MNE_STR_RST);
      API::print_string(" $");
      fmt_hex8(API::print_char, code & 070);
      return addr + 1;
    default: // 0, 1, 2, 4, 5
      if ((code & 01) == 01) {
        return decode_push_pop<API>(addr, code, prefix);
      } else {
        return decode_jp_cond<API>(addr, code);
      }
    }
  }
}

template <typename API>
uint16_t impl_dasm(uint16_t addr, uint16_t end) {
  for (;;) {
    // Print "addr:  opcode"
    fmt_hex16(API::print_char, addr);
    API::print_string(":  ");
    uint16_t next = decode_base<API>(addr);
    API::print_char('\n');
    // Do while end does not overlap with opcode
    if (uint16_t(end - addr) < uint16_t(next - addr)) {
      return next;
    }
    addr = next;
  }
}

} // namespace z80
} // namespace uMon
