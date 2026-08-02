//===-- MMIXInstrInfo.cpp - MMIX instruction information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXInstrInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MMIXGenInstrInfo.inc"

MMIXInstrInfo::MMIXInstrInfo(const MMIXSubtarget &STI)
    : MMIXGenInstrInfo(STI, RI), RI() {}

void MMIXInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MBBI,
                                const DebugLoc &DL, Register DstReg,
                                Register SrcReg, bool KillSrc,
                                bool RenamableDest, bool RenamableSrc) const {
  if (!MMIX::GPR64RegClass.contains(DstReg, SrcReg))
    report_fatal_error("MMIX cannot copy between these register classes");

  BuildMI(MBB, MBBI, DL, get(MMIX::ORI))
      .addReg(DstReg, RegState::Define | getRenamableRegState(RenamableDest))
      .addReg(SrcReg,
              getKillRegState(KillSrc) | getRenamableRegState(RenamableSrc))
      .addImm(0);
}

static bool isOctaSpillClass(const TargetRegisterClass *RC) {
  // FPR64 holds the full 64-bit floating representation. LDSF and STSF
  // perform conversions and therefore cannot preserve an FPR64 spill value.
  return MMIX::GPR64RegClass.hasSubClassEq(RC) ||
         MMIX::FPR64RegClass.hasSubClassEq(RC);
}

void MMIXInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MBBI,
                                        Register SrcReg, bool IsKill,
                                        int FrameIndex,
                                        const TargetRegisterClass *RC, Register,
                                        MachineInstr::MIFlag Flags) const {
  if (!isOctaSpillClass(RC))
    report_fatal_error("MMIX cannot spill this register class");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MBBI, DebugLoc(), get(MMIX::STOUI))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

void MMIXInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MBBI,
                                         Register DstReg, int FrameIndex,
                                         const TargetRegisterClass *RC,
                                         Register, unsigned SubReg,
                                         MachineInstr::MIFlag Flags) const {
  if (SubReg)
    report_fatal_error("MMIX does not support partial register reloads");
  if (!isOctaSpillClass(RC))
    report_fatal_error("MMIX cannot reload this register class");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MBBI, DebugLoc(), get(MMIX::LDOUI), DstReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO)
      .setMIFlag(Flags);
}

Register MMIXInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                            int &FrameIndex) const {
  TypeSize MemBytes = TypeSize::getZero();
  return isLoadFromStackSlot(MI, FrameIndex, MemBytes);
}

Register MMIXInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                            int &FrameIndex,
                                            TypeSize &MemBytes) const {
  if (MI.getOpcode() != MMIX::LDOUI || !MI.getOperand(1).isFI() ||
      !MI.getOperand(2).isImm() || MI.getOperand(2).getImm() != 0)
    return Register();

  FrameIndex = MI.getOperand(1).getIndex();
  MemBytes = TypeSize::getFixed(8);
  return MI.getOperand(0).getReg();
}

Register MMIXInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                           int &FrameIndex) const {
  TypeSize MemBytes = TypeSize::getZero();
  return isStoreToStackSlot(MI, FrameIndex, MemBytes);
}

Register MMIXInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                           int &FrameIndex,
                                           TypeSize &MemBytes) const {
  if (MI.getOpcode() != MMIX::STOUI || !MI.getOperand(1).isFI() ||
      !MI.getOperand(2).isImm() || MI.getOperand(2).getImm() != 0)
    return Register();

  FrameIndex = MI.getOperand(1).getIndex();
  MemBytes = TypeSize::getFixed(8);
  return MI.getOperand(0).getReg();
}
