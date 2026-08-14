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
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
unsigned getRelocationFieldSize(RelType type) {
  switch (type) {
  case R_MMIX_8:
    return 1;
  case R_MMIX_16:
    return 2;
  case R_MMIX_24:
  case R_MMIX_32:
    return 4;
  case R_MMIX_64:
    return 8;
  default:
    return 0;
  }
}

void checkMMIXBitfield(Ctx &ctx, uint8_t *loc, uint64_t val, unsigned bits,
                       const Relocation &rel) {
  uint64_t mask = maxUIntN(bits);
  if (val > mask && val < ~mask)
    reportRangeError(ctx, loc, rel, Twine(static_cast<int64_t>(val)),
                     -static_cast<int64_t>(uint64_t(1) << bits), mask);
}

class MMIX final : public TargetInfo {
public:
  MMIX(Ctx &ctx);

  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  int64_t getImplicitAddend(const uint8_t *buf,
                            RelType type) const override;
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
  switch (type) {
  case R_MMIX_NONE:
    return R_NONE;
  case R_MMIX_8:
  case R_MMIX_16:
  case R_MMIX_24:
  case R_MMIX_32:
  case R_MMIX_64:
    return R_ABS;
  default:
    break;
  }

  Err(ctx) << getErrorLoc(ctx, loc) << "unsupported relocation " << type
           << " against symbol " << &s;
  return R_NONE;
}

int64_t MMIX::getImplicitAddend(const uint8_t *buf, RelType type) const {
  Err(ctx) << getErrorLoc(ctx, buf) << "MMIX supports only RELA relocations; "
           << type << " has no explicit addend";
  return 0;
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

    if (unsigned size = getRelocationFieldSize(type)) {
      uint64_t offset = it->r_offset;
      if (offset >= sec.getSize()) {
        Err(ctx) << &sec << ": relocation " << type << " offset " << offset
                 << " is outside the section";
        continue;
      }
      if (size > sec.getSize() - offset) {
        Err(ctx) << &sec << ": relocation " << type << " field at offset "
                 << offset << " extends past the end of the section";
        continue;
      }
    }
    rs.scan<ELFT, RelTy>(it, type, rs.getAddend<ELFT>(*it, type));
  }
}

void MMIX::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_MMIX_NONE:
    return;
  case R_MMIX_8:
    checkMMIXBitfield(ctx, loc, val, 8, rel);
    *loc = val;
    return;
  case R_MMIX_16:
    checkMMIXBitfield(ctx, loc, val, 16, rel);
    write16be(loc, val);
    return;
  case R_MMIX_24:
    checkMMIXBitfield(ctx, loc, val, 24, rel);
    write32be(loc, (read32be(loc) & 0xff000000) | (val & 0xffffff));
    return;
  case R_MMIX_32:
    checkMMIXBitfield(ctx, loc, val, 32, rel);
    write32be(loc, val);
    return;
  case R_MMIX_64:
    write64be(loc, val);
    return;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unsupported relocation " << rel.type;
  }
}

void elf::setMMIXTargetInfo(Ctx &ctx) { ctx.target.reset(new MMIX(ctx)); }
