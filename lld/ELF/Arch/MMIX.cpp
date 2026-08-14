//===- MMIX.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "OutputSections.h"
#include "RelocScan.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"
#include <optional>

using namespace llvm;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
constexpr uint32_t swymInstruction = 0xfd000000;
constexpr uint32_t setlOpcode = 0xe3;
constexpr uint32_t inchOpcode = 0xe4;
constexpr uint32_t incmhOpcode = 0xe5;
constexpr uint32_t incmlOpcode = 0xe6;
constexpr uint32_t goImmediateOpcode = 0x9f;
constexpr uint32_t pushgoImmediateOpcode = 0xbf;
constexpr uint32_t pc19ValueMask = 0xffff;
constexpr uint32_t pc27ValueMask = 0xffffff;

struct MMIXRelaxationSequence {
  uint8_t size;
  uint8_t opcodeMask;
  uint8_t opcode;
};

std::optional<MMIXRelaxationSequence> getRelaxationSequence(RelType type) {
  switch (type) {
  case R_MMIX_GETA:
    return MMIXRelaxationSequence{16, 0xfe, 0xf4};
  case R_MMIX_CBRANCH:
    return MMIXRelaxationSequence{24, 0xe0, 0x40};
  case R_MMIX_PUSHJ:
    return MMIXRelaxationSequence{20, 0xfe, 0xf2};
  case R_MMIX_JMP:
    return MMIXRelaxationSequence{20, 0xfe, 0xf0};
  default:
    return std::nullopt;
  }
}

StringRef getUnsupportedRelocationReason(RelType type) {
  switch (type) {
  case R_MMIX_GETA_1:
  case R_MMIX_GETA_2:
  case R_MMIX_GETA_3:
  case R_MMIX_CBRANCH_J:
  case R_MMIX_CBRANCH_1:
  case R_MMIX_CBRANCH_2:
  case R_MMIX_CBRANCH_3:
  case R_MMIX_PUSHJ_1:
  case R_MMIX_PUSHJ_2:
  case R_MMIX_PUSHJ_3:
  case R_MMIX_JMP_1:
  case R_MMIX_JMP_2:
  case R_MMIX_JMP_3:
    return "GNU relaxation continuation cannot be used as standalone input";
  case R_MMIX_GNU_VTINHERIT:
  case R_MMIX_GNU_VTENTRY:
    return "requires GNU vtable metadata support";
  case R_MMIX_REG_OR_BYTE:
  case R_MMIX_REG:
  case R_MMIX_BASE_PLUS_OFFSET:
  case R_MMIX_LOCAL:
    return "requires MMIX register-model support";
  case R_MMIX_PUSHJ_STUBBABLE:
    return "requires MMIX range-extension stub support";
  default:
    return {};
  }
}

bool isTerminalRelocation(RelType type) {
  return type == R_MMIX_ADDR19 || type == R_MMIX_ADDR27;
}

