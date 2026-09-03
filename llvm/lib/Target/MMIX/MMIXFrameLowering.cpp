//===-- MMIXFrameLowering.cpp - MMIX frame lowering ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXFrameLowering.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXInstrInfo.h"
#include "MMIXMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"

#include <iterator>

using namespace llvm;

static bool needsDebugCFI(const MachineFunction &MF) {
  return MF.getFunction().getSubprogram() != nullptr &&
         MF.getTarget().getMCAsmInfo().getOutputAssemblerDialect() ==
             MMIXII::CanonicalAsmVariant;
}

static void emitCFI(MachineFunction &MF, MachineBasicBlock &MBB,
                    MachineBasicBlock::iterator MBBI,
                    const MCCFIInstruction &Instruction,
                    MachineInstr::MIFlag Flag) {
  unsigned Index = MF.addFrameInst(Instruction);
  BuildMI(MBB, MBBI, DebugLoc(),
          MF.getSubtarget().getInstrInfo()->get(TargetOpcode::CFI_INSTRUCTION))
      .addCFIIndex(Index)
      .setMIFlag(Flag);
}

MMIXFrameLowering::MMIXFrameLowering()
    : TargetFrameLowering(StackGrowsDown, Align(8), /*LocalAreaOffset=*/0,
                          Align(8)) {}

bool MMIXFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  return !MF.getFrameInfo().hasVarSizedObjects();
}

StackOffset
MMIXFrameLowering::getFrameIndexReference(const MachineFunction &MF, int FI,
                                          Register &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int64_t Offset = MFI.getObjectOffset(FI) + MFI.getOffsetAdjustment();

  if (hasFP(MF)) {
    FrameReg = MMIX::R253;
  } else {
    FrameReg = MMIX::R254;
    Offset += MFI.getStackSize();
  }

  return StackOffset::getFixed(Offset);
}

void MMIXFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                             BitVector &SavedRegs,
                                             RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  // PUSHJ/PUSHGO and POP preserve local registers through the hardware
  // register stack, so they must never receive ordinary memory save slots.
  for (MCPhysReg Reg = MMIX::R0; Reg <= MMIX::R30; ++Reg)
    SavedRegs.reset(Reg);

  if (hasFP(MF))
    SavedRegs.set(MMIX::R253);

  if (MF.getFrameInfo().hasCalls() && needsDebugCFI(MF)) {
    auto *MMFI = MF.getInfo<MMIXMachineFunctionInfo>();
    if (!MMFI->hasDebugReturnAddressFrameIndex())
      MMFI->setDebugReturnAddressFrameIndex(
          MF.getFrameInfo().CreateStackObject(8, Align(8), false));
  }
}

void MMIXFrameLowering::processFunctionBeforeFrameFinalized(
    MachineFunction &MF, RegScavenger *) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  for (int FI = 0, End = MFI.getObjectIndexEnd(); FI != End; ++FI) {
    if (!MFI.isDeadObjectIndex(FI) && MFI.getObjectAlign(FI) < Align(4))
      MFI.setObjectAlignment(FI, Align(4));
  }
}

static void validateFrame(const MachineFunction &MF,
                          const MMIXFrameLowering &TFI) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.getMaxAlign() > TFI.getStackAlign())
    reportFatalUsageError(Twine("MMIX does not support stack realignment in ") +
                          "function '" + MF.getName() + "'");
  if (MF.shouldSplitStack())
    reportFatalUsageError(
        Twine("MMIX does not support split stacks in function '") +
        MF.getName() + "'");
  if (MF.getFunction().hasFnAttribute("probe-stack"))
    reportFatalUsageError(
        Twine("MMIX does not support stack probing in function '") +
        MF.getName() + "'");
  if (MF.getFunction().hasUWTable())
    reportFatalUsageError(
        Twine("MMIX does not support runtime unwind tables in function '") +
        MF.getName() + "'");
}

