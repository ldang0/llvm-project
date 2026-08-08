//===-- MMIXALAsmStreamer.cpp - Buffer MMIXAL assembly events ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALAsmStreamer.h"
#include "MMIXALInstPrinter.h"
#include "MMIXBaseInfo.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"
#include <cassert>
#include <limits>
#include <system_error>
#include <utility>

using namespace llvm;

namespace {

constexpr StringLiteral UnsupportedBufferedModule =
    "MMIXAL buffered module emission is not implemented";
constexpr size_t MMIXALInputLineLimit = 72;

class MMIXALDataListEmitter {
  raw_ostream &OS;
  StringRef Directive;
  unsigned Width;
  size_t LineLength = 0;

  std::string formatValue(uint64_t Value) const {
    if (Value == 0)
      return "0";
    SmallString<18> Text;
    raw_svector_ostream ValueOS(Text);
    ValueOS << '#' << format_hex_no_prefix(Value, Width * 2, /*Upper=*/true);
    return std::string(Text);
  }

public:
  MMIXALDataListEmitter(raw_ostream &OS, StringRef Directive, unsigned Width)
      : OS(OS), Directive(Directive), Width(Width) {}

  void emit(uint64_t Value) {
    const std::string Text = formatValue(Value);
    const size_t SeparatorLength = LineLength == 0 ? 0 : 2;
    const size_t PrefixLength = Directive.size() + 2;
    if (LineLength != 0 &&
        LineLength + SeparatorLength + Text.size() > MMIXALInputLineLimit) {
      OS << '\n';
      LineLength = 0;
    }
    if (LineLength == 0) {
      OS << '\t' << Directive << ' ';
      LineLength = PrefixLength;
    } else {
      OS << ", ";
      LineLength += SeparatorLength;
    }
    OS << Text;
    LineLength += Text.size();
  }

  void finish() {
    if (LineLength != 0)
      OS << '\n';
  }
};

StringRef getDataDirective(unsigned Width) {
  switch (Width) {
  case 1:
    return "BYTE";
  case 2:
    return "WYDE";
  case 4:
    return "TETRA";
  case 8:
    return "OCTA";
  default:
    llvm_unreachable("unsupported MMIXAL scalar width");
  }
}

uint64_t truncateToWidth(uint64_t Value, unsigned Width) {
  if (Width == 8)
    return Value;
  return Value & ((uint64_t(1) << (Width * 8)) - 1);
}

void emitRepeatedScalar(raw_ostream &OS, unsigned Width, uint64_t Value,
                        uint64_t Count) {
  MMIXALDataListEmitter Emitter(OS, getDataDirective(Width), Width);
  Value = truncateToWidth(Value, Width);
  for (uint64_t I = 0; I != Count; ++I)
    Emitter.emit(Value);
  Emitter.finish();
}

void emitBigEndianBytes(raw_ostream &OS, uint64_t Value, unsigned Width,
                        uint64_t Count = 1) {
  MMIXALDataListEmitter Emitter(OS, "BYTE", 1);
  for (uint64_t I = 0; I != Count; ++I)
    for (unsigned Byte = Width; Byte != 0; --Byte)
      Emitter.emit((Value >> ((Byte - 1) * 8)) & 0xff);
  Emitter.finish();
}

void emitByteArray(raw_ostream &OS, StringRef Bytes) {
  MMIXALDataListEmitter Emitter(OS, "BYTE", 1);
  for (unsigned char Byte : Bytes.bytes())
    Emitter.emit(Byte);
  Emitter.finish();
}

void emitExplicitZeros(raw_ostream &OS, uint64_t Address, uint64_t Size) {
  while (Size != 0) {
    unsigned Width = 1;
    for (unsigned Candidate : {8u, 4u, 2u})
      if (Size >= Candidate && Address % Candidate == 0) {
        Width = Candidate;
        break;
      }

    uint64_t Count = 1;
    if (Width == 8)
      Count = Size / Width;
    emitRepeatedScalar(OS, Width, 0, Count);
    const uint64_t Emitted = Count * Width;
    Address += Emitted;
    Size -= Emitted;
  }
}

Error advanceDataAddress(uint64_t &Address, uint64_t Size, uint64_t ItemEnd) {
  if (Address > ItemEnd || Size > ItemEnd - Address)
    return createStringError(
        "MMIXAL data event exceeds its planned allocation interval");
  Address += Size;
  return Error::success();
}

bool hasUnsupportedSpecialPurpose(StringRef Name) {
  return Name == ".eh_frame" || Name.starts_with(".gcc_except_table") ||
         Name.starts_with(".init_array") || Name.starts_with(".fini_array") ||
         Name.starts_with(".preinit_array") || Name.starts_with(".ctors") ||
         Name.starts_with(".dtors") || Name.starts_with(".tdata") ||
         Name.starts_with(".tbss") || Name.starts_with(".note") ||
         Name.starts_with(".debug") || Name.starts_with(".llvm") ||
         Name.starts_with(".got") || Name.starts_with(".plt") ||
         Name.starts_with(".mmix") || Name.starts_with(".MMIX");
}

const MCSymbol *getBareSymbol(const MCExpr &Expr) {
  if (const auto *Symbol = dyn_cast<MCSymbolRefExpr>(&Expr))
    return Symbol->getKind() == 0 ? &Symbol->getSymbol() : nullptr;
  if (const auto *Unary = dyn_cast<MCUnaryExpr>(&Expr))
    if (Unary->getOpcode() == MCUnaryExpr::Plus)
      return getBareSymbol(*Unary->getSubExpr());
  return nullptr;
}

void emitAbsoluteLocation(uint64_t Address, raw_ostream &OS) {
  OS << "\tLOC #" << format_hex_no_prefix(Address, 16, /*Upper=*/true) << '\n';
}

Error validateInstructionAddress(
    const MCInst &Inst, uint64_t Address, const MMIXALInstPrinter &Printer,
    const MMIXALLayoutPlan &Layout,
    const DenseMap<const MCSymbol *, uint64_t> &AliasAddresses,
    const DenseSet<const MCSymbol *> &DefinedSymbols,
    ArrayRef<const MCSymbol *> Dependencies) {
  if ((Address & 3) != 0)
    return createStringError("MMIXAL instruction address is not four-byte "
                             "aligned");

  const MCInstrDesc &Desc = Printer.getInstructionDesc(Inst.getOpcode());
  const MMIXII::MMIXALSelectionKind Selection =
      MMIXII::getMMIXALSelection(Desc.TSFlags);
  const bool IsForward = Selection == MMIXII::MMIXALSelectionForward;
  const bool IsBackward = Selection == MMIXII::MMIXALSelectionBackward;
  if (!IsForward && !IsBackward) {
    for (const MCSymbol *Dependency : Dependencies)
      if (!DefinedSymbols.contains(Dependency))
        return createStringError(
            Twine("MMIXAL instruction references symbol '") +
            Dependency->getName() + "' before its definition");
    return Error::success();
  }

  if (Desc.getNumOperands() == 0 ||
      Desc.getNumOperands() > Inst.getNumOperands())
    return createStringError("MMIXAL relative instruction has no target");
  const MCOperand &TargetOperand = Inst.getOperand(Desc.getNumOperands() - 1);
  if (!TargetOperand.isExpr())
    return createStringError(
        "MMIXAL source relative target requires an expression");

  const MCExpr &TargetExpr = *TargetOperand.getExpr();
  const MCSymbol *TargetSymbol = getBareSymbol(TargetExpr);
  uint64_t TargetAddress;
  int64_t AbsoluteTarget;
  if (TargetExpr.evaluateAsAbsolute(AbsoluteTarget)) {
    TargetAddress = static_cast<uint64_t>(AbsoluteTarget);
  } else if (TargetSymbol) {
    std::optional<uint64_t> Resolved = Layout.getSymbolAddress(*TargetSymbol);
    const auto Alias = AliasAddresses.find(TargetSymbol);
    if (!Resolved && Alias != AliasAddresses.end())
      Resolved = Alias->second;
    if (!Resolved)
      return createStringError(Twine("MMIXAL relative target '") +
                               TargetSymbol->getName() +
                               "' has no allocated definition");
    TargetAddress = *Resolved;
  } else {
    return createStringError(
        "MMIXAL relative target must be an absolute address or bare symbol");
  }

  for (const MCSymbol *Dependency : Dependencies)
    if (Dependency != TargetSymbol && !DefinedSymbols.contains(Dependency))
      return createStringError(
          Twine("MMIXAL relative instruction references symbol '") +
          Dependency->getName() + "' before its definition");

  if ((TargetAddress & 3) != 0)
    return createStringError("MMIXAL relative target is not four-byte aligned");
  if ((IsForward && TargetAddress < Address) ||
      (IsBackward && TargetAddress >= Address))
    return createStringError(
        "MMIXAL relative target direction does not match instruction");

  const unsigned Width = MMIXII::getPCRelativeWidth(Desc.TSFlags);
  if (Width != 16 && Width != 24)
    return createStringError("MMIXAL relative instruction has invalid width");
  const uint64_t Distance =
      IsBackward ? Address - TargetAddress : TargetAddress - Address;
  if ((Distance & 3) != 0)
    return createStringError("MMIXAL relative target is not four-byte aligned");
  const uint64_t MaxWords =
      IsBackward ? uint64_t(1) << Width : (uint64_t(1) << Width) - 1;
  if (Distance / 4 > MaxWords)
    return createStringError("MMIXAL relative target is out of range");
  return Error::success();
}

} // namespace

