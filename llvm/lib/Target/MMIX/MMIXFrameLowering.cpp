//===-- MMIXFrameLowering.cpp - MMIX frame lowering ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXFrameLowering.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

MMIXFrameLowering::MMIXFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(8), /*LocalAreaOffset=*/0,
                          Align(8)) {}

void MMIXFrameLowering::emitPrologue(MachineFunction &,
                                     MachineBasicBlock &) const {
  llvm_unreachable("MMIX prologue emission is not implemented");
}

void MMIXFrameLowering::emitEpilogue(MachineFunction &,
                                     MachineBasicBlock &) const {
  llvm_unreachable("MMIX epilogue emission is not implemented");
}

bool MMIXFrameLowering::hasFPImpl(const MachineFunction &) const {
  return false;
}
