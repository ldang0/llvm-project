//===-- MMIXInstPrinter.cpp - Convert MMIX MCInst to assembly syntax ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXInstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "MMIXGenAsmWriter.inc"

void MMIXInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                StringRef Annot, const MCSubtargetInfo &STI,
                                raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void MMIXInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    O << getRegisterName(Op.getReg());
  } else if (Op.isImm()) {
    O << formatImm(Op.getImm());
  } else {
    assert(Op.isExpr() && "expected an MMIX register, immediate, or symbol");
    MAI.printExpr(O, *Op.getExpr());
  }
}

void MMIXInstPrinter::printOperand(const MCInst *MI, uint64_t Address,
                                   unsigned OpNo, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Address != 0 && PrintBranchImmAsAddress && Op.isImm()) {
    constexpr uint64_t InstructionSize = 4;
    const uint64_t Target =
        Address + static_cast<uint64_t>(Op.getImm()) * InstructionSize;
    markup(O, Markup::Target) << formatHex(Target);
    return;
  }
  printOperand(MI, OpNo, O);
}