MMIXALAsmStreamer::MMIXALAsmStreamer(
    MCContext &Context, std::unique_ptr<formatted_raw_ostream> Output,
    std::unique_ptr<MMIXALInstPrinter> InstPrinter)
    : MCStreamer(Context), Output(std::move(Output)),
      InstPrinter(std::move(InstPrinter)) {
  assert(this->Output && "MMIXAL streamer requires an output stream");
  assert(this->InstPrinter &&
         "MMIXAL streamer requires an instruction printer");
}

MMIXALAsmStreamer::~MMIXALAsmStreamer() = default;

size_t MMIXALAsmStreamer::getGroupIndex(LogicalGroup Group) {
  return static_cast<size_t>(Group);
}

void MMIXALAsmStreamer::recordClassificationError(const Twine &Message) {
  if (ClassificationError.empty())
    ClassificationError = Message.str();
}

Expected<MMIXALAsmStreamer::LogicalGroup>
MMIXALAsmStreamer::classifySection(const MCSection &Section,
                                   uint32_t Subsection) const {
  if (Subsection != 0)
    return createStringError(
        Twine("MMIXAL cannot classify section '") + Section.getName() +
        "': nonzero ELF subsections have unsupported ordering semantics");
  if (getContext().getObjectFileType() != MCContext::IsELF)
    return createStringError(Twine("MMIXAL cannot classify non-ELF section '") +
                             Section.getName() + "'");

  const auto &ELFSection = static_cast<const MCSectionELF &>(Section);
  const StringRef Name = ELFSection.getName();
  const unsigned Type = ELFSection.getType();
  const unsigned Flags = ELFSection.getFlags();
  if (!(Flags & ELF::SHF_ALLOC))
    return createStringError(Twine("MMIXAL cannot allocate section '") + Name +
                             "': section is not allocated");
  if (hasUnsupportedSpecialPurpose(Name))
    return createStringError(
        Twine("MMIXAL cannot allocate section '") + Name +
        "': special-purpose section semantics are unsupported");

  constexpr unsigned OrdinaryFlags =
      ELF::SHF_ALLOC | ELF::SHF_WRITE | ELF::SHF_EXECINSTR;
  if (Flags & ~OrdinaryFlags)
    return createStringError(
        Twine("MMIXAL cannot allocate section '") + Name +
        "': merge, TLS, group, ordering, or target-specific ELF flags are "
        "unsupported");
  if (ELFSection.getEntrySize() != 0)
    return createStringError(
        Twine("MMIXAL cannot allocate section '") + Name +
        "': fixed-entry section semantics are unsupported");
  if (Type != ELF::SHT_PROGBITS && Type != ELF::SHT_NOBITS)
    return createStringError(Twine("MMIXAL cannot allocate section '") + Name +
                             "': unsupported ELF section type");

  const bool IsWritable = Flags & ELF::SHF_WRITE;
  const bool IsExecutable = Flags & ELF::SHF_EXECINSTR;
  if (IsExecutable && IsWritable)
    return createStringError(Twine("MMIXAL cannot allocate section '") + Name +
                             "': writable executable sections are ambiguous");
  if (Type == ELF::SHT_NOBITS) {
    if (!IsWritable || IsExecutable)
      return createStringError(
          Twine("MMIXAL cannot allocate section '") + Name +
          "': zero-storage sections must be writable and non-executable");
    return LogicalGroup::ZeroStorage;
  }
  if (IsExecutable)
    return LogicalGroup::Text;
  if (IsWritable)
    return LogicalGroup::WritableData;
  return LogicalGroup::ReadOnly;
}

