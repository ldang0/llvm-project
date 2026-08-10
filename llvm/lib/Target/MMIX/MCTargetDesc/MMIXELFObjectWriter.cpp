//===-- MMIXELFObjectWriter.cpp - MMIX ELF writer ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXFixupKinds.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/ADT/Twine.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"

using namespace llvm;

namespace {

class MMIXELFObjectWriter : public MCELFObjectTargetWriter {
  unsigned rejectRelocation(const MCFixup &Fixup, const Twine &Message) const {
    reportError(Fixup.getLoc(), Message);
    // MCAssembler suppresses object emission after an error, so this recovery
    // value cannot become a relocation record.
    return 0;
  }

public:
  MMIXELFObjectWriter()
      : MCELFObjectTargetWriter(
            /*Is64Bit=*/true, ELF::ELFOSABI_NONE, ELF::EM_MMIX,
            /*HasRelocationAddend=*/true, /*ABIVersion=*/0) {}

protected:
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &,
                        bool IsPCRel) const override {
    unsigned AbsoluteType;
    unsigned PCRelativeType;
    switch (Fixup.getKind()) {
    case FK_Data_1:
      AbsoluteType = ELF::R_MMIX_8;
      PCRelativeType = ELF::R_MMIX_PC_8;
      break;
    case FK_Data_2:
      AbsoluteType = ELF::R_MMIX_16;
      PCRelativeType = ELF::R_MMIX_PC_16;
      break;
    case FK_Data_4:
      AbsoluteType = ELF::R_MMIX_32;
      PCRelativeType = ELF::R_MMIX_PC_32;
      break;
    case FK_Data_8:
      AbsoluteType = ELF::R_MMIX_64;
      PCRelativeType = ELF::R_MMIX_PC_64;
      break;
    case MMIX::fixup_mmix_branch_forward:
      return rejectRelocation(
          Fixup, "MMIX 16-bit forward PC-relative instruction relocation "
                 "is not implemented");
    case MMIX::fixup_mmix_branch_backward:
      return rejectRelocation(
          Fixup, "MMIX 16-bit backward PC-relative instruction relocation "
                 "is not implemented");
    case MMIX::fixup_mmix_jump_forward:
      return rejectRelocation(
          Fixup, "MMIX 24-bit forward PC-relative instruction relocation "
                 "is not implemented");
    case MMIX::fixup_mmix_jump_backward:
      return rejectRelocation(
          Fixup, "MMIX 24-bit backward PC-relative instruction relocation "
                 "is not implemented");
    case MMIX::fixup_mmix_data_24:
      return rejectRelocation(
          Fixup, "MMIX 24-in-32 absolute data relocation is not implemented");
    case MMIX::fixup_mmix_pcrel_24:
      return rejectRelocation(
          Fixup,
          "MMIX 24-in-32 PC-relative data relocation is not implemented");
    default:
      return rejectRelocation(Fixup, "MMIX ELF relocation is not implemented");
    }

    return IsPCRel ? PCRelativeType : AbsoluteType;
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter> llvm::createMMIXELFObjectWriter() {
  return std::make_unique<MMIXELFObjectWriter>();
}
