// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#include "uMon.hpp"

#include <stdlib.h>

namespace uMon {

uint32_t parse_u32(const char* str) {
  switch (str[0])
  {
  case '$': // hex
    return strtoul(str + 1, nullptr, 16);
  case '&': // octal
    return strtoul(str + 1, nullptr, 8);
  case '%': // binary
    return strtoul(str + 1, nullptr, 2);
  default: // decimal
    return strtoul(str, nullptr, 10);
  }
}

} // namespace uMon