unsigned getRelocationFieldSize(RelType type) {
  switch (type) {
  case R_MMIX_8:
  case R_MMIX_PC_8:
    return 1;
  case R_MMIX_16:
  case R_MMIX_PC_16:
    return 2;
  case R_MMIX_24:
  case R_MMIX_32:
  case R_MMIX_PC_24:
  case R_MMIX_PC_32:
  case R_MMIX_ADDR19:
  case R_MMIX_ADDR27:
    return 4;
  case R_MMIX_64:
  case R_MMIX_PC_64:
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

void relocateMMIXTerminal(uint8_t *loc, Ctx &ctx, uint64_t val,
                          uint32_t valueMask, const Relocation &rel) {
  constexpr int64_t instructionSize = 4;
  int64_t delta = static_cast<int64_t>(val);
  int64_t min = -static_cast<int64_t>(valueMask + 1) * instructionSize;
  int64_t max = static_cast<int64_t>(valueMask) * instructionSize;
  checkAlignment(ctx, loc, val, instructionSize, rel);
  if (delta < min || delta > max)
    reportRangeError(ctx, loc, rel, Twine(delta), min, max);

  constexpr uint32_t directionMask = uint32_t(1) << 24;
  uint32_t word = read32be(loc) & ~(directionMask | valueMask);
  if (delta < 0)
    word |= directionMask;
  word |= static_cast<uint64_t>(delta / instructionSize) & valueMask;
  write32be(loc, word);
}

bool isDirectMMIXTransfer(uint64_t target, uint64_t place, uint32_t valueMask) {
  if (target & 3)
    return false;
  int64_t delta = static_cast<int64_t>(target - place);
  constexpr int64_t instructionSize = 4;
  int64_t min = -static_cast<int64_t>(valueMask + 1) * instructionSize;
  int64_t max = static_cast<int64_t>(valueMask) * instructionSize;
  return delta >= min && delta <= max;
}

void writeAbsoluteAddress(uint8_t *loc, uint8_t reg, uint64_t value) {
  write32be(loc, (setlOpcode << 24) | (uint32_t(reg) << 16) | (value & 0xffff));
  write32be(loc + 4, (incmlOpcode << 24) | (uint32_t(reg) << 16) |
                         ((value >> 16) & 0xffff));
  write32be(loc + 8, (incmhOpcode << 24) | (uint32_t(reg) << 16) |
                         ((value >> 32) & 0xffff));
  write32be(loc + 12, (inchOpcode << 24) | (uint32_t(reg) << 16) |
                          ((value >> 48) & 0xffff));
}

void relocateMMIXExpanded(uint8_t *loc, uint64_t value, const Relocation &rel) {
  constexpr uint8_t scratchRegister = 255;
  uint8_t originalX = loc[1];
  switch (rel.type) {
  case R_MMIX_GETA:
    writeAbsoluteAddress(loc, originalX, value);
    return;
  case R_MMIX_CBRANCH: {
    constexpr uint32_t conditionInversionBit = uint32_t(1) << 27;
    constexpr uint32_t predictionInversionBit = uint32_t(1) << 28;
    constexpr uint32_t branchFieldMask = 0xffff;
    constexpr uint32_t instructionsToSkip = 6;
    uint32_t branch = read32be(loc);
    branch ^= conditionInversionBit | predictionInversionBit;
    branch = (branch & ~branchFieldMask) | instructionsToSkip;
    write32be(loc, branch);
    writeAbsoluteAddress(loc + 4, scratchRegister, value);
    write32be(loc + 20, (goImmediateOpcode << 24) | 0xffff00);
    return;
  }
  case R_MMIX_PUSHJ:
    writeAbsoluteAddress(loc, scratchRegister, value);
    write32be(loc + 16, (pushgoImmediateOpcode << 24) |
                            (uint32_t(originalX) << 16) | 0xff00);
    return;
  case R_MMIX_JMP:
    writeAbsoluteAddress(loc, scratchRegister, value);
    write32be(loc + 16, (goImmediateOpcode << 24) | 0xffff00);
    return;
  default:
    llvm_unreachable("not an expanding MMIX relocation");
  }
}

class MMIX final : public TargetInfo {
public:
  MMIX(Ctx &ctx);

  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
  template <class ELFT, class RelTy>
  void scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels,
                       unsigned shard);
  void scanSection(InputSectionBase &sec, unsigned shard) override {
    elf::scanSection1<MMIX, ELF64BE>(*this, sec, shard);
  }
  bool relaxOnce(int pass) const override;
  void finalizeRelax(int passes) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;

private:
  enum class RelaxationState : uint8_t { Pending, Direct, Expanded, Invalid };

  struct RelaxationSite {
    InputSection *section;
    uint32_t relocationIndex;
    RelaxationState state = RelaxationState::Pending;
  };

  mutable SmallVector<RelaxationSite, 0> relaxationSites;
  mutable bool relaxationSitesInitialized = false;
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
  RelExpr expr = R_NONE;
  bool implemented = true;
  switch (type) {
  case R_MMIX_NONE:
    return R_NONE;
  case R_MMIX_8:
  case R_MMIX_16:
  case R_MMIX_24:
  case R_MMIX_32:
  case R_MMIX_64:
    expr = R_ABS;
    break;
  case R_MMIX_PC_8:
  case R_MMIX_PC_16:
  case R_MMIX_PC_24:
  case R_MMIX_PC_32:
  case R_MMIX_PC_64:
  case R_MMIX_ADDR19:
  case R_MMIX_ADDR27:
  case R_MMIX_GETA:
  case R_MMIX_CBRANCH:
  case R_MMIX_PUSHJ:
  case R_MMIX_JMP:
    expr = R_PC;
    break;
  default:
    implemented = false;
    break;
  }

  if (!implemented) {
    StringRef reason = getUnsupportedRelocationReason(type);
    if (reason.empty())
      Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v
               << ") against symbol " << &s;
    else
      Err(ctx) << getErrorLoc(ctx, loc) << "unsupported relocation " << type
               << " against symbol " << &s << ": " << reason;
    return R_NONE;
  }

  if (s.isTls()) {
    Err(ctx) << getErrorLoc(ctx, loc) << "relocation " << type
             << " against TLS symbol " << &s << " is unsupported";
    return R_NONE;
  }
  return expr;
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

    if (std::optional<MMIXRelaxationSequence> sequence =
            getRelaxationSequence(type)) {
      uint64_t offset = it->r_offset;
      Symbol &sym = sec.getFile<ELFT>()->getSymbol(it->getSymbol(false));
      if ((offset & 3) != 0) {
        Err(ctx) << &sec << ": relaxation relocation " << type << " offset "
                 << offset << " is not 4-byte aligned against symbol " << &sym;
        continue;
      }
      ArrayRef<uint8_t> contents = sec.content();
      if (offset >= contents.size() ||
          sequence->size > contents.size() - offset) {
        Err(ctx) << &sec << ": " << type << " requires a "
                 << static_cast<unsigned>(sequence->size)
                 << "-byte reserved sequence at offset " << offset
                 << " against symbol " << &sym;
        continue;
      }

      contents = contents.slice(offset, sequence->size);
      if ((contents[0] & sequence->opcodeMask) != sequence->opcode) {
        Err(ctx) << &sec << ": " << type
                 << " reserved sequence has an invalid primary instruction "
                 << "at offset " << offset << " against symbol " << &sym;
        continue;
      }

      bool validPadding = true;
      for (unsigned i = 4; i < sequence->size; i += 4)
        validPadding &= read32be(contents.data() + i) == swymInstruction;
      if (!validPadding) {
        Err(ctx) << &sec << ": " << type
                 << " reserved sequence contains non-SWYM padding at offset "
                 << offset << " against symbol " << &sym;
        continue;
      }
    }

    if (unsigned size = getRelocationFieldSize(type)) {
      uint64_t offset = it->r_offset;
      Symbol &sym = sec.getFile<ELFT>()->getSymbol(it->getSymbol(false));
      if (offset >= sec.getSize()) {
        Err(ctx) << &sec << ": relocation " << type << " offset " << offset
                 << " is outside the section against symbol " << &sym;
        continue;
      }
      if (size > sec.getSize() - offset) {
        Err(ctx) << &sec << ": relocation " << type << " field at offset "
                 << offset << " extends past the end of the section against "
                 << "symbol " << &sym;
        continue;
      }
      if (isTerminalRelocation(type) && (offset & 3) != 0) {
        Err(ctx) << &sec << ": relocation " << type << " field offset "
                 << offset << " is not 4-byte aligned against symbol " << &sym;
        continue;
      }
    }
    rs.scan<ELFT, RelTy>(it, type, rs.getAddend<ELFT>(*it, type));
  }
}

