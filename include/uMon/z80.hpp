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
  uMON_EXPECT_UINT(uint16_t, start, args);
  uMON_OPTION_UINT(uint16_t, size, args, 1);
  uint16_t next = impl_dasm<API>(start, start + size - 1);
  set_prompt<API>(args.command(), next);
}

} // namespace z80
} // namespace uMon