std::optional<MMIXALAsmStreamer::LogicalGroup>
MMIXALAsmStreamer::classifyCurrentSection() {
  const MCSectionSubPair Current = getCurrentSection();
  if (!Current.first) {
    recordClassificationError(
        "MMIXAL allocated event has no active MC section");
    return std::nullopt;
  }
  Expected<LogicalGroup> Group =
      classifySection(*Current.first, Current.second);
  if (!Group) {
    recordClassificationError(toString(Group.takeError()));
    return std::nullopt;
  }
  return *Group;
}

MMIXALAsmStreamer::BufferedItem *MMIXALAsmStreamer::getOrCreateCurrentItem() {
  if (CurrentItem)
    return &*CurrentItem;
  std::optional<LogicalGroup> Group = classifyCurrentSection();
  if (!Group)
    return nullptr;
  CurrentItem.emplace(
      BufferedItem{*Group, getCurrentSection().first, NextItemOrder++});
  return &*CurrentItem;
}

void MMIXALAsmStreamer::flushCurrentItem() {
  if (!CurrentItem)
    return;
  if (!CurrentItem->EventIndices.empty())
    ItemGroups[getGroupIndex(CurrentItem->Group)].push_back(
        std::move(*CurrentItem));
  CurrentItem.reset();
}

void MMIXALAsmStreamer::appendEventToCurrentItem(BufferedEvent Event,
                                                 std::optional<uint64_t> Size,
                                                 bool IsPayload) {
  BufferedItem *Item = getOrCreateCurrentItem();
  const size_t EventIndex = Events.size();
  Events.push_back(std::move(Event));
  if (!Item)
    return;

  const BufferedEvent &StoredEvent = Events.back();
  updateCurrentItemGroupForDependencies(StoredEvent.Dependencies);
  Item->EventIndices.push_back(EventIndex);
  for (const MCSymbol *Dependency : StoredEvent.Dependencies)
    if (!llvm::is_contained(Item->Dependencies, Dependency))
      Item->Dependencies.push_back(Dependency);
  if (Size) {
    if (*Size > std::numeric_limits<uint64_t>::max() - Item->KnownSize)
      Item->SizeIsKnown = false;
    else if (Item->SizeIsKnown)
      Item->KnownSize += *Size;
  } else if (IsPayload) {
    Item->SizeIsKnown = false;
  }
  Item->HasPayload |= IsPayload;
}

void MMIXALAsmStreamer::updateCurrentItemGroupForSymbol(
    const MCSymbol &Symbol) {
  BufferedItem *Item = getOrCreateCurrentItem();
  if (!Item)
    return;
  std::optional<MMIXALSymbolTable::PrivateSymbolKind> Kind =
      Symbols.getPrivateSymbolKind(Symbol);
  if (!Kind)
    return;

  std::optional<LogicalGroup> RequiredGroup;
  if (*Kind == MMIXALSymbolTable::PrivateSymbolKind::ConstantPool)
    RequiredGroup = LogicalGroup::ConstantPool;
  else if (*Kind == MMIXALSymbolTable::PrivateSymbolKind::JumpTable)
    RequiredGroup = LogicalGroup::JumpTable;
  else if (*Kind == MMIXALSymbolTable::PrivateSymbolKind::BlockAddress &&
           Item->Group != LogicalGroup::Text)
    RequiredGroup = LogicalGroup::JumpTable;
  if (!RequiredGroup)
    return;

  if (Item->Group == LogicalGroup::Text ||
      Item->Group == LogicalGroup::ZeroStorage) {
    recordClassificationError(Twine("MMIXAL private data symbol '") +
                              Symbol.getName() +
                              "' is defined in an incompatible section");
    return;
  }
  if ((Item->Group == LogicalGroup::ConstantPool ||
       Item->Group == LogicalGroup::JumpTable) &&
      Item->Group != *RequiredGroup) {
    recordClassificationError(
        Twine("MMIXAL item has conflicting private symbol classes at '") +
        Symbol.getName() + "'");
    return;
  }
  Item->Group = *RequiredGroup;
}

void MMIXALAsmStreamer::updateCurrentItemGroupForDependencies(
    ArrayRef<const MCSymbol *> Dependencies) {
  if (!CurrentItem || CurrentItem->Group == LogicalGroup::Text ||
      CurrentItem->Group == LogicalGroup::ConstantPool ||
      CurrentItem->Group == LogicalGroup::JumpTable)
    return;

  for (const MCSymbol *Dependency : Dependencies) {
    std::optional<MMIXALSymbolTable::PrivateSymbolKind> Kind =
        Symbols.getPrivateSymbolKind(*Dependency);
    if (Kind != MMIXALSymbolTable::PrivateSymbolKind::BlockAddress)
      continue;
    if (CurrentItem->Group == LogicalGroup::ZeroStorage) {
      recordClassificationError(
          "MMIXAL zero-storage item references a block address");
      return;
    }
    CurrentItem->Group = LogicalGroup::JumpTable;
    return;
  }
}