MMIXTailCallFrameState
MMIXFrameLowering::analyzeTailCallFrame(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  MMIXTailCallFrameState State;
  State.HasDynamicStack = MFI.hasVarSizedObjects();
  State.RequiresStackRealignment = MFI.getMaxAlign() > getStackAlign();
  State.CanRestoreFrame =
      !MFI.hasOpaqueSPAdjustment() && !MF.shouldSplitStack() &&
      !MF.getFunction().hasFnAttribute("probe-stack") &&
      MFI.getStackSize() <= uint64_t(INT64_MAX);
  return State;
}

void MMIXFrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  assert(&MBB == &MF.front() && "MMIX does not support shrink wrapping");
  validateFrame(MF, *this);

  MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t StackSize = MFI.getStackSize();
  if (StackSize > uint64_t(INT64_MAX))
    report_fatal_error("MMIX stack frame is too large");

  const auto *TII = MF.getSubtarget().getInstrInfo();
  const auto &MMIXII = *static_cast<const MMIXInstrInfo *>(TII);
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL;
  const auto *TRI = MF.getSubtarget().getRegisterInfo();
  const bool NeedsCFI = needsDebugCFI(MF);

  if (NeedsCFI)
    emitCFI(MF, MBB, MBBI,
            MCCFIInstruction::cfiDefCfa(
                nullptr, TRI->getDwarfRegNum(MMIX::R254, true), 0),
            MachineInstr::FrameSetup);

  if (MFI.hasCalls())
    BuildMI(MBB, MBBI, DL, TII->get(MMIX::GET), MMIX::R30)
        .addReg(MMIX::RJ)
        .setMIFlag(MachineInstr::FrameSetup);

  MMIXII.adjustReg(MBB, MBBI, DL, MMIX::R254, MMIX::R254, -int64_t(StackSize),
                   MachineInstr::FrameSetup);

  if (NeedsCFI) {
    emitCFI(MF, MBB, MBBI,
            MCCFIInstruction::cfiDefCfaOffset(nullptr, StackSize),
            MachineInstr::FrameSetup);

    auto *MMFI = MF.getInfo<MMIXMachineFunctionInfo>();
    if (MMFI->hasDebugReturnAddressFrameIndex()) {
      int FrameIndex = MMFI->getDebugReturnAddressFrameIndex();
      TII->storeRegToStackSlot(MBB, MBBI, MMIX::R30, false, FrameIndex,
                               &MMIX::GPR64RegClass, Register(),
                               MachineInstr::FrameSetup);
      emitCFI(MF, MBB, MBBI,
              MCCFIInstruction::createOffset(
                  nullptr, TRI->getDwarfRegNum(MMIX::RJ, true),
                  MFI.getObjectOffset(FrameIndex)),
              MachineInstr::FrameSetup);
    }
  }

  if (hasFP(MF)) {
    // The generic callee-save spill of the old frame pointer is at the entry
    // of the block. Establish the new frame pointer after that store.
    std::advance(MBBI, MFI.getCalleeSavedInfo().size());
    if (NeedsCFI)
      for (const CalleeSavedInfo &CSI : MFI.getCalleeSavedInfo())
        emitCFI(MF, MBB, MBBI,
                MCCFIInstruction::createOffset(
                    nullptr, TRI->getDwarfRegNum(CSI.getReg(), true),
                    MFI.getObjectOffset(CSI.getFrameIdx())),
                MachineInstr::FrameSetup);
    MMIXII.adjustReg(MBB, MBBI, DL, MMIX::R253, MMIX::R254, int64_t(StackSize),
                     MachineInstr::FrameSetup);
    if (NeedsCFI)
      emitCFI(MF, MBB, MBBI,
              MCCFIInstruction::cfiDefCfa(
                  nullptr, TRI->getDwarfRegNum(MMIX::R253, true), 0),
              MachineInstr::FrameSetup);
  }
}

