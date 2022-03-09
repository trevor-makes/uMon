// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#pragma once

#include "z80/dasm.hpp"
#include "uCLI.hpp"

namespace uMon {
namespace z80 {

template <typename API>
void cmd_dasm(uCLI::Args args) {
  // Default size to one instruction if not provided
  uint16_t start, size = 1;
  if (!parse_unsigned<API>(args.next(), start)) { return; }
  if (args.has_next() && !parse_unsigned<API>(args.next(), size)) { return; }
  uint16_t next = impl_dasm<API>(start, start + size - 1);
  set_prompt<API>(args.command(), next);
}

} // namespace z80
} // namespace uMon
