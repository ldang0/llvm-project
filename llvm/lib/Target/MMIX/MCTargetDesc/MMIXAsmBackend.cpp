//===-- MMIXAsmBackend.cpp - MMIX assembler backend -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXFixupKinds.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>

using namespace llvm;

namespace {

class MMIXELFObjectWriter : public MCELFObjectTargetWriter {
public:
  MMIXELFObjectWriter()
      : MCELFObjectTargetWriter(/*Is64Bit=*/true, /*OSABI=*/0,
                                ELF::EM_NONE, /*HasRelocationAddend=*/false) {}

protected:
  unsigned getRelocType(const MCFixup &, const MCValue &,
                        bool) const override {
    return 0;
  }
};

class MMIXAsmBackend : public MCAsmBackend {
public:
  MMIXAsmBackend() : MCAsmBackend(llvm::endianness::big) {}
  ~MMIXAsmBackend() override = default;

  void applyFixup(const MCFragment &, const MCFixup &Fixup, const MCValue &,
                  uint8_t *Data, uint64_t Value, bool IsResolved) override {
    int64_t Delta = static_cast<int64_t>(Value);
    const bool IsBranch = Fixup.getKind() == MMIX::fixup_mmix_branch_forward ||
                          Fixup.getKind() == MMIX::fixup_mmix_branch_backward;
    const bool IsBackward =
        Fixup.getKind() == MMIX::fixup_mmix_branch_backward ||
        Fixup.getKind() == MMIX::fixup_mmix_jump_backward;
    const unsigned Width = IsBranch ? 16 : 24;
    const int64_t Min = IsBackward ? -(int64_t(1) << Width) : 0;
    const int64_t Max = IsBackward ? -1 : (int64_t(1) << Width) - 1;

    if (!IsResolved) {
      getContext().reportError(
          Fixup.getLoc(),
          "unresolved MMIX PC-relative fixup requires relocation support");
      return;
    }
    if ((Delta & 3) != 0) {
      getContext().reportError(
          Fixup.getLoc(), "MMIX PC-relative fixup is not instruction aligned");
      return;
    }
    Delta /= 4;
    if (Delta < Min || Delta > Max) {
      getContext().reportError(Fixup.getLoc(),
                               "MMIX PC-relative fixup is out of range");
      return;
    }

    const uint32_t Encoded =
        static_cast<uint32_t>(Delta) & ((uint32_t(1) << Width) - 1);
    uint32_t Word = support::endian::read32be(Data);
    const unsigned Shift = 0;
    const uint32_t Mask = ((uint32_t(1) << Width) - 1) << Shift;
    Word = (Word & ~(Mask)) | (Encoded << Shift);
    support::endian::write32be(Data, Word);
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return std::make_unique<MMIXELFObjectWriter>();
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    static const MCFixupKindInfo Infos[MMIX::NumTargetFixupKinds] = {
        {"fixup_mmix_branch_forward", 0, 16, 0},
        {"fixup_mmix_branch_backward", 0, 16, 0},
        {"fixup_mmix_jump_forward", 0, 24, 0},
        {"fixup_mmix_jump_backward", 0, 24, 0},
    };

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    return Infos[Kind - FirstTargetFixupKind];
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *) const override {
    if (Count % 4 != 0)
      return false;
    static constexpr char Zero[4] = {0, 0, 0, 0};
    for (uint64_t I = 0; I < Count; I += 4)
      OS.write(Zero, sizeof(Zero));
    return true;
  }
};

} // namespace

MCAsmBackend *llvm::createMMIXAsmBackend(const Target &,
                                         const MCSubtargetInfo &,
                                         const MCRegisterInfo &,
                                         const MCTargetOptions &) {
  return new MMIXAsmBackend();
}