void MMIXALAsmStreamer::reset() {
  MCStreamer::reset();
  Events.clear();
  for (auto &Group : ItemGroups)
    Group.clear();
  CurrentItem.reset();
  DependencySink = nullptr;
  ClassificationError.clear();
  NextItemOrder = 0;
  Symbols.reset();
  ActiveFunctionName.clear();
  PendingFunctionSymbols.clear();
  PendingFunctionSymbolSet.clear();
  PendingModuleSymbols.clear();
  PendingModuleSymbolSet.clear();
  HasActiveFunction = false;
}

void MMIXALAsmStreamer::switchSection(MCSection *Section, uint32_t Subsection) {
  assert(Section && "cannot switch to a null section");
  const MCSectionSubPair Previous = getCurrentSection();
  if (Previous != MCSectionSubPair(Section, Subsection))
    flushCurrentItem();
  MCStreamer::switchSection(Section, Subsection);

  BufferedEvent Event{EventKind::SectionSwitch};
  Event.Section = Section;
  Event.Subsection = Subsection;
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::emitBytes(StringRef Data) {
  if (Data.empty())
    return;
  BufferedEvent Event{EventKind::Bytes};
  Event.Bytes = Data.str();
  Event.Section = getCurrentSection().first;
  BufferedItem *Item = getOrCreateCurrentItem();
  if (Item && Item->Group == LogicalGroup::ZeroStorage)
    recordClassificationError(
        "MMIXAL zero-storage section contains initialized bytes");
  appendEventToCurrentItem(std::move(Event), Data.size());
}

void MMIXALAsmStreamer::emitInstruction(const MCInst &Inst,
                                        const MCSubtargetInfo &STI) {
  SmallVector<const MCSymbol *, 2> Dependencies;
  DependencySink = &Dependencies;
  MCStreamer::emitInstruction(Inst, STI);
  DependencySink = nullptr;
  BufferedEvent Event{EventKind::Instruction};
  Event.Inst = Inst;
  Event.STI = &STI;
  Event.Section = getCurrentSection().first;
  Event.Dependencies = std::move(Dependencies);
  BufferedItem *Item = getOrCreateCurrentItem();
  if (Item && Item->Group != LogicalGroup::Text)
    recordClassificationError("MMIXAL instruction is not in executable text");
  appendEventToCurrentItem(std::move(Event), 4);
}

void MMIXALAsmStreamer::emitLabel(MCSymbol *Symbol, SMLoc) {
  assert(Symbol && "cannot emit a null symbol");
  observeSymbol(*Symbol);
  if (CurrentItem && CurrentItem->HasPayload)
    flushCurrentItem();
  updateCurrentItemGroupForSymbol(*Symbol);
  BufferedEvent Event{EventKind::Label};
  Event.Symbol = Symbol;
  Event.Section = getCurrentSection().first;
  appendEventToCurrentItem(std::move(Event), 0, false);
  if (CurrentItem)
    CurrentItem->OwningSymbols.push_back(Symbol);
}

void MMIXALAsmStreamer::emitAssignment(MCSymbol *Symbol, const MCExpr *Value) {
  assert(Symbol && Value && "cannot emit a null assignment");
  observeSymbol(*Symbol);
  SmallVector<const MCSymbol *, 2> Dependencies;
  DependencySink = &Dependencies;
  MCStreamer::emitAssignment(Symbol, Value);
  DependencySink = nullptr;

  BufferedEvent Event{EventKind::Assignment};
  Event.Symbol = Symbol;
  Event.Expression = Value;
  Event.Dependencies = std::move(Dependencies);
  Event.Section = getCurrentSection().first;
  if (CurrentItem)
    appendEventToCurrentItem(std::move(Event), 0, false);
  else
    Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::visitUsedSymbol(const MCSymbol &Symbol) {
  observeSymbol(Symbol);
  if (DependencySink && !llvm::is_contained(*DependencySink, &Symbol))
    DependencySink->push_back(&Symbol);
}

bool MMIXALAsmStreamer::emitSymbolAttribute(MCSymbol *Symbol,
                                            MCSymbolAttr Attribute) {
  assert(Symbol && "cannot emit an attribute for a null symbol");
  observeSymbol(*Symbol);
  BufferedEvent Event{EventKind::SymbolAttribute};
  Event.Symbol = Symbol;
  Event.Section = getCurrentSection().first;
  Event.Attribute = Attribute;
  Events.push_back(std::move(Event));
  return true;
}

void MMIXALAsmStreamer::emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                         Align ByteAlignment) {
  assert(Symbol && "cannot emit a null common symbol");
  observeSymbol(*Symbol);
  flushCurrentItem();
  BufferedEvent Event{EventKind::CommonSymbol};
  Event.Symbol = Symbol;
  Event.Size = Size;
  Event.Alignment = ByteAlignment;
  const size_t EventIndex = Events.size();
  Events.push_back(std::move(Event));

  BufferedItem Item{LogicalGroup::ZeroStorage, nullptr, NextItemOrder++};
  Item.EventIndices.push_back(EventIndex);
  Item.OwningSymbols.push_back(Symbol);
  Item.RequiredAlignment = ByteAlignment.value();
  Item.KnownSize = Size;
  Item.HasPayload = true;
  ItemGroups[getGroupIndex(Item.Group)].push_back(std::move(Item));
}

void MMIXALAsmStreamer::emitLocalCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                              Align ByteAlignment) {
  emitCommonSymbol(Symbol, Size, ByteAlignment);
}

void MMIXALAsmStreamer::emitZerofill(MCSection *Section, MCSymbol *Symbol,
                                     uint64_t Size, Align ByteAlignment,
                                     SMLoc) {
  assert(Section && "cannot emit zerofill without a section");
  flushCurrentItem();
  Expected<LogicalGroup> Group = classifySection(*Section, 0);
  if (!Group) {
    recordClassificationError(toString(Group.takeError()));
    return;
  }
  if (*Group != LogicalGroup::ZeroStorage) {
    recordClassificationError(
        Twine("MMIXAL zerofill requires a zero-storage section, not '") +
        Section->getName() + "'");
    return;
  }

  BufferedEvent Event{EventKind::ZeroFill};
  Event.Symbol = Symbol;
  Event.Section = Section;
  Event.Size = Size;
  Event.Alignment = ByteAlignment;
  const size_t EventIndex = Events.size();
  Events.push_back(std::move(Event));

  BufferedItem Item{LogicalGroup::ZeroStorage, Section, NextItemOrder++};
  Item.EventIndices.push_back(EventIndex);
  if (Symbol) {
    observeSymbol(*Symbol);
    Item.OwningSymbols.push_back(Symbol);
  }
  Item.RequiredAlignment = ByteAlignment.value();
  Item.KnownSize = Size;
  Item.HasPayload = true;
  ItemGroups[getGroupIndex(Item.Group)].push_back(std::move(Item));
}

