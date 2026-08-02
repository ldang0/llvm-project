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
    : MMIXGenInstrInfo(STI, RI, MMIX::ADJCALLSTACKDOWN, MMIX::ADJCALLSTACKUP),
      RI() {}

static int getSingleWydeIndex(uint64_t Value) {
  int Index = -1;
  for (int I = 0; I != 4; ++I) {
    if (((Value >> (I * 16)) & 0xffff) == 0)
      continue;
    if (Index != -1)
      return -1;
    Index = I;
  }
  return Index == -1 ? 0 : Index;
}

void MMIXInstrInfo::loadImmediate(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MBBI,
                                  const DebugLoc &DL, Register DstReg,
                                  uint64_t Value,
                                  MachineInstr::MIFlag Flags) const {
  static constexpr unsigned SetOpcodes[] = {MMIX::SETL, MMIX::SETML,
                                            MMIX::SETMH, MMIX::SETH};
  static constexpr unsigned IncOpcodes[] = {MMIX::INCL, MMIX::INCML,
                                            MMIX::INCMH, MMIX::INCH};

  // Handle compact signed, shifted-wyde, and complemented forms before the
  // generic SET-plus-INC sequence.
  int64_t SignedValue = static_cast<int64_t>(Value);
  if (SignedValue < 0 && SignedValue >= -255) {
    BuildMI(MBB, MBBI, DL, get(MMIX::NEGUI), DstReg)
        .addImm(0)
        .addImm(-SignedValue)
        .setMIFlag(Flags);
    return;
  }

  auto EmitSingleWyde = [&](uint64_t WydeValue) {
    int Index = getSingleWydeIndex(WydeValue);
    assert(Index >= 0 && "value is not a single wyde");
    BuildMI(MBB, MBBI, DL, get(SetOpcodes[Index]), DstReg)
        .addImm((WydeValue >> (Index * 16)) & 0xffff)
        .setMIFlag(Flags);
  };

  if (getSingleWydeIndex(Value) >= 0) {
    EmitSingleWyde(Value);
    return;
  }

  uint64_t Magnitude = 0 - Value;
  if (SignedValue < 0 && getSingleWydeIndex(Magnitude) >= 0) {
    EmitSingleWyde(Magnitude);
    BuildMI(MBB, MBBI, DL, get(MMIX::NEGU), DstReg)
        .addImm(0)
        .addReg(DstReg)
        .setMIFlag(Flags);
    return;
  }

  uint64_t Complement = ~Value;
  if (getSingleWydeIndex(Complement) >= 0) {
    EmitSingleWyde(Complement);
    BuildMI(MBB, MBBI, DL, get(MMIX::NORI), DstReg)
        .addReg(DstReg)
        .addImm(0)
        .setMIFlag(Flags);
    return;
  }

  bool First = true;
  for (int I = 0; I != 4; ++I) {
    uint64_t Part = (Value >> (I * 16)) & 0xffff;
    if (!Part)
      continue;
    BuildMI(MBB, MBBI, DL, get(First ? SetOpcodes[I] : IncOpcodes[I]), DstReg)
        .addImm(Part)
        .setMIFlag(Flags);
    First = false;
  }
}

bool MMIXInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  if (MI.getOpcode() != MMIX::LOAD_IMM64)
    return false;

  loadImmediate(*MI.getParent(), MI.getIterator(), MI.getDebugLoc(),
                MI.getOperand(0).getReg(), uint64_t(MI.getOperand(1).getImm()));
  MI.eraseFromParent();
  return true;
}

void MMIXInstrInfo::adjustReg(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              const DebugLoc &DL, Register DstReg,
                              Register SrcReg, int64_t Amount,
                              MachineInstr::MIFlag Flags) const {
  if (Amount == 0) {
    if (DstReg != SrcReg)
      BuildMI(MBB, MBBI, DL, get(MMIX::ORI), DstReg)
          .addReg(SrcReg)
          .addImm(0)
          .setMIFlag(Flags);
    return;
  }

  bool Subtract = Amount < 0;
  uint64_t Magnitude = Subtract ? 0 - uint64_t(Amount) : uint64_t(Amount);
  if (Magnitude <= 255) {
    BuildMI(MBB, MBBI, DL, get(Subtract ? MMIX::SUBUI : MMIX::ADDUI), DstReg)
        .addReg(SrcReg)
        .addImm(Magnitude)
        .setMIFlag(Flags);
    return;
  }

  loadImmediate(MBB, MBBI, DL, MMIX::R255, Magnitude, Flags);
  BuildMI(MBB, MBBI, DL, get(Subtract ? MMIX::SUBU : MMIX::ADDU), DstReg)
      .addReg(SrcReg)
      .addReg(MMIX::R255, RegState::Kill)
      .setMIFlag(Flags);
}

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