void MMIXFrameLowering::emitCallerStateRestore(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI, const DebugLoc &DL) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize > uint64_t(INT64_MAX))
    report_fatal_error("MMIX stack frame is too large");

  const auto &MMIXII =
      *static_cast<const MMIXInstrInfo *>(MF.getSubtarget().getInstrInfo());
  const auto *TRI = MF.getSubtarget().getRegisterInfo();
  const bool NeedsCFI = needsDebugCFI(MF);
  if (MF.getFrameInfo().hasVarSizedObjects()) {
    MachineBasicBlock::iterator FirstRestore = MBBI;
    ArrayRef<CalleeSavedInfo> CSI = MF.getFrameInfo().getCalleeSavedInfo();
    if (!CSI.empty())
      FirstRestore = std::prev(MBBI, CSI.size());
    MMIXII.adjustReg(MBB, FirstRestore, DL, MMIX::R254, MMIX::R253, 0,
                     MachineInstr::FrameDestroy);
    if (NeedsCFI)
      emitCFI(MF, MBB, FirstRestore,
              MCCFIInstruction::cfiDefCfa(
                  nullptr, TRI->getDwarfRegNum(MMIX::R254, true), 0),
              MachineInstr::FrameDestroy);
  } else {
    if (NeedsCFI && hasFP(MF))
      emitCFI(MF, MBB, MBBI,
              MCCFIInstruction::cfiDefCfa(
                  nullptr, TRI->getDwarfRegNum(MMIX::R254, true), StackSize),
              MachineInstr::FrameDestroy);
    MMIXII.adjustReg(MBB, MBBI, DL, MMIX::R254, MMIX::R254,
                     int64_t(StackSize), MachineInstr::FrameDestroy);
    if (NeedsCFI)
      emitCFI(MF, MBB, MBBI, MCCFIInstruction::cfiDefCfaOffset(nullptr, 0),
              MachineInstr::FrameDestroy);
  }
  if (NeedsCFI)
    for (const CalleeSavedInfo &CSI : MF.getFrameInfo().getCalleeSavedInfo())
      emitCFI(MF, MBB, MBBI,
              MCCFIInstruction::createRestore(
                  nullptr, TRI->getDwarfRegNum(CSI.getReg(), true)),
              MachineInstr::FrameDestroy);
  if (MF.getFrameInfo().hasCalls())
    BuildMI(MBB, MBBI, DL, MMIXII.get(MMIX::PUT), MMIX::RJ)
        .addReg(MMIX::R30)
        .setMIFlag(MachineInstr::FrameDestroy);
  if (NeedsCFI && MF.getFrameInfo().hasCalls())
    emitCFI(MF, MBB, MBBI,
            MCCFIInstruction::createRestore(
                nullptr, TRI->getDwarfRegNum(MMIX::RJ, true)),
            MachineInstr::FrameDestroy);
}

static bool isTailTransfer(const MachineInstr &MI) {
  return MI.getOpcode() == MMIX::INDIRECT_TAIL_STATE ||
         MI.getOpcode() == MMIX::DIRECT_TAIL_STATE ||
         MI.getOpcode() == MMIX::MATERIALIZED_DIRECT_TAIL_STATE;
}

void MMIXFrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
  DebugLoc DL = MBBI == MBB.end() ? DebugLoc() : MBBI->getDebugLoc();
  if (MBBI != MBB.end() && isTailTransfer(*MBBI)) {
    MMIXTailCallFrameState State = analyzeTailCallFrame(MF);
    if (!State.isEligible())
      reportFatalUsageError(
          Twine("MMIX tail transfer is ineligible: ") +
          MMIXTailCallEligibility(State.getReason()).getReasonText() +
          " in function '" + MF.getName() + "'");
  }
  emitCallerStateRestore(MF, MBB, MBBI, DL);
}

MachineBasicBlock::iterator MMIXFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  if (!hasReservedCallFrame(MF)) {
    int64_t Amount = alignTo(MI->getOperand(0).getImm(), getStackAlign());
    if (MI->getOpcode() == MMIX::ADJCALLSTACKDOWN)
      Amount = -Amount;
    const auto &MMIXII = *static_cast<const MMIXInstrInfo *>(
        MF.getSubtarget().getInstrInfo());
    MMIXII.adjustReg(MBB, MI, MI->getDebugLoc(), MMIX::R254, MMIX::R254,
                     Amount);
  }
  return MBB.erase(MI);
}

bool MMIXFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken();
}
