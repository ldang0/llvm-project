//===-- MMIXELFObjectWriter.cpp - MMIX ELF writer ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class MMIXELFObjectWriter : public MCELFObjectTargetWriter {
public:
  MMIXELFObjectWriter()
      : MCELFObjectTargetWriter(
            /*Is64Bit=*/true, ELF::ELFOSABI_NONE, ELF::EM_MMIX,
            /*HasRelocationAddend=*/true, /*ABIVersion=*/0) {}

protected:
  unsigned getRelocType(const MCFixup &, const MCValue &, bool) const override {
    report_fatal_error("MMIX ELF relocations are not implemented");
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter> llvm::createMMIXELFObjectWriter() {
  return std::make_unique<MMIXELFObjectWriter>();
}
