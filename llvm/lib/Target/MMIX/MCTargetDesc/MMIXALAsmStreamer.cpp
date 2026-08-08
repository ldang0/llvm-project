//===-- MMIXALAsmStreamer.cpp - Buffer MMIXAL assembly events ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALAsmStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormattedStream.h"
#include <cassert>
#include <system_error>
#include <utility>

using namespace llvm;

MMIXALAsmStreamer::MMIXALAsmStreamer(
    MCContext &Context, std::unique_ptr<formatted_raw_ostream> Output,
    std::unique_ptr<MCInstPrinter> InstPrinter)
    : MCStreamer(Context), Output(std::move(Output)),
      InstPrinter(std::move(InstPrinter)) {
  assert(this->Output && "MMIXAL streamer requires an output stream");
  assert(this->InstPrinter &&
         "MMIXAL streamer requires an instruction printer");
}

MMIXALAsmStreamer::~MMIXALAsmStreamer() = default;

void MMIXALAsmStreamer::reset() {
  MCStreamer::reset();
  Events.clear();
  Symbols.reset();
  ActiveFunctionName.clear();
  PendingFunctionSymbols.clear();
  PendingFunctionSymbolSet.clear();
  PendingModuleSymbols.clear();
  PendingModuleSymbolSet.clear();
  HasActiveFunction = false;
}

void MMIXALAsmStreamer::emitBytes(StringRef Data) {
  if (Data.empty())
    return;
  BufferedEvent Event{EventKind::Bytes};
  Event.Bytes = Data.str();
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::emitInstruction(const MCInst &Inst,
                                        const MCSubtargetInfo &STI) {
  MCStreamer::emitInstruction(Inst, STI);
  BufferedEvent Event{EventKind::Instruction};
  Event.Inst = Inst;
  Event.STI = &STI;
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::emitLabel(MCSymbol *Symbol, SMLoc) {
  assert(Symbol && "cannot emit a null symbol");
  observeSymbol(*Symbol);
  BufferedEvent Event{EventKind::Label};
  Event.Symbol = Symbol;
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::visitUsedSymbol(const MCSymbol &Symbol) {
  observeSymbol(Symbol);
}

bool MMIXALAsmStreamer::emitSymbolAttribute(MCSymbol *Symbol,
                                            MCSymbolAttr Attribute) {
  assert(Symbol && "cannot emit an attribute for a null symbol");
  observeSymbol(*Symbol);
  BufferedEvent Event{EventKind::SymbolAttribute};
  Event.Symbol = Symbol;
  Event.Attribute = Attribute;
  Events.push_back(std::move(Event));
  return true;
}

void MMIXALAsmStreamer::emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                         Align ByteAlignment) {
  assert(Symbol && "cannot emit a null common symbol");
  observeSymbol(*Symbol);
  BufferedEvent Event{EventKind::CommonSymbol};
  Event.Symbol = Symbol;
  Event.Size = Size;
  Event.Alignment = ByteAlignment;
  Events.push_back(std::move(Event));
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

void MMIXALAsmStreamer::finishImpl() {
  if (!Symbols.isFinalized())
    if (Error Err = finalizeSymbolMappings()) {
      getContext().reportError(SMLoc(), toString(std::move(Err)));
      return;
    }
  if (!Events.empty()) {
    getContext().reportError(
        SMLoc(), "MMIXAL buffered module emission is not implemented");
    return;
  }

  Output->flush();
}
