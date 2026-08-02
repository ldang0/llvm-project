//===-- MMIXMCCodeEmitter.cpp - Convert MMIX MCInst to bytes --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXFixupKinds.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace {

static unsigned getMMIXOpcode(const MCInstrInfo &MCII, const MCInst &MI) {
  return MCII.get(MI.getOpcode()).TSFlags & 0xff;
}

static bool isBranch(unsigned Opcode) {
  return Opcode >= 0x40 && Opcode <= 0x5f;
}

static bool isLongPCRelative(unsigned Opcode) {
  return Opcode >= 0xf0 && Opcode <= 0xf5;
}

static bool isBackward(unsigned Opcode) {
  return (Opcode >= 0x41 && Opcode <= 0x5f && (Opcode & 1)) || Opcode == 0xf1 ||
         Opcode == 0xf3 || Opcode == 0xf5;
}

class MMIXMCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  const MCRegisterInfo &MRI;
  MCContext &Ctx;

  unsigned getMachineOpValue(const MCOperand &MO) const {
    if (MO.isReg())
      return MRI.getEncodingValue(MO.getReg());
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm());
    return 0;
  }

  MCFixupKind getPCRelativeFixup(unsigned Opcode) const {
    if (isBranch(Opcode))
      return isBackward(Opcode) ? MMIX::fixup_mmix_branch_backward
                                : MMIX::fixup_mmix_branch_forward;
    return isBackward(Opcode) ? MMIX::fixup_mmix_jump_backward
                              : MMIX::fixup_mmix_jump_forward;
  }

  static unsigned getOperandShift(unsigned Opcode, unsigned Operand) {
    if (isBranch(Opcode))
      return Operand == 1 ? 0 : 16;
    if (Opcode == 0xf0 || Opcode == 0xf1)
      return 0;
    if (Opcode >= 0xf2 && Opcode <= 0xf5)
      return Operand == 0 ? 16 : 0;
    if (Opcode == 0xf8)
      return Operand == 0 ? 16 : 0;
    if (Opcode >= 0xe0 && Opcode <= 0xef && Operand == 1)
      return 0;
    if (Opcode == 0xfc || Opcode == 0xf9 || Opcode == 0xfb)
      return 0;
    if (Opcode == 0xfe && Operand == 1)
      return 0;
    if ((Opcode == 0xf6 || Opcode == 0xf7) && Operand == 1)
      return 0;
    return 16 - 8 * Operand;
  }

  static unsigned getOperandWidth(unsigned Opcode, unsigned Operand) {
    if (isBranch(Opcode) && Operand == 1)
      return 16;
    if (Opcode == 0xf0 || Opcode == 0xf1)
      return 24;
    if (Opcode >= 0xf2 && Opcode <= 0xf5 && Operand == 1)
      return 16;
    if (Opcode == 0xf8 && Operand == 1)
      return 16;
    if (Opcode >= 0xe0 && Opcode <= 0xef && Operand == 1)
      return 16;
    if (Opcode == 0xfc)
      return 24;
    return 8;
  }

public:
  MMIXMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), MRI(*Ctx.getRegisterInfo()), Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &) const override {
    const unsigned Opcode = getMMIXOpcode(MCII, MI);
    uint32_t Word = Opcode << 24;

    assert(MI.getNumOperands() <= 3 && "MMIX instructions have three fields");
    for (unsigned I = 0; I < MI.getNumOperands(); ++I) {
      const MCOperand &MO = MI.getOperand(I);
      const unsigned Shift = getOperandShift(Opcode, I);
      const unsigned Width = getOperandWidth(Opcode, I);
      const uint32_t Mask = (1u << Width) - 1;

      if (MO.isExpr()) {
        if (!isBranch(Opcode) && !isLongPCRelative(Opcode)) {
          Ctx.reportError(MI.getLoc(),
                          "MMIX expression operand is not relocatable");
          continue;
        }
        Fixups.push_back(
            MCFixup::create(0, MO.getExpr(), getPCRelativeFixup(Opcode), true));
        continue;
      }

      Word |= (getMachineOpValue(MO) & Mask) << Shift;
    }

    CB.push_back(static_cast<char>(Word >> 24));
    CB.push_back(static_cast<char>(Word >> 16));
    CB.push_back(static_cast<char>(Word >> 8));
    CB.push_back(static_cast<char>(Word));
  }
};

} // namespace

MCCodeEmitter *llvm::createMMIXMCCodeEmitter(const MCInstrInfo &MCII,
                                             MCContext &Ctx) {
  return new MMIXMCCodeEmitter(MCII, Ctx);
}
