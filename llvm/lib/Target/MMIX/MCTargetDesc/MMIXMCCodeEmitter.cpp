//===-- MMIXMCCodeEmitter.cpp - Convert MMIX MCInst to bytes --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXBaseInfo.h"
#include "MMIXFixupKinds.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/EndianStream.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace {

class MMIXMCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  const MCRegisterInfo &MRI;
  MCContext &Ctx;

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getPCRelativeOpValue(const MCInst &MI, unsigned OpNo,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  static MCFixupKind getPCRelativeFixup(uint64_t TSFlags) {
    const bool IsBackward = TSFlags & MMIXII::PCRelativeBackward;
    if (MMIXII::getPCRelativeWidth(TSFlags) == 16)
      return IsBackward ? MMIX::fixup_mmix_branch_backward
                        : MMIX::fixup_mmix_branch_forward;
    return IsBackward ? MMIX::fixup_mmix_jump_backward
                      : MMIX::fixup_mmix_jump_forward;
  }

public:
  MMIXMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), MRI(*Ctx.getRegisterInfo()), Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override {
    const uint32_t Word = getBinaryCodeForInstr(MI, Fixups, STI);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::big);
  }
};

unsigned
MMIXMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                     SmallVectorImpl<MCFixup> & /*Fixups*/,
                                     const MCSubtargetInfo & /*STI*/) const {
  if (MO.isReg())
    return MRI.getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  Ctx.reportError(MI.getLoc(),
                  "unresolved MMIX symbolic instruction operand requires "
                  "relocation support");
  return 0;
}

unsigned
MMIXMCCodeEmitter::getPCRelativeOpValue(const MCInst &MI, unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo & /*STI*/) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  const uint64_t TSFlags = MCII.get(MI.getOpcode()).TSFlags;
  const unsigned Width = MMIXII::getPCRelativeWidth(TSFlags);
  assert((Width == 16 || Width == 24) &&
         "unexpected MMIX PC-relative field width");

  if (MO.isExpr()) {
    const MCExpr *Expr = MO.getExpr();
    if (const auto *Specifier = dyn_cast<MCSpecifierExpr>(Expr)) {
      if (Specifier->getSpecifier() != MMIXII::S_GETA) {
        Ctx.reportError(MI.getLoc(),
                        "unsupported MMIX PC-relative expression specifier");
        return 0;
      }
      if (MI.getOpcode() != MMIX::GETA) {
        Ctx.reportError(MI.getLoc(),
                        "'%geta' expression requires a GETA instruction");
        return 0;
      }

      Expr = Specifier->getSubExpr();
      MCValue Target;
      if (!Expr->evaluateAsRelocatable(Target, nullptr) ||
          !Target.getAddSym() || Target.getSubSym()) {
        Ctx.reportError(
            Specifier->getLoc(),
            "expanding GETA requires one symbol plus an optional addend");
        return 0;
      }
      Fixups.push_back(
          MCFixup::create(0, Expr, MMIX::fixup_mmix_geta, /*IsPCRel=*/true));
      return 0;
    }
    Fixups.push_back(MCFixup::create(0, Expr, getPCRelativeFixup(TSFlags),
                                     /*IsPCRel=*/true));
    return 0;
  }

  if (!MO.isImm()) {
    Ctx.reportError(MI.getLoc(),
                    "MMIX PC-relative operand is not an immediate");
    return 0;
  }

  const bool IsBackward = TSFlags & MMIXII::PCRelativeBackward;
  const int64_t Min = IsBackward ? -(int64_t(1) << Width) : 0;
  const int64_t Max =
      IsBackward ? -1 : static_cast<int64_t>((uint64_t(1) << Width) - 1);
  if (MO.getImm() < Min || MO.getImm() > Max) {
    Ctx.reportError(MI.getLoc(), "MMIX PC-relative operand is out of range");
    return 0;
  }

  return static_cast<unsigned>(MO.getImm());
}

#include "MMIXGenMCCodeEmitter.inc"

} // namespace

MCCodeEmitter *llvm::createMMIXMCCodeEmitter(const MCInstrInfo &MCII,
                                             MCContext &Ctx) {
  return new MMIXMCCodeEmitter(MCII, Ctx);
}
