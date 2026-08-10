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
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/Endian.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace {

class MMIXAsmBackend : public MCAsmBackend {
public:
  MMIXAsmBackend() : MCAsmBackend(llvm::endianness::big) {}
  ~MMIXAsmBackend() override = default;

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    if (!IsResolved) {
      maybeAddReloc(F, Fixup, Target, Value, IsResolved);
      return;
    }

    const MCFixupKind Kind = Fixup.getKind();
    if (Kind < FirstTargetFixupKind) {
      unsigned Size;
      switch (Kind) {
      case FK_Data_1:
        Size = 1;
        break;
      case FK_Data_2:
        Size = 2;
        break;
      case FK_Data_4:
        Size = 4;
        break;
      case FK_Data_8:
        Size = 8;
        break;
      default:
        getContext().reportError(Fixup.getLoc(),
                                 "unsupported resolved MMIX data fixup");
        return;
      }
      assert(Fixup.getOffset() + Size <= F.getSize() &&
             "invalid MMIX data fixup offset");
      for (unsigned I = 0; I != Size; ++I)
        Data[I] |= static_cast<uint8_t>(Value >> ((Size - I - 1) * 8));
      return;
    }

    if (Kind == MMIX::fixup_mmix_data_24 || Kind == MMIX::fixup_mmix_pcrel_24) {
      assert(Fixup.getOffset() + 4 <= F.getSize() &&
             "invalid MMIX 24-in-32 data fixup offset");
      if (Value > 0xffffff && Value < 0xffffffffff000000ULL) {
        getContext().reportError(Fixup.getLoc(),
                                 "MMIX 24-bit data fixup is out of range");
        return;
      }

      uint32_t Word = support::endian::read32be(Data);
      Word = (Word & 0xff000000) | (static_cast<uint32_t>(Value) & 0xffffff);
      support::endian::write32be(Data, Word);
      return;
    }

    int64_t Delta = static_cast<int64_t>(Value);
    const bool IsBranch = Kind == MMIX::fixup_mmix_branch_forward ||
                          Kind == MMIX::fixup_mmix_branch_backward;
    const bool IsBackward = Kind == MMIX::fixup_mmix_branch_backward ||
                            Kind == MMIX::fixup_mmix_jump_backward;
    const unsigned Width = IsBranch ? 16 : 24;
    const int64_t Min = IsBackward ? -(int64_t(1) << Width) : 0;
    const int64_t Max = IsBackward ? -1 : (int64_t(1) << Width) - 1;

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
    return createMMIXELFObjectWriter();
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    static const MCFixupKindInfo Infos[MMIX::NumTargetFixupKinds] = {
        {"fixup_mmix_branch_forward", 0, 16, 0},
        {"fixup_mmix_branch_backward", 0, 16, 0},
        {"fixup_mmix_jump_forward", 0, 24, 0},
        {"fixup_mmix_jump_backward", 0, 24, 0},
        {"fixup_mmix_data_24", 0, 24, 0},
        {"fixup_mmix_pcrel_24", 0, 24, 0},
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
