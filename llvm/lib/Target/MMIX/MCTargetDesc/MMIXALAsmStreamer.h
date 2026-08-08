//===-- MMIXALAsmStreamer.h - Buffer MMIXAL assembly events ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALASMSTREAMER_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALASMSTREAMER_H

#include "MMIXALSymbolTable.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace llvm {

class formatted_raw_ostream;
class MCInstPrinter;
class MCSymbol;

class MMIXALAsmStreamer final : public MCStreamer {
  enum class EventKind {
    Bytes,
    CommonSymbol,
    Instruction,
    Label,
    SymbolAttribute
  };

  struct BufferedEvent {
    EventKind Kind;
    MCInst Inst;
    std::string Bytes;
    const MCSubtargetInfo *STI = nullptr;
    MCSymbol *Symbol = nullptr;
    MCSymbolAttr Attribute = MCSA_Invalid;
    uint64_t Size = 0;
    Align Alignment{1};
  };

  std::unique_ptr<formatted_raw_ostream> Output;
  std::unique_ptr<MCInstPrinter> InstPrinter;
  SmallVector<BufferedEvent, 0> Events;
  MMIXALSymbolTable Symbols;
  std::string ActiveFunctionName;
  SmallVector<const MCSymbol *, 0> PendingFunctionSymbols;
  SmallPtrSet<const MCSymbol *, 8> PendingFunctionSymbolSet;
  SmallVector<const MCSymbol *, 0> PendingModuleSymbols;
  SmallPtrSet<const MCSymbol *, 8> PendingModuleSymbolSet;
  bool HasActiveFunction = false;

  void observeSymbol(const MCSymbol &Symbol);

public:
  MMIXALAsmStreamer(MCContext &Context,
                    std::unique_ptr<formatted_raw_ostream> Output,
                    std::unique_ptr<MCInstPrinter> InstPrinter);
  ~MMIXALAsmStreamer() override;

  void reset() override;
  void emitBytes(StringRef Data) override;
  void emitInstruction(const MCInst &Inst, const MCSubtargetInfo &STI) override;
  void emitLabel(MCSymbol *Symbol, SMLoc Loc = SMLoc()) override;
  void visitUsedSymbol(const MCSymbol &Symbol) override;
  bool emitSymbolAttribute(MCSymbol *Symbol, MCSymbolAttr Attribute) override;
  void emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                        Align ByteAlignment) override;
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
  size_t getNumRegisteredSymbols() const {
    return Symbols.getNumRegisteredSymbols();
  }
  size_t getNumSourceBlockAliases() const {
    return Symbols.getNumSourceBlockAliases();
  }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALASMSTREAMER_H
