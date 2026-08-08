//===-- MMIXALAsmStreamer.h - Buffer MMIXAL assembly events ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALASMSTREAMER_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALASMSTREAMER_H

#include "MMIXALLayout.h"
#include "MMIXALSymbolTable.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace llvm {

class formatted_raw_ostream;
class MCExpr;
class MCSection;
class MCSymbol;
class MMIXALInstPrinter;
class Twine;

class MMIXALAsmStreamer final : public MCStreamer {
public:
  using LogicalGroup = MMIXALLogicalGroup;
  using AlignmentRequest = MMIXALAlignmentRequest;
  using BufferedItem = MMIXALBufferedItem;

private:
  enum class EventKind {
    Alignment,
    Assignment,
    Bytes,
    CommonSymbol,
    Fill,
    Instruction,
    Label,
    SectionSwitch,
    Value,
    SymbolAttribute
  };

  struct BufferedEvent {
    EventKind Kind;
    MCInst Inst;
    std::string Bytes;
    const MCSubtargetInfo *STI = nullptr;
    MCSymbol *Symbol = nullptr;
    const MCSection *Section = nullptr;
    const MCExpr *Expression = nullptr;
    SmallVector<const MCSymbol *, 2> Dependencies;
    MCSymbolAttr Attribute = MCSA_Invalid;
    uint64_t Size = 0;
    uint64_t FillValue = 0;
    int64_t RepeatValue = 0;
    unsigned ValueSize = 0;
    uint32_t Subsection = 0;
    Align Alignment{1};
    int64_t AlignmentFill = 0;
    uint8_t AlignmentFillLength = 1;
    unsigned MaxBytesToEmit = 0;
    bool IsCodeAlignment = false;
  };

  std::unique_ptr<formatted_raw_ostream> Output;
  std::unique_ptr<MMIXALInstPrinter> InstPrinter;
  SmallVector<BufferedEvent, 0> Events;
  MMIXALItemGroups ItemGroups;
  std::optional<BufferedItem> CurrentItem;
  SmallVector<const MCSymbol *, 2> *DependencySink = nullptr;
  std::string ClassificationError;
  uint64_t NextItemOrder = 0;
  MMIXALSymbolTable Symbols;
  std::string ActiveFunctionName;
  SmallVector<const MCSymbol *, 0> PendingFunctionSymbols;
  SmallPtrSet<const MCSymbol *, 8> PendingFunctionSymbolSet;
  SmallVector<const MCSymbol *, 0> PendingModuleSymbols;
  SmallPtrSet<const MCSymbol *, 8> PendingModuleSymbolSet;
  bool HasActiveFunction = false;

  static size_t getGroupIndex(LogicalGroup Group);
  void recordClassificationError(const Twine &Message);
  Expected<LogicalGroup> classifySection(const MCSection &Section,
                                         uint32_t Subsection) const;
  std::optional<LogicalGroup> classifyCurrentSection();
  BufferedItem *getOrCreateCurrentItem();
  void flushCurrentItem();
  void appendEventToCurrentItem(BufferedEvent Event,
                                std::optional<uint64_t> Size,
                                bool IsPayload = true);
  void updateCurrentItemGroupForSymbol(const MCSymbol &Symbol);
  void observeSymbol(const MCSymbol &Symbol);
  Error renderTextModule(const MMIXALLayoutPlan &Layout, raw_ostream &OS);

public:
  MMIXALAsmStreamer(MCContext &Context,
                    std::unique_ptr<formatted_raw_ostream> Output,
                    std::unique_ptr<MMIXALInstPrinter> InstPrinter);
  ~MMIXALAsmStreamer() override;

  void reset() override;
  void switchSection(MCSection *Section, uint32_t Subsection = 0) override;
  void emitBytes(StringRef Data) override;
  void emitInstruction(const MCInst &Inst, const MCSubtargetInfo &STI) override;
  void emitLabel(MCSymbol *Symbol, SMLoc Loc = SMLoc()) override;
  void emitAssignment(MCSymbol *Symbol, const MCExpr *Value) override;
  void visitUsedSymbol(const MCSymbol &Symbol) override;
  bool emitSymbolAttribute(MCSymbol *Symbol, MCSymbolAttr Attribute) override;
  void emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                        Align ByteAlignment) override;
  void emitLocalCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                             Align ByteAlignment) override;
  void emitZerofill(MCSection *Section, MCSymbol *Symbol = nullptr,
                    uint64_t Size = 0, Align ByteAlignment = Align(1),
                    SMLoc Loc = SMLoc()) override;
  void emitTBSSSymbol(MCSection *Section, MCSymbol *Symbol, uint64_t Size,
                      Align ByteAlignment = Align(1)) override;
  void emitValueImpl(const MCExpr *Value, unsigned Size,
                     SMLoc Loc = SMLoc()) override;
  void emitFill(const MCExpr &NumBytes, uint64_t FillValue,
                SMLoc Loc = SMLoc()) override;
  void emitFill(const MCExpr &NumValues, int64_t Size, int64_t Expr,
                SMLoc Loc = SMLoc()) override;
  void emitValueToAlignment(Align Alignment, int64_t Fill = 0,
                            uint8_t FillLength = 1,
                            unsigned MaxBytesToEmit = 0) override;
  void emitCodeAlignment(Align Alignment, const MCSubtargetInfo &STI,
                         unsigned MaxBytesToEmit = 0) override;
  void finishImpl() override;

  Error registerUserSymbol(const MCSymbol &Symbol, StringRef RawName);
  Error registerSourceBlock(const MCSymbol &AddressSymbol,
                            StringRef FunctionName, StringRef BlockName);
  Error registerFunctionPrivateSymbol(const MCSymbol &Symbol,
                                      StringRef FunctionName,
                                      MMIXALSymbolTable::PrivateSymbolKind Kind,
                                      uint64_t Ordinal);
  Error registerModulePrivateSymbol(const MCSymbol &Symbol,
                                    MMIXALSymbolTable::PrivateSymbolKind Kind,
                                    uint64_t Ordinal);
  Error beginFunctionSymbols(StringRef FunctionName);
  Error endFunctionSymbols(const MCSymbol *FunctionEnd);
  Error finalizeSymbolMappings();
  Expected<StringRef> getMappedSymbol(const MCSymbol &Symbol) const;
  Expected<StringRef> getSourceBlockAlias(const MCSymbol &AddressSymbol) const;

  size_t getNumBufferedEvents() const { return Events.size(); }
  ArrayRef<BufferedItem> getBufferedItems(LogicalGroup Group);
  size_t getNumBufferedItems();
  bool hasClassificationError() const { return !ClassificationError.empty(); }
  size_t getNumRegisteredSymbols() const {
    return Symbols.getNumRegisteredSymbols();
  }
  size_t getNumSourceBlockAliases() const {
    return Symbols.getNumSourceBlockAliases();
  }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALASMSTREAMER_H
