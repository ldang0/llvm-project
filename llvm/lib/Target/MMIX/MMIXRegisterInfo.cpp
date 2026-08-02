//===-- MMIXRegisterInfo.cpp - MMIX register information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXRegisterInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXFrameLowering.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "MMIXGenRegisterInfo.inc"

MMIXRegisterInfo::MMIXRegisterInfo() : MMIXGenRegisterInfo(/*RA=*/0) {}

const MCPhysReg *
MMIXRegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  static const MCPhysReg CalleeSavedRegs[] = {0};
  return CalleeSavedRegs;
}

BitVector MMIXRegisterInfo::getReservedRegs(const MachineFunction &) const {
  // Task 31 replaces this conservative policy with the provisional ABI map.
  return BitVector(getNumRegs(), true);
}

bool MMIXRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator, int,
                                           unsigned, RegScavenger *) const {
  llvm_unreachable("MMIX frame-index elimination is not implemented");
}

Register MMIXRegisterInfo::getFrameRegister(const MachineFunction &) const {
  return MMIX::R254;
}
