//===-- MMIXInstrInfo.h - MMIX instruction information --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXINSTRINFO_H
#define LLVM_LIB_TARGET_MMIX_MMIXINSTRINFO_H

#include "MMIXRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "MMIXGenInstrInfo.inc"

namespace llvm {

class MMIXSubtarget;

class MMIXInstrInfo final : public MMIXGenInstrInfo {
  MMIXRegisterInfo RI;

public:
  explicit MMIXInstrInfo(const MMIXSubtarget &STI);

  const MMIXRegisterInfo &getRegisterInfo() const { return RI; }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXINSTRINFO_H
