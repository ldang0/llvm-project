//===- MMIX.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RelocScan.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"

using namespace llvm;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class MMIX final : public TargetInfo {
public:
  MMIX(Ctx &ctx);

  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  template <class ELFT, class RelTy>
  void scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels,
                       unsigned shard);
  void scanSection(InputSectionBase &sec, unsigned shard) override {
    elf::scanSection1<MMIX, ELF64BE>(*this, sec, shard);
  }
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

MMIX::MMIX(Ctx &ctx) : TargetInfo(ctx) {
  if (ctx.arg.ekind != ELF64BEKind)
    ErrAlways(ctx) << "MMIX supports only ELF64 big-endian input and output";

  if (ctx.arg.shared)
    ErrAlways(ctx) << "MMIX does not support shared object output";
  else if (ctx.arg.pie)
    ErrAlways(ctx) << "MMIX does not support PIE output";
  else if (ctx.arg.relocatable)
    ErrAlways(ctx) << "MMIX lld supports only static executable output";

  if (!ctx.arg.dynamicLinker.empty())
    ErrAlways(ctx) << "MMIX does not support a dynamic linker";
  if (ctx.arg.oFormatBinary)
    ErrAlways(ctx) << "MMIX lld supports only ELF output";
  if (ctx.arg.osabi != ELFOSABI_NONE)
    ErrAlways(ctx) << "MMIX supports only the System V ELF OSABI";
  if (!ctx.sharedFiles.empty())
    ErrAlways(ctx) << "MMIX does not support dynamic shared object inputs";
  for (InputFile *file : ctx.objectFiles)
    if (file->abiVersion != 0)
      ErrAlways(ctx) << file << ": unsupported MMIX ELF ABI version "
                     << static_cast<unsigned>(file->abiVersion);
}

RelExpr MMIX::getRelExpr(RelType type, const Symbol &s,
                         const uint8_t *loc) const {
  if (type == R_MMIX_NONE)
    return R_NONE;

  Err(ctx) << getErrorLoc(ctx, loc) << "unsupported relocation " << type
           << " against symbol " << &s;
  return R_NONE;
}

template <class ELFT, class RelTy>
void MMIX::scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels,
                           unsigned shard) {
  RelocScan rs(ctx, &sec, shard);
  sec.relocations.reserve(rels.size());

  for (auto it = rels.begin(); it != rels.end(); ++it) {
    RelType type = it->getType(false);
    if (type == R_MMIX_NONE)
      continue;
    rs.scan<ELFT, RelTy>(it, type, rs.getAddend<ELFT>(*it, type));
  }
}

void MMIX::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  if (rel.type != R_MMIX_NONE)
    Err(ctx) << getErrorLoc(ctx, loc) << "unsupported relocation " << rel.type;
}

void elf::setMMIXTargetInfo(Ctx &ctx) { ctx.target.reset(new MMIX(ctx)); }
