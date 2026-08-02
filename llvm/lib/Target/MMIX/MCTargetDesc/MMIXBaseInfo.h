//===-- MMIXBaseInfo.h - MMIX instruction metadata ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXBASEINFO_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXBASEINFO_H

#include <cstdint>

namespace llvm {
namespace MMIXII {

enum : uint64_t {
  OpcodeMask = 0xff,
  PCRelativeBackward = uint64_t(1) << 8,
  PCRelativeWidthShift = 9,
  PCRelativeWidthMask = uint64_t(0x1f) << PCRelativeWidthShift,
};

inline unsigned getPCRelativeWidth(uint64_t TSFlags) {
  return (TSFlags & PCRelativeWidthMask) >> PCRelativeWidthShift;
}

} // namespace MMIXII
} // namespace llvm

#endif