bool MMIX::relaxOnce(int) const {
  if (!relaxationSitesInitialized) {
    SmallVector<InputSection *, 0> storage;
    for (OutputSection *osec : ctx.outputSections) {
      if (!(osec->flags & SHF_EXECINSTR))
        continue;
      for (InputSection *sec : getInputSections(*osec, storage)) {
        ArrayRef<Relocation> rels = sec->relocs();
        for (auto [index, rel] : llvm::enumerate(rels))
          if (getRelaxationSequence(rel.type))
            relaxationSites.push_back({sec, static_cast<uint32_t>(index)});
      }
    }
    relaxationSitesInitialized = true;
  }

  bool changed = false;
  for (RelaxationSite &site : relaxationSites) {
    if (site.state == RelaxationState::Invalid)
      continue;

    Relocation &rel = site.section->relocs()[site.relocationIndex];
    uint64_t target = rel.sym->getVA(ctx, rel.addend);
    uint64_t place = site.section->getVA(rel.offset);
    if (target & 3) {
      const uint8_t *loc = site.section->content().data() + rel.offset;
      Err(ctx) << getErrorLoc(ctx, loc) << "relocation " << rel.type
               << " against symbol " << rel.sym
               << " has a target that is not 4-byte aligned: 0x"
               << utohexstr(target);
      site.state = RelaxationState::Invalid;
      rel.expr = R_NONE;
      continue;
    }

    uint32_t valueMask = rel.type == R_MMIX_JMP ? pc27ValueMask : pc19ValueMask;
    bool direct = isDirectMMIXTransfer(target, place, valueMask);
    if (site.state == RelaxationState::Pending) {
      site.state = direct ? RelaxationState::Direct : RelaxationState::Expanded;
      rel.expr = direct ? R_PC : R_ABS;
      changed = true;
    } else if (site.state == RelaxationState::Direct && !direct) {
      site.state = RelaxationState::Expanded;
      rel.expr = R_ABS;
      changed = true;
    }
  }
  return changed;
}

