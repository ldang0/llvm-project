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

MMIXRegisterInfo::MMIXRegisterInfo() : MMIXGenRegisterInfo(MMIX::RJ) {}

const MCPhysReg *
MMIXRegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  return CSR_MMIX_SaveList;
}

const uint32_t *
MMIXRegisterInfo::getCallPreservedMask(const MachineFunction &,
                                       CallingConv::ID CC) const {
  return CC == CallingConv::C ? CSR_MMIX_RegMask : nullptr;
}

BitVector MMIXRegisterInfo::getReservedRegs(const MachineFunction &) const {
  BitVector Reserved(getNumRegs(), true);

  for (MCPhysReg Reg : MMIX::GPR64CodeGenRegClass)
    Reserved.reset(Reg);

  return Reserved;
}

const TargetRegisterClass *
MMIXRegisterInfo::getPointerRegClass(unsigned Kind) const {
  assert(Kind == 0 && "MMIX has only one pointer register kind");
  return &MMIX::GPR64CodeGenRegClass;
}

bool MMIXRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator, int,
                                           unsigned, RegScavenger *) const {
  llvm_unreachable("MMIX frame-index elimination is not implemented");
}

Register MMIXRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return getFrameLowering(MF)->hasFP(MF) ? MMIX::R253 : MMIX::R254;
}
