//===-- MMIXISelLowering.h - MMIX DAG lowering interface ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXISELLOWERING_H
#define LLVM_LIB_TARGET_MMIX_MMIXISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class MMIXSubtarget;

class MMIXTargetLowering final : public TargetLowering {
public:
  MMIXTargetLowering(const TargetMachine &TM, const MMIXSubtarget &STI);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXISELLOWERING_H
