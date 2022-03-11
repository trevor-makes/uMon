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
uint8_t impl_asm(uint16_t addr, uint8_t mne) {
  switch (mne) {
  case MNE_CCF:
    API::write_byte(addr, 0x3F);
    return 1;
  case MNE_CPD:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xA9);
    return 2;
  case MNE_CPDR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xB9);
    return 2;
  case MNE_CPI:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xA1);
    return 2;
  case MNE_CPIR:
    API::write_byte(addr, 0xED);
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
  case MNE_EI:
    API::write_byte(addr, 0xFB);
    return 1;
  case MNE_EXX:
    API::write_byte(addr, 0xD9);
    return 1;
  case MNE_HALT:
    API::write_byte(addr, 0x76);
    return 1;
  case MNE_IND:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xAA);
    return 2;
  case MNE_INDR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xBA);
    return 2;
  case MNE_INI:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xA2);
    return 2;
  case MNE_INIR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xB2);
    return 2;
  case MNE_LDD:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xA8);
    return 2;
  case MNE_LDDR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xB8);
    return 2;
  case MNE_LDI:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xA0);
    return 2;
  case MNE_LDIR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xB0);
    return 2;
  case MNE_NEG:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0x44);
    return 2;
  case MNE_NOP:
    API::write_byte(addr, 0x00);
    return 1;
  case MNE_OTDR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xBB);
    return 2;
  case MNE_OTIR:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xB3);
    return 2;
  case MNE_OUTD:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xAB);
    return 2;
  case MNE_OUTI:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0xA3);
    return 2;
  case MNE_RETI:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0x4D);
    return 2;
  case MNE_RETN:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0x45);
    return 2;
  case MNE_RLA:
    API::write_byte(addr, 0x17);
    return 1;
  case MNE_RLCA:
    API::write_byte(addr, 0x07);
    return 1;
  case MNE_RLD:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0x6F);
    return 2;
  case MNE_RRA:
    API::write_byte(addr, 0x1F);
    return 1;
  case MNE_RRCA:
    API::write_byte(addr, 0x0F);
    return 1;
  case MNE_RRD:
    API::write_byte(addr, 0xED);
    API::write_byte(addr + 1, 0x67);
    return 2;
  case MNE_SCF:
    API::write_byte(addr, 0x37);
    return 1;
  }
  return 0;
}

} // namespace z80
} // namespace uMon
