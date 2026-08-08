//===-- MMIXALAsmStreamer.cpp - Buffer MMIXAL assembly events ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALAsmStreamer.h"
#include "MMIXALDependencyGraph.h"
#include "MMIXALExpression.h"
#include "MMIXALInstPrinter.h"
#include "MMIXBaseInfo.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"
#include <cassert>
#include <functional>
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
    const size_t SeparatorLength = LineLength == 0 ? 0 : 1;
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
      OS << ',';
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

void emitAbsoluteLocation(uint64_t Address, raw_ostream &OS) {
  OS << "\tLOC #" << format_hex_no_prefix(Address, 16, /*Upper=*/true) << '\n';
}

void printMMIXALSourceInstruction(function_ref<void(raw_ostream &)> Print,
                                  raw_ostream &OS) {
  SmallString<128> Buffer;
  raw_svector_ostream BufferOS(Buffer);
  Print(BufferOS);

  StringRef Remaining = Buffer;
  while (true) {
    const size_t Separator = Remaining.find(", ");
    if (Separator == StringRef::npos)
      break;
    OS << Remaining.take_front(Separator + 1);
    Remaining = Remaining.drop_front(Separator + 2);
  }
  OS << Remaining;
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
  const MCSymbol *TargetSymbol = getBareMMIXALSymbol(TargetExpr);
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
    : MMIXALAsmStreamer(Context, std::move(Output), std::move(InstPrinter),
                        nullptr) {}

MMIXALAsmStreamer::MMIXALAsmStreamer(
    MCContext &Context, std::unique_ptr<formatted_raw_ostream> Output,
    std::unique_ptr<MMIXALInstPrinter> InstPrinter,
    std::unique_ptr<MCCodeEmitter> CodeEmitter)
    : MCStreamer(Context), Output(std::move(Output)),
      InstPrinter(std::move(InstPrinter)), CodeEmitter(std::move(CodeEmitter)) {
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

void MMIXALAsmStreamer::recordUnsupportedEvent(StringRef Event) {
  recordClassificationError(Twine("MMIXAL does not support ") + Event +
                            " events");
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

void MMIXALAsmStreamer::emitCFIStartProcImpl(MCDwarfFrameInfo &Frame) {
  recordUnsupportedEvent("CFI/unwind");
}

void MMIXALAsmStreamer::emitCFIEndProcImpl(MCDwarfFrameInfo &Frame) {
  MCStreamer::emitCFIEndProcImpl(Frame);
  recordUnsupportedEvent("CFI/unwind");
}

void MMIXALAsmStreamer::emitRawTextImpl(StringRef Text) {
  recordUnsupportedEvent("opaque assembly text");
}

void MMIXALAsmStreamer::reset() {
  MCStreamer::reset();
  if (CodeEmitter)
    CodeEmitter->reset();
  PreludeGlobalRegisters.clear();
  PreludeNames.clear();
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
  ModuleLocalSymbols.clear();
  HasActiveFunction = false;
  IsPreludeFinalized = false;
}

void MMIXALAsmStreamer::changeSection(MCSection *Section, uint32_t Subsection) {
  flushCurrentItem();
  MCStreamer::changeSection(Section, Subsection);

  bool IsSuppressibleNote = false;
  if (getContext().getObjectFileType() == MCContext::IsELF) {
    const auto &ELFSection = static_cast<const MCSectionELF &>(*Section);
    IsSuppressibleNote = Subsection == 0 && ELFSection.getFlags() == 0 &&
                         ELFSection.getEntrySize() == 0 &&
                         (ELFSection.getType() == ELF::SHT_NOTE ||
                          (ELFSection.getType() == ELF::SHT_PROGBITS &&
                           ELFSection.getName() == ".note.GNU-stack"));
  }
  if (!IsSuppressibleNote)
    if (Expected<LogicalGroup> Group = classifySection(*Section, Subsection);
        !Group)
      recordClassificationError(toString(Group.takeError()));

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
  if (Item && Item->Group == LogicalGroup::ZeroStorage &&
      llvm::any_of(Data.bytes(), [](unsigned char Byte) { return Byte != 0; }))
    recordClassificationError(
        "MMIXAL zero-storage section contains nonzero bytes");
  appendEventToCurrentItem(std::move(Event), Data.size());
}

void MMIXALAsmStreamer::emitInstruction(const MCInst &Inst,
                                        const MCSubtargetInfo &STI) {
  if (!getCurrentSection().first) {
    printMMIXALSourceInstruction(
        [&](raw_ostream &OS) { InstPrinter->printInst(&Inst, 0, "", STI, OS); },
        *Output);
    if (CodeEmitter) {
      SmallString<16> Code;
      SmallVector<MCFixup, 4> Fixups;
      CodeEmitter->encodeInstruction(Inst, Code, Fixups, STI);
      assert(Fixups.empty() &&
             "disassembled MMIX instruction unexpectedly requires a fixup");
      Output->PadToColumn(getContext().getAsmInfo().getCommentColumn());
      *Output << getContext().getAsmInfo().getCommentString() << " encoding: [";
      for (size_t I = 0; I != Code.size(); ++I) {
        if (I != 0)
          *Output << ',';
        *Output << format("0x%02x", static_cast<uint8_t>(Code[I]));
      }
      *Output << ']';
    }
    *Output << '\n';
    return;
  }

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

void MMIXALAsmStreamer::emitLabel(MCSymbol *Symbol, SMLoc Loc) {
  assert(Symbol && "cannot emit a null symbol");
  observeSymbol(*Symbol);
  MCStreamer::emitLabel(Symbol, Loc);
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

void MMIXALAsmStreamer::emitConditionalAssignment(MCSymbol *Symbol,
                                                  const MCExpr *Value) {
  recordUnsupportedEvent("conditional symbol assignment");
}

void MMIXALAsmStreamer::emitWeakReference(MCSymbol *Alias,
                                          const MCSymbol *Symbol) {
  recordUnsupportedEvent("weak symbol reference");
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
  if (IsModuleEmission && Attribute == MCSA_Global)
    return true;
  if (IsModuleEmission && Attribute == MCSA_Local) {
    ModuleLocalSymbols.insert(Symbol);
    return true;
  }
  if (Attribute == MCSA_ELF_TypeFunction || Attribute == MCSA_ELF_TypeObject ||
      Attribute == MCSA_ELF_TypeNoType)
    return true;
  recordUnsupportedEvent("symbol linkage or visibility");
  return true;
}

void MMIXALAsmStreamer::emitEHSymAttributes(const MCSymbol *Symbol,
                                            MCSymbol *EHSymbol) {
  recordUnsupportedEvent("exception-handling symbol attribute");
}

void MMIXALAsmStreamer::emitELFSize(MCSymbol *Symbol, const MCExpr *Value) {
  if (IsModuleEmission)
    return;
  recordUnsupportedEvent("ELF symbol size");
}

void MMIXALAsmStreamer::emitELFSymverDirective(const MCSymbol *OriginalSym,
                                               StringRef Name,
                                               bool KeepOriginalSym) {
  recordUnsupportedEvent("ELF symbol version");
}

void MMIXALAsmStreamer::emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                         Align ByteAlignment) {
  assert(Symbol && "cannot emit a null common symbol");
  observeSymbol(*Symbol);
  if (IsModuleEmission && ModuleLocalSymbols.erase(Symbol)) {
    emitZerofill(getContext().getObjectFileInfo()->getBSSSection(), Symbol,
                 Size, ByteAlignment);
    return;
  }
  recordUnsupportedEvent("common-symbol allocation");
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
  recordUnsupportedEvent("thread-local zero-storage allocation");
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

void MMIXALAsmStreamer::emitULEB128Value(const MCExpr *Value) {
  recordUnsupportedEvent("ULEB128 data");
}

void MMIXALAsmStreamer::emitSLEB128Value(const MCExpr *Value) {
  recordUnsupportedEvent("SLEB128 data");
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

void MMIXALAsmStreamer::emitNops(int64_t NumBytes, int64_t ControlledNopLength,
                                 SMLoc Loc, const MCSubtargetInfo &STI) {
  recordUnsupportedEvent("explicit nop padding");
}

void MMIXALAsmStreamer::emitPrefAlign(Align Alignment, const MCSymbol &End,
                                      bool EmitNops, uint8_t Fill,
                                      const MCSubtargetInfo &STI) {
  recordUnsupportedEvent("prefix alignment");
}

void MMIXALAsmStreamer::emitValueToOffset(const MCExpr *Offset,
                                          unsigned char Value, SMLoc Loc) {
  recordUnsupportedEvent("address-offset padding");
}

void MMIXALAsmStreamer::emitFileDirective(StringRef Filename) {}

void MMIXALAsmStreamer::emitFileDirective(StringRef Filename,
                                          StringRef CompilerVersion,
                                          StringRef TimeStamp,
                                          StringRef Description) {}

void MMIXALAsmStreamer::emitIdent(StringRef IdentString) {}

Expected<unsigned> MMIXALAsmStreamer::tryEmitDwarfFileDirective(
    unsigned FileNo, StringRef Directory, StringRef Filename,
    std::optional<MD5::MD5Result> Checksum, std::optional<StringRef> Source,
    unsigned CUID) {
  return MCStreamer::tryEmitDwarfFileDirective(FileNo, Directory, Filename,
                                               Checksum, Source, CUID);
}

void MMIXALAsmStreamer::emitDwarfFile0Directive(
    StringRef Directory, StringRef Filename,
    std::optional<MD5::MD5Result> Checksum, std::optional<StringRef> Source,
    unsigned CUID) {
  MCStreamer::emitDwarfFile0Directive(Directory, Filename, Checksum, Source,
                                      CUID);
}

void MMIXALAsmStreamer::emitDwarfLocDirective(unsigned FileNo, unsigned Line,
                                              unsigned Column, unsigned Flags,
                                              unsigned Isa,
                                              unsigned Discriminator,
                                              StringRef FileName,
                                              StringRef Comment) {
  MCStreamer::emitDwarfLocDirective(FileNo, Line, Column, Flags, Isa,
                                    Discriminator, FileName, Comment);
}

void MMIXALAsmStreamer::emitDwarfLocLabelDirective(SMLoc Loc, StringRef Name) {
  recordUnsupportedEvent("DWARF location label");
}

void MMIXALAsmStreamer::emitDwarfLineStartLabel(MCSymbol *StartSym) {
  recordUnsupportedEvent("address-bearing DWARF line table");
}

void MMIXALAsmStreamer::emitDwarfLineEndEntry(MCSection *Section,
                                              MCSymbol *LastLabel,
                                              MCSymbol *EndLabel) {
  recordUnsupportedEvent("address-bearing DWARF line table");
}

void MMIXALAsmStreamer::emitDwarfAdvanceLineAddr(int64_t LineDelta,
                                                 const MCSymbol *LastLabel,
                                                 const MCSymbol *Label,
                                                 unsigned PointerSize) {
  recordUnsupportedEvent("address-bearing DWARF line table");
}

void MMIXALAsmStreamer::emitCFISections(bool EH, bool Debug, bool SFrame) {
  recordUnsupportedEvent("CFI/unwind section selection");
}

void MMIXALAsmStreamer::emitSyntaxDirective(StringRef Syntax,
                                            StringRef Options) {
  recordUnsupportedEvent("source syntax directive");
}

void MMIXALAsmStreamer::emitRelocDirective(const MCExpr &Offset, StringRef Name,
                                           const MCExpr *Expr, SMLoc Loc) {
  recordUnsupportedEvent("explicit relocation");
}

void MMIXALAsmStreamer::emitAddrsig() {
  recordUnsupportedEvent("address-significance table");
}

void MMIXALAsmStreamer::emitAddrsigSym(const MCSymbol *Symbol) {
  recordUnsupportedEvent("address-significance symbol");
}

Error MMIXALAsmStreamer::registerUserSymbol(const MCSymbol &Symbol,
                                            StringRef RawName) {
  return Symbols.registerUserSymbol(Symbol, RawName);
}

Error MMIXALAsmStreamer::registerEntrySymbol(const MCSymbol &Symbol,
                                             StringRef RawName) {
  return Symbols.registerEntrySymbol(Symbol, RawName);
}

void MMIXALAsmStreamer::addPreludeGlobalRegister(StringRef Name,
                                                 unsigned AllocatedRegister,
                                                 uint64_t InitialValue) {
  if (IsPreludeFinalized) {
    recordClassificationError(
        "cannot add an MMIXAL prelude declaration after finalization");
    return;
  }
  if (!MMIXALSymbolMapper::isValidOrdinarySymbol(Name) ||
      !Name.starts_with("__LLVM_G_")) {
    recordClassificationError(
        Twine("invalid target-owned MMIXAL prelude name: ") + Name);
    return;
  }
  if (!PreludeNames.insert(Name).second) {
    recordClassificationError(
        Twine("duplicate target-owned MMIXAL prelude name: ") + Name);
    return;
  }

  if (!PreludeGlobalRegisters.empty() &&
      PreludeGlobalRegisters.back().AllocatedRegister == 0) {
    recordClassificationError(
        "MMIXAL GREG allocation cannot continue below $0");
    return;
  }

  const unsigned ExpectedRegister =
      PreludeGlobalRegisters.empty()
          ? 254
          : PreludeGlobalRegisters.back().AllocatedRegister - 1;
  if (AllocatedRegister != ExpectedRegister) {
    recordClassificationError(
        Twine("MMIXAL GREG declaration for $") + Twine(AllocatedRegister) +
        " is out of allocation order; expected $" + Twine(ExpectedRegister));
    return;
  }

  PreludeGlobalRegisters.push_back(
      {Name.str(), AllocatedRegister, InitialValue});
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
  if (!Symbol.isTemporary()) {
    if (Error Err = Symbols.registerUserSymbol(Symbol, Symbol.getName()))
      recordClassificationError(toString(std::move(Err)));
    return;
  }
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

Expected<SmallVector<const MMIXALPlacedItem *, 0>>
MMIXALAsmStreamer::scheduleItems(
    const MMIXALLayoutPlan &Layout,
    DenseMap<const MCSymbol *, uint64_t> &AliasAddresses) const {
  struct AliasDefinition {
    const MCSymbol *Alias;
    const MCSymbol *Target;
    size_t Item;
  };

  const ArrayRef<MMIXALPlacedItem> Items = Layout.getItems();
  DenseMap<const MCSymbol *, size_t> DefinitionItems;
  DenseMap<const MCSymbol *, const MCSymbol *> AliasTargets;
  SmallVector<AliasDefinition, 0> Aliases;
  SmallVector<MMIXALItemDependencies, 0> Dependencies(Items.size());

  auto RegisterDefinition = [&](const MCSymbol &Symbol, size_t Item) -> Error {
    if (!DefinitionItems.try_emplace(&Symbol, Item).second)
      return createStringError(Twine("duplicate MMIXAL definition for '") +
                               Symbol.getName() + "'");
    return Error::success();
  };

  for (size_t Item = 0; Item != Items.size(); ++Item) {
    for (const MCSymbol *Symbol : Items[Item].Input->OwningSymbols)
      if (Error Err = RegisterDefinition(*Symbol, Item))
        return std::move(Err);
    for (size_t EventIndex : Items[Item].Input->EventIndices) {
      if (EventIndex >= Events.size())
        return createStringError("MMIXAL item contains an invalid event index");
      const BufferedEvent &Event = Events[EventIndex];
      if (Event.Kind != EventKind::Assignment)
        continue;
      if (Error Err = validateMMIXALExpression(*Event.Expression))
        return std::move(Err);
      const MCSymbol *Target = getBareMMIXALSymbol(*Event.Expression);
      if (!Target)
        return createStringError(
            "MMIXAL defined-target alias requires a bare symbol");
      if (Error Err = RegisterDefinition(*Event.Symbol, Item))
        return std::move(Err);
      AliasTargets.try_emplace(Event.Symbol, Target);
      Aliases.push_back({Event.Symbol, Target, Item});
    }
  }

  auto RequireDefinitions = [&](size_t Item, ArrayRef<const MCSymbol *> Symbols,
                                bool AddEdges) -> Error {
    for (const MCSymbol *Symbol : Symbols) {
      if (Expected<StringRef> Name = this->Symbols.getMappedSymbol(*Symbol);
          !Name)
        return Name.takeError();
      const auto Definition = DefinitionItems.find(Symbol);
      if (Definition == DefinitionItems.end())
        return createStringError(Twine("MMIXAL symbol '") + Symbol->getName() +
                                 "' has no allocated definition");
      if (AddEdges && Definition->second != Item &&
          !llvm::is_contained(Dependencies[Item], Definition->second))
        Dependencies[Item].push_back(Definition->second);
    }
    return Error::success();
  };

  for (const AliasDefinition &Alias : Aliases) {
    const MCSymbol *Target[] = {Alias.Target};
    if (Error Err = RequireDefinitions(Alias.Item, Target, true))
      return std::move(Err);
  }

  for (size_t Item = 0; Item != Items.size(); ++Item) {
    for (size_t EventIndex : Items[Item].Input->EventIndices) {
      const BufferedEvent &Event = Events[EventIndex];
      if (Event.Expression && Event.Kind != EventKind::Assignment)
        if (Error Err = validateMMIXALExpression(*Event.Expression))
          return std::move(Err);

      if (Event.Kind == EventKind::Value) {
        int64_t Absolute;
        if (Event.Expression->evaluateAsAbsolute(Absolute))
          continue;
        const MCSymbol *BareOCTATarget =
            Event.ValueSize == 8 ? getBareMMIXALSymbol(*Event.Expression)
                                 : nullptr;
        if (BareOCTATarget && !DefinitionItems.contains(BareOCTATarget))
          return createStringError(Twine("MMIXAL OCTA target '") +
                                   BareOCTATarget->getName() +
                                   "' has no allocated definition");
        if (Error Err =
                RequireDefinitions(Item, Event.Dependencies, !BareOCTATarget))
          return std::move(Err);
        continue;
      }

      if (Event.Kind != EventKind::Instruction)
        continue;
      for (const MCOperand &Operand : Event.Inst)
        if (Operand.isExpr())
          if (Error Err = validateMMIXALExpression(*Operand.getExpr()))
            return std::move(Err);

      const MCInstrDesc &Desc =
          InstPrinter->getInstructionDesc(Event.Inst.getOpcode());
      const MMIXII::MMIXALSelectionKind Selection =
          MMIXII::getMMIXALSelection(Desc.TSFlags);
      const bool IsRelative = Selection == MMIXII::MMIXALSelectionForward ||
                              Selection == MMIXII::MMIXALSelectionBackward;
      if (!IsRelative) {
        if (Error Err = RequireDefinitions(Item, Event.Dependencies, true))
          return std::move(Err);
        continue;
      }

      if (Desc.getNumOperands() == 0 ||
          Desc.getNumOperands() > Event.Inst.getNumOperands())
        return createStringError("MMIXAL relative instruction has no target");
      const MCOperand &Target =
          Event.Inst.getOperand(Desc.getNumOperands() - 1);
      if (!Target.isExpr())
        return createStringError(
            "MMIXAL source relative target requires an expression");
      int64_t Absolute;
      if (!Target.getExpr()->evaluateAsAbsolute(Absolute) &&
          !getBareMMIXALSymbol(*Target.getExpr()))
        return createStringError(
            "MMIXAL relative target must be an absolute address or bare "
            "symbol");
      if (Error Err = RequireDefinitions(Item, Event.Dependencies, false))
        return std::move(Err);
    }
  }

  Expected<SmallVector<size_t, 0>> Order = scheduleMMIXALItems(Dependencies);
  if (!Order)
    return Order.takeError();

  DenseSet<const MCSymbol *> ResolvingAliases;
  std::function<Expected<uint64_t>(const MCSymbol &)> ResolveAddress =
      [&](const MCSymbol &Symbol) -> Expected<uint64_t> {
    if (std::optional<uint64_t> Address = Layout.getSymbolAddress(Symbol))
      return *Address;
    if (const auto Address = AliasAddresses.find(&Symbol);
        Address != AliasAddresses.end())
      return Address->second;
    const auto Target = AliasTargets.find(&Symbol);
    if (Target == AliasTargets.end())
      return createStringError(Twine("MMIXAL symbol '") + Symbol.getName() +
                               "' has no allocated definition");
    if (!ResolvingAliases.insert(&Symbol).second)
      return createStringError(Twine("MMIXAL alias dependency cycle at '") +
                               Symbol.getName() + "'");
    Expected<uint64_t> Address = ResolveAddress(*Target->second);
    ResolvingAliases.erase(&Symbol);
    if (!Address)
      return Address.takeError();
    AliasAddresses.try_emplace(&Symbol, *Address);
    return *Address;
  };

  for (const AliasDefinition &Alias : Aliases)
    if (Expected<uint64_t> Address = ResolveAddress(*Alias.Alias); !Address)
      return Address.takeError();

  SmallVector<const MMIXALPlacedItem *, 0> ScheduledItems;
  ScheduledItems.reserve(Order->size());
  for (size_t Item : *Order)
    ScheduledItems.push_back(&Items[Item]);
  return ScheduledItems;
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

  DenseMap<const MCSymbol *, uint64_t> AliasAddresses;
  Expected<SmallVector<const MMIXALPlacedItem *, 0>> ScheduledItems =
      scheduleItems(Layout, AliasAddresses);
  if (!ScheduledItems)
    return ScheduledItems.takeError();

  DenseSet<const MCSymbol *> DefinedSymbols;
  auto ResolveSymbol =
      [this, &DefinedSymbols](const MCSymbol &Symbol) -> MMIXALSymbolPrintInfo {
    return {cantFail(Symbols.getMappedSymbol(Symbol)),
            DefinedSymbols.contains(&Symbol)};
  };

  renderPrelude(OS);

  for (const MMIXALPlacedItem *Scheduled : *ScheduledItems) {
    const MMIXALPlacedItem &Placed = *Scheduled;
    std::optional<uint64_t> CurrentLocation;
    for (const MMIXALPaddingInterval &Padding : Placed.Padding) {
      if (!Padding.Request.IsCodeAlignment) {
        if (Padding.Request.Fill != 0)
          return createStringError(
              "MMIXAL data alignment with nonzero fill is unsupported");
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
        printMMIXALSourceInstruction(
            [&](raw_ostream &InstructionOS) {
              InstPrinter->printInstWithSymbolNames(
                  &Nop, Address, *Padding.Request.STI, ResolveSymbol,
                  InstructionOS);
            },
            OS);
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
        const MCSymbol *Target = getBareMMIXALSymbol(*Event.Expression);
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
        printMMIXALSourceInstruction(
            [&](raw_ostream &InstructionOS) {
              InstPrinter->printInstWithSymbolNames(&Event.Inst, Address,
                                                    *Event.STI, ResolveSymbol,
                                                    InstructionOS);
            },
            OS);
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
          if (Address % Event.ValueSize != 0)
            return createStringError(
                "MMIXAL symbolic data value is not naturally aligned");
          const bool AllowsBareFutureSymbol =
              Event.ValueSize == 8 &&
              getBareMMIXALSymbol(*Event.Expression) != nullptr;
          if (!AllowsBareFutureSymbol)
            for (const MCSymbol *Dependency : Event.Dependencies)
              if (!DefinedSymbols.contains(Dependency))
                return createStringError(
                    Twine("MMIXAL data expression references symbol '") +
                    Dependency->getName() + "' before its definition");
          OS << '\t' << getDataDirective(Event.ValueSize) << ' ';
          if (Error Err = printMMIXALExpression(*Event.Expression,
                                                getContext().getAsmInfo(),
                                                ResolveSymbol, OS))
            return Err;
          OS << '\n';
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

void MMIXALAsmStreamer::renderPrelude(raw_ostream &OS) const {
  for (const PreludeGlobalRegister &Declaration : PreludeGlobalRegisters) {
    OS << Declaration.Name << "\tGREG ";
    if (Declaration.InitialValue == 0)
      OS << '0';
    else
      OS << '#'
         << format_hex_no_prefix(Declaration.InitialValue, 16, /*Upper=*/true);
    OS << '\n';
  }
}

void MMIXALAsmStreamer::finishImpl() {
  flushCurrentItem();
  IsPreludeFinalized = true;
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