void MMIX::finalizeRelax(int passes) const {
  Log(ctx) << "MMIX relaxation passes: " << passes;
}

void MMIX::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_MMIX_NONE:
    return;
  case R_MMIX_8:
  case R_MMIX_PC_8:
    checkMMIXBitfield(ctx, loc, val, 8, rel);
    *loc = val;
    return;
  case R_MMIX_16:
  case R_MMIX_PC_16:
    checkMMIXBitfield(ctx, loc, val, 16, rel);
    write16be(loc, val);
    return;
  case R_MMIX_24:
  case R_MMIX_PC_24:
    checkMMIXBitfield(ctx, loc, val, 24, rel);
    write32be(loc, (read32be(loc) & 0xff000000) | (val & 0xffffff));
    return;
  case R_MMIX_32:
  case R_MMIX_PC_32:
    checkMMIXBitfield(ctx, loc, val, 32, rel);
    write32be(loc, val);
    return;
  case R_MMIX_64:
  case R_MMIX_PC_64:
    write64be(loc, val);
    return;
  case R_MMIX_ADDR19:
    relocateMMIXTerminal(loc, ctx, val, pc19ValueMask, rel);
    return;
  case R_MMIX_ADDR27:
    relocateMMIXTerminal(loc, ctx, val, pc27ValueMask, rel);
    return;
  case R_MMIX_GETA:
  case R_MMIX_CBRANCH:
  case R_MMIX_PUSHJ:
    if (rel.expr == R_PC)
      relocateMMIXTerminal(loc, ctx, val, pc19ValueMask, rel);
    else if (rel.expr == R_ABS)
      relocateMMIXExpanded(loc, val, rel);
    return;
  case R_MMIX_JMP:
    if (rel.expr == R_PC)
      relocateMMIXTerminal(loc, ctx, val, pc27ValueMask, rel);
    else if (rel.expr == R_ABS)
      relocateMMIXExpanded(loc, val, rel);
    return;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unsupported relocation " << rel.type;
  }
}

void elf::setMMIXTargetInfo(Ctx &ctx) { ctx.target.reset(new MMIX(ctx)); }
