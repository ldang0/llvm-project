//===-- MMIXFrameLowering.h - MMIX frame lowering -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXFRAMELOWERING_H
#define LLVM_LIB_TARGET_MMIX_MMIXFRAMELOWERING_H

#include "MMIXTailCall.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class MMIXFrameLowering final : public TargetFrameLowering {
  void emitCallerStateRestore(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              const DebugLoc &DL) const;

public:
  MMIXFrameLowering();

  bool hasReservedCallFrame(const MachineFunction &MF) const override;
  bool enableCFIFixup(const MachineFunction &MF) const override;
  StackOffset getFrameIndexReference(const MachineFunction &MF, int FI,
                                     Register &FrameReg) const override;
  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS = nullptr) const override;
  void processFunctionBeforeFrameFinalized(
      MachineFunction &MF, RegScavenger *RS = nullptr) const override;
  MMIXTailCallFrameState analyzeTailCallFrame(const MachineFunction &MF) const;
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXFRAMELOWERING_H
