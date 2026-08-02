//===-- MMIXSubtarget.h - Define Subtarget for MMIX ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXSUBTARGET_H
#define LLVM_LIB_TARGET_MMIX_MMIXSUBTARGET_H

#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "MMIXGenSubtargetInfo.inc"

namespace llvm {

class MMIXSubtarget : public MMIXGenSubtargetInfo {
  virtual void anchor();

#define GET_SUBTARGETINFO_MACRO(ATTRIBUTE, DEFAULT, GETTER)                  \
  bool ATTRIBUTE = DEFAULT;
#include "MMIXGenSubtargetInfo.inc"

public:
  MMIXSubtarget(const Triple &TT, StringRef CPU, StringRef FS);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const TargetRegisterInfo *getRegisterInfo() const override { return nullptr; }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXSUBTARGET_H
