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
#include "llvm/Support/FormattedStream.h"
#include <cassert>
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
  BufferedEvent Event{EventKind::Instruction};
  Event.Inst = Inst;
  Event.STI = &STI;
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::emitLabel(MCSymbol *Symbol, SMLoc) {
  assert(Symbol && "cannot emit a null symbol");
  BufferedEvent Event{EventKind::Label};
  Event.Symbol = Symbol;
  Events.push_back(std::move(Event));
}

bool MMIXALAsmStreamer::emitSymbolAttribute(MCSymbol *Symbol,
                                            MCSymbolAttr Attribute) {
  assert(Symbol && "cannot emit an attribute for a null symbol");
  BufferedEvent Event{EventKind::SymbolAttribute};
  Event.Symbol = Symbol;
  Event.Attribute = Attribute;
  Events.push_back(std::move(Event));
  return true;
}

void MMIXALAsmStreamer::emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                         Align ByteAlignment) {
  assert(Symbol && "cannot emit a null common symbol");
  BufferedEvent Event{EventKind::CommonSymbol};
  Event.Symbol = Symbol;
  Event.Size = Size;
  Event.Alignment = ByteAlignment;
  Events.push_back(std::move(Event));
}

void MMIXALAsmStreamer::finishImpl() {
  if (!Events.empty()) {
    getContext().reportError(
        SMLoc(), "MMIXAL buffered module emission is not implemented");
    return;
  }

  Output->flush();
}
