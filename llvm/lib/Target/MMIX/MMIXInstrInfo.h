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

  void loadImmediate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                     const DebugLoc &DL, Register DstReg, uint64_t Value,
                     MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const;
  void adjustReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                 const DebugLoc &DL, Register DstReg, Register SrcReg,
                 int64_t Amount,
                 MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const;

  bool expandPostRAPseudo(MachineInstr &MI) const override;

  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;
  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const override;
  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;
  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;
  bool reverseBranchCondition(
      SmallVectorImpl<MachineOperand> &Cond) const override;
  bool isBranchOffsetInRange(unsigned BranchOpc,
                             int64_t BrOffset) const override;
  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;
  MachineBasicBlock *
  getBranchDestBlock(const MachineInstr &MI) const override;
  void insertIndirectBranch(MachineBasicBlock &MBB,
                            MachineBasicBlock &NewDestBB,
                            MachineBasicBlock &RestoreBB, const DebugLoc &DL,
                            int64_t BrOffset = 0,
                            RegScavenger *RS = nullptr) const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   const DebugLoc &DL, Register DstReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;

  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, Register DstReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  Register isLoadFromStackSlot(const MachineInstr &MI,
                               int &FrameIndex) const override;
  Register isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex,
                               TypeSize &MemBytes) const override;
  Register isStoreToStackSlot(const MachineInstr &MI,
                              int &FrameIndex) const override;
  Register isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex,
                              TypeSize &MemBytes) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXINSTRINFO_H