void MMIXALAsmStreamer::emitTBSSSymbol(MCSection *Section, MCSymbol *Symbol,
                                       uint64_t Size, Align ByteAlignment) {
  if (Symbol)
    observeSymbol(*Symbol);
  recordClassificationError("MMIXAL thread-local zero storage is unsupported");
  BufferedEvent Event{EventKind::CommonSymbol};
  Event.Symbol = Symbol;
  Event.Section = Section;
  Event.Size = Size;
  Event.Alignment = ByteAlignment;
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::emitValueImpl(const MCExpr *Value, unsigned Size,
                                      SMLoc) {
  assert(Value && "cannot emit a null value expression");
  SmallVector<const MCSymbol *, 2> Dependencies;
  DependencySink = &Dependencies;
  MCStreamer::emitValueImpl(Value, Size);
  DependencySink = nullptr;

  BufferedEvent Event{EventKind::Value};
  Event.Section = getCurrentSection().first;
  Event.Expression = Value;
  Event.Dependencies = std::move(Dependencies);
  Event.ValueSize = Size;
  BufferedItem *Item = getOrCreateCurrentItem();
  if (Item && Item->Group == LogicalGroup::ZeroStorage)
    recordClassificationError(
        "MMIXAL zero-storage section contains an initialized value");
  appendEventToCurrentItem(std::move(Event), Size);
}

void MMIXALAsmStreamer::emitFill(const MCExpr &NumBytes, uint64_t FillValue,
                                 SMLoc) {
  SmallVector<const MCSymbol *, 2> Dependencies;
  DependencySink = &Dependencies;
  visitUsedExpr(NumBytes);
  DependencySink = nullptr;

  int64_t ByteCount = 0;
  std::optional<uint64_t> KnownSize;
  if (NumBytes.evaluateAsAbsolute(ByteCount) && ByteCount >= 0)
    KnownSize = static_cast<uint64_t>(ByteCount);

  BufferedEvent Event{EventKind::ByteFill};
  Event.Section = getCurrentSection().first;
  Event.Expression = &NumBytes;
  Event.Dependencies = std::move(Dependencies);
  Event.FillValue = FillValue;
  BufferedItem *Item = getOrCreateCurrentItem();
  if (Item && Item->Group == LogicalGroup::ZeroStorage && FillValue != 0)
    recordClassificationError(
        "MMIXAL zero-storage section contains a nonzero fill");
  appendEventToCurrentItem(std::move(Event), KnownSize);
}

void MMIXALAsmStreamer::emitFill(const MCExpr &NumValues, int64_t Size,
                                 int64_t Expr, SMLoc) {
  SmallVector<const MCSymbol *, 2> Dependencies;
  DependencySink = &Dependencies;
  visitUsedExpr(NumValues);
  DependencySink = nullptr;

  int64_t Count = 0;
  std::optional<uint64_t> KnownSize;
  if (Size >= 0 && NumValues.evaluateAsAbsolute(Count) && Count >= 0 &&
      static_cast<uint64_t>(Count) <=
          std::numeric_limits<uint64_t>::max() /
              static_cast<uint64_t>(Size == 0 ? 1 : Size))
    KnownSize = static_cast<uint64_t>(Count) * static_cast<uint64_t>(Size);

  BufferedEvent Event{EventKind::RepeatedValue};
  Event.Section = getCurrentSection().first;
  Event.Expression = &NumValues;
  Event.Dependencies = std::move(Dependencies);
  Event.ValueSize = Size < 0 ? 0 : static_cast<unsigned>(Size);
  Event.RepeatValue = Expr;
  BufferedItem *Item = getOrCreateCurrentItem();
  if (Item && Item->Group == LogicalGroup::ZeroStorage && Expr != 0)
    recordClassificationError(
        "MMIXAL zero-storage section contains a nonzero repeated fill");
  appendEventToCurrentItem(std::move(Event), KnownSize);
}

void MMIXALAsmStreamer::emitValueToAlignment(Align Alignment, int64_t Fill,
                                             uint8_t FillLength,
                                             unsigned MaxBytesToEmit) {
  if (CurrentItem &&
      (CurrentItem->HasPayload || !CurrentItem->OwningSymbols.empty()))
    flushCurrentItem();
  BufferedEvent Event{EventKind::Alignment};
  Event.Section = getCurrentSection().first;
  Event.Alignment = Alignment;
  Event.AlignmentFill = Fill;
  Event.AlignmentFillLength = FillLength;
  Event.MaxBytesToEmit = MaxBytesToEmit;
  appendEventToCurrentItem(std::move(Event), 0, false);
  if (CurrentItem) {
    CurrentItem->Alignments.push_back(
        {Alignment.value(), Fill, FillLength, MaxBytesToEmit, false});
    if (CurrentItem->RequiredAlignment < Alignment.value())
      CurrentItem->RequiredAlignment = Alignment.value();
  }
}

void MMIXALAsmStreamer::emitCodeAlignment(Align Alignment,
                                          const MCSubtargetInfo &STI,
                                          unsigned MaxBytesToEmit) {
  if (CurrentItem &&
      (CurrentItem->HasPayload || !CurrentItem->OwningSymbols.empty()))
    flushCurrentItem();
  BufferedEvent Event{EventKind::Alignment};
  Event.Section = getCurrentSection().first;
  Event.Alignment = Alignment;
  Event.MaxBytesToEmit = MaxBytesToEmit;
  Event.IsCodeAlignment = true;
  BufferedItem *Item = getOrCreateCurrentItem();
  if (Item && Item->Group != LogicalGroup::Text)
    recordClassificationError(
        "MMIXAL code alignment is not in executable text");
  appendEventToCurrentItem(std::move(Event), 0, false);
  if (CurrentItem) {
    CurrentItem->Alignments.push_back(
        {Alignment.value(), 0, 1, MaxBytesToEmit, true, &STI});
    if (CurrentItem->RequiredAlignment < Alignment.value())
      CurrentItem->RequiredAlignment = Alignment.value();
  }
}

Error MMIXALAsmStreamer::registerUserSymbol(const MCSymbol &Symbol,
                                            StringRef RawName) {
  return Symbols.registerUserSymbol(Symbol, RawName);
}

Error MMIXALAsmStreamer::registerSourceBlock(const MCSymbol &AddressSymbol,
                                             StringRef FunctionName,
                                             StringRef BlockName) {
  return Symbols.registerSourceBlock(AddressSymbol, FunctionName, BlockName);
}

Error MMIXALAsmStreamer::registerFunctionPrivateSymbol(
    const MCSymbol &Symbol, StringRef FunctionName,
    MMIXALSymbolTable::PrivateSymbolKind Kind, uint64_t Ordinal) {
  return Symbols.registerFunctionPrivateSymbol(Symbol, FunctionName, Kind,
                                               Ordinal);
}

Error MMIXALAsmStreamer::registerModulePrivateSymbol(
    const MCSymbol &Symbol, MMIXALSymbolTable::PrivateSymbolKind Kind,
    uint64_t Ordinal) {
  return Symbols.registerModulePrivateSymbol(Symbol, Kind, Ordinal);
}

Error MMIXALAsmStreamer::beginFunctionSymbols(StringRef FunctionName) {
  if (HasActiveFunction)
    return createStringError(
        std::errc::invalid_argument,
        "cannot begin MMIXAL symbol registration for nested functions");
  if (Symbols.isFinalized())
    return createStringError(
        std::errc::invalid_argument,
        "cannot begin MMIXAL function symbols after finalization");

  ActiveFunctionName = FunctionName.str();
  PendingFunctionSymbols.clear();
  PendingFunctionSymbolSet.clear();
  HasActiveFunction = true;
  return Error::success();
}

void MMIXALAsmStreamer::observeSymbol(const MCSymbol &Symbol) {
  if (Symbols.isRegistered(Symbol))
    return;
  if (HasActiveFunction) {
    if (PendingFunctionSymbolSet.insert(&Symbol).second)
      PendingFunctionSymbols.push_back(&Symbol);
    return;
  }
  if (PendingModuleSymbolSet.insert(&Symbol).second)
    PendingModuleSymbols.push_back(&Symbol);
}

Error MMIXALAsmStreamer::endFunctionSymbols(const MCSymbol *FunctionEnd) {
  if (!HasActiveFunction)
    return createStringError(
        std::errc::invalid_argument,
        "cannot end MMIXAL symbol registration without an active function");

  if (FunctionEnd && !Symbols.isRegistered(*FunctionEnd))
    if (Error Err = Symbols.registerFunctionPrivateSymbol(
            *FunctionEnd, ActiveFunctionName,
            MMIXALSymbolTable::PrivateSymbolKind::FunctionEnd, 0))
      return Err;

  uint64_t TemporaryOrdinal = 0;
  for (const MCSymbol *Symbol : PendingFunctionSymbols) {
    if (Symbols.isRegistered(*Symbol))
      continue;
    if (Error Err = Symbols.registerFunctionPrivateSymbol(
            *Symbol, ActiveFunctionName,
            MMIXALSymbolTable::PrivateSymbolKind::Temporary,
            TemporaryOrdinal++))
      return Err;
  }

  ActiveFunctionName.clear();
  PendingFunctionSymbols.clear();
  PendingFunctionSymbolSet.clear();
  HasActiveFunction = false;
  return Error::success();
}

Error MMIXALAsmStreamer::finalizeSymbolMappings() {
  if (HasActiveFunction)
    return createStringError(
        std::errc::invalid_argument,
        "cannot finalize MMIXAL symbols while a function is active");
  uint64_t TemporaryOrdinal = 0;
  for (const MCSymbol *Symbol : PendingModuleSymbols) {
    if (Symbols.isRegistered(*Symbol))
      continue;
    if (Error Err = Symbols.registerModulePrivateSymbol(
            *Symbol, MMIXALSymbolTable::PrivateSymbolKind::Temporary,
            TemporaryOrdinal++))
      return Err;
  }
  return Symbols.finalize();
}

Expected<StringRef>
MMIXALAsmStreamer::getMappedSymbol(const MCSymbol &Symbol) const {
  return Symbols.getMappedSymbol(Symbol);
}

Expected<StringRef>
MMIXALAsmStreamer::getSourceBlockAlias(const MCSymbol &AddressSymbol) const {
  return Symbols.getSourceBlockAlias(AddressSymbol);
}

ArrayRef<MMIXALAsmStreamer::BufferedItem>
MMIXALAsmStreamer::getBufferedItems(LogicalGroup Group) {
  flushCurrentItem();
  return ItemGroups[getGroupIndex(Group)];
}

size_t MMIXALAsmStreamer::getNumBufferedItems() {
  flushCurrentItem();
  size_t Count = 0;
  for (const auto &Group : ItemGroups)
    Count += Group.size();
  return Count;
}

Error MMIXALAsmStreamer::renderModule(const MMIXALLayoutPlan &Layout,
                                      raw_ostream &OS) {
  DenseSet<size_t> ItemEventIndices;
  for (const MMIXALPlacedItem &Placed : Layout.getItems())
    for (size_t EventIndex : Placed.Input->EventIndices)
      if (!ItemEventIndices.insert(EventIndex).second)
        return createStringError(
            "MMIXAL event belongs to more than one logical item");
  for (size_t EventIndex = 0; EventIndex != Events.size(); ++EventIndex)
    switch (Events[EventIndex].Kind) {
    case EventKind::SectionSwitch:
      break;
    case EventKind::Alignment:
    case EventKind::Assignment:
    case EventKind::ByteFill:
    case EventKind::Bytes:
    case EventKind::Instruction:
    case EventKind::Label:
    case EventKind::RepeatedValue:
    case EventKind::Value:
    case EventKind::ZeroFill:
      if (!ItemEventIndices.contains(EventIndex))
        return createStringError(UnsupportedBufferedModule);
      break;
    default:
      return createStringError(UnsupportedBufferedModule);
    }

  DenseSet<const MCSymbol *> DefinedSymbols;
  DenseMap<const MCSymbol *, uint64_t> AliasAddresses;
  auto ResolveSymbol =
      [this, &DefinedSymbols](const MCSymbol &Symbol) -> MMIXALSymbolPrintInfo {
    return {cantFail(Symbols.getMappedSymbol(Symbol)),
            DefinedSymbols.contains(&Symbol)};
  };

  for (const MMIXALPlacedItem &Placed : Layout.getItems()) {
    std::optional<uint64_t> CurrentLocation;
    for (const MMIXALPaddingInterval &Padding : Placed.Padding) {
      if (!Padding.Request.IsCodeAlignment) {
        emitAbsoluteLocation(Padding.End, OS);
        CurrentLocation = Padding.End;
        continue;
      }
      if (!Padding.Request.STI)
        return createStringError(
            "MMIXAL executable padding has no subtarget information");
      if ((Padding.Begin & 3) != 0 || ((Padding.End - Padding.Begin) & 3) != 0)
        return createStringError(
            "MMIXAL executable padding is not four-byte aligned");

      emitAbsoluteLocation(Padding.Begin, OS);
      MCInst Nop;
      Nop.setOpcode(MMIX::SWYM);
      Nop.addOperand(MCOperand::createImm(0));
      Nop.addOperand(MCOperand::createImm(0));
      Nop.addOperand(MCOperand::createImm(0));
      for (uint64_t Address = Padding.Begin; Address != Padding.End;
           Address += 4) {
        InstPrinter->printInstWithSymbolNames(
            &Nop, Address, *Padding.Request.STI, ResolveSymbol, OS);
        OS << '\n';
      }
      CurrentLocation = Padding.End;
    }

    if (!CurrentLocation || *CurrentLocation != Placed.Begin)
      emitAbsoluteLocation(Placed.Begin, OS);
    uint64_t Address = Placed.Begin;
    for (size_t EventIndex : Placed.Input->EventIndices) {
      if (EventIndex >= Events.size())
        return createStringError("MMIXAL item contains an invalid event index");
      const BufferedEvent &Event = Events[EventIndex];
      switch (Event.Kind) {
      case EventKind::Alignment:
        break;
      case EventKind::Assignment: {
        const MCSymbol *Target = getBareSymbol(*Event.Expression);
        if (!Target)
          return createStringError(
              "MMIXAL defined-target alias requires a bare symbol");
        std::optional<uint64_t> TargetAddress =
            Layout.getSymbolAddress(*Target);
        const auto TargetAlias = AliasAddresses.find(Target);
        if (!TargetAddress && TargetAlias != AliasAddresses.end())
          TargetAddress = TargetAlias->second;
        if (!TargetAddress)
          return createStringError(Twine("MMIXAL alias target '") +
                                   Target->getName() +
                                   "' has no allocated address");
        const bool IsCurrentLocation = *TargetAddress == Address;
        if (!IsCurrentLocation && !DefinedSymbols.contains(Target))
          return createStringError(Twine("MMIXAL alias target '") +
                                   Target->getName() +
                                   "' is not defined before the alias");

        Expected<StringRef> AliasName = Symbols.getMappedSymbol(*Event.Symbol);
        if (!AliasName)
          return AliasName.takeError();
        Expected<StringRef> TargetName = Symbols.getMappedSymbol(*Target);
        if (!TargetName)
          return TargetName.takeError();
        if (!DefinedSymbols.insert(Event.Symbol).second)
          return createStringError(Twine("duplicate MMIXAL definition for '") +
                                   *AliasName + "'");
        AliasAddresses.try_emplace(Event.Symbol, *TargetAddress);
        OS << *AliasName << "\tIS ";
        if (IsCurrentLocation)
          OS << '@';
        else
          OS << *TargetName;
        OS << '\n';
        break;
      }
      case EventKind::Label: {
        Expected<StringRef> Name = Symbols.getMappedSymbol(*Event.Symbol);
        if (!Name)
          return Name.takeError();
        if (!DefinedSymbols.insert(Event.Symbol).second)
          return createStringError(Twine("duplicate MMIXAL definition for '") +
                                   *Name + "'");
        OS << *Name << "\tIS @\n";
        if (Symbols.hasSourceBlockAlias(*Event.Symbol)) {
          Expected<StringRef> Alias =
              Symbols.getSourceBlockAlias(*Event.Symbol);
          if (!Alias)
            return Alias.takeError();
          OS << *Alias << "\tIS @\n";
        }
        break;
      }
      case EventKind::Instruction:
        if (Placed.Input->Group != LogicalGroup::Text)
          return createStringError(
              "MMIXAL instruction is outside executable text");
        for (const MCSymbol *Dependency : Event.Dependencies) {
          Expected<StringRef> Name = Symbols.getMappedSymbol(*Dependency);
          if (!Name)
            return Name.takeError();
        }
        if (Error Err = validateInstructionAddress(
                Event.Inst, Address, *InstPrinter, Layout, AliasAddresses,
                DefinedSymbols, Event.Dependencies))
          return Err;
        if (!Event.STI)
          return createStringError(
              "MMIXAL instruction has no subtarget information");
        InstPrinter->printInstWithSymbolNames(&Event.Inst, Address, *Event.STI,
                                              ResolveSymbol, OS);
        OS << '\n';
        Address += 4;
        break;
      case EventKind::Bytes:
        if (Placed.Input->Group == LogicalGroup::Text)
          return createStringError(
              "MMIXAL raw bytes are unsupported in executable text");
        emitByteArray(OS, Event.Bytes);
        if (Error Err =
                advanceDataAddress(Address, Event.Bytes.size(), Placed.End))
          return Err;
        break;
      case EventKind::Value: {
        if (Placed.Input->Group == LogicalGroup::Text)
          return createStringError(
              "MMIXAL data value is unsupported in executable text");
        if (Event.ValueSize != 1 && Event.ValueSize != 2 &&
            Event.ValueSize != 4 && Event.ValueSize != 8)
          return createStringError(
              "MMIXAL data value width must be 1, 2, 4, or 8 bytes");
        int64_t SignedValue;
        if (Event.Expression->evaluateAsAbsolute(SignedValue)) {
          const uint64_t Value = static_cast<uint64_t>(SignedValue);
          if (Address % Event.ValueSize == 0)
            emitRepeatedScalar(OS, Event.ValueSize, Value, 1);
          else
            emitBigEndianBytes(OS, Value, Event.ValueSize);
        } else {
          if (Event.ValueSize != 8)
            return createStringError(
                "MMIXAL symbolic data value must use OCTA");
          const MCSymbol *Target = getBareSymbol(*Event.Expression);
          if (!Target)
            return createStringError(
                "MMIXAL OCTA value requires a bare symbol");
          Expected<StringRef> Name = Symbols.getMappedSymbol(*Target);
          if (!Name)
            return Name.takeError();
          std::optional<uint64_t> TargetAddress =
              Layout.getSymbolAddress(*Target);
          const auto TargetAlias = AliasAddresses.find(Target);
          if (!TargetAddress && TargetAlias != AliasAddresses.end())
            TargetAddress = TargetAlias->second;
          if (!TargetAddress)
            return createStringError(Twine("MMIXAL OCTA target '") +
                                     Target->getName() +
                                     "' has no allocated definition");
          OS << "\tOCTA " << *Name << '\n';
        }
        if (Error Err =
                advanceDataAddress(Address, Event.ValueSize, Placed.End))
          return Err;
        break;
      }
      case EventKind::ByteFill: {
        if (Placed.Input->Group == LogicalGroup::Text)
          return createStringError(
              "MMIXAL byte fill is unsupported in executable text");
        int64_t SignedCount;
        if (!Event.Expression->evaluateAsAbsolute(SignedCount) ||
            SignedCount < 0)
          return createStringError(
              "MMIXAL byte fill count must be a nonnegative absolute value");
        const uint64_t Count = static_cast<uint64_t>(SignedCount);
        if (truncateToWidth(Event.FillValue, 1) == 0)
          emitExplicitZeros(OS, Address, Count);
        else
          emitRepeatedScalar(OS, 1, Event.FillValue, Count);
        if (Error Err = advanceDataAddress(Address, Count, Placed.End))
          return Err;
        break;
      }
      case EventKind::RepeatedValue: {
        if (Placed.Input->Group == LogicalGroup::Text)
          return createStringError(
              "MMIXAL repeated data is unsupported in executable text");
        int64_t SignedCount;
        if (!Event.Expression->evaluateAsAbsolute(SignedCount) ||
            SignedCount < 0)
          return createStringError(
              "MMIXAL repeated-data count must be a nonnegative absolute "
              "value");
        if (Event.ValueSize > 8)
          return createStringError(
              "MMIXAL repeated-data width must not exceed 8 bytes");
        const uint64_t Count = static_cast<uint64_t>(SignedCount);
        if (Event.ValueSize != 0 &&
            Count > std::numeric_limits<uint64_t>::max() / Event.ValueSize)
          return createStringError("MMIXAL repeated-data size overflows");
        const uint64_t Size = Count * Event.ValueSize;
        if (Event.ValueSize != 0) {
          const uint64_t Value =
              static_cast<uint64_t>(static_cast<uint32_t>(Event.RepeatValue));
          if ((Event.ValueSize == 1 || Event.ValueSize == 2 ||
               Event.ValueSize == 4 || Event.ValueSize == 8) &&
              Address % Event.ValueSize == 0)
            emitRepeatedScalar(OS, Event.ValueSize, Value, Count);
          else
            emitBigEndianBytes(OS, Value, Event.ValueSize, Count);
        }
        if (Error Err = advanceDataAddress(Address, Size, Placed.End))
          return Err;
        break;
      }
      case EventKind::ZeroFill:
        if (Placed.Input->Group != LogicalGroup::ZeroStorage)
          return createStringError("MMIXAL zero fill is outside zero storage");
        if (Event.Symbol) {
          Expected<StringRef> Name = Symbols.getMappedSymbol(*Event.Symbol);
          if (!Name)
            return Name.takeError();
          if (!DefinedSymbols.insert(Event.Symbol).second)
            return createStringError(
                Twine("duplicate MMIXAL definition for '") + *Name + "'");
          OS << *Name << "\tIS @\n";
        }
        emitExplicitZeros(OS, Address, Event.Size);
        if (Error Err = advanceDataAddress(Address, Event.Size, Placed.End))
          return Err;
        break;
      default:
        return createStringError(UnsupportedBufferedModule);
      }
    }
    if (Address != Placed.End)
      return createStringError(
          "MMIXAL item size does not match its buffered contents");
  }
  return Error::success();
}

void MMIXALAsmStreamer::finishImpl() {
  flushCurrentItem();
  if (!ClassificationError.empty()) {
    getContext().reportError(SMLoc(), ClassificationError);
    return;
  }
  if (!Symbols.isFinalized())
    if (Error Err = finalizeSymbolMappings()) {
      getContext().reportError(SMLoc(), toString(std::move(Err)));
      return;
    }
  Expected<MMIXALLayoutPlan> Layout = planMMIXALBareMetalLayout(ItemGroups);
  if (!Layout) {
    getContext().reportError(SMLoc(), toString(Layout.takeError()));
    return;
  }
  std::string Source;
  raw_string_ostream SourceOS(Source);
  if (Error Err = renderModule(*Layout, SourceOS)) {
    getContext().reportError(SMLoc(), toString(std::move(Err)));
    return;
  }
  SourceOS.flush();
  *Output << Source;

  Output->flush();
}
