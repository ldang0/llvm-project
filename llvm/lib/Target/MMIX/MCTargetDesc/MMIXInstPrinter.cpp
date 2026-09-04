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

void MMIXInstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
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

void MMIXInstPrinter::printOperand(const MCInst *MI, uint64_t /*Address*/,
                                   unsigned OpNo, raw_ostream &O) {
  // Canonical MMIX syntax spells PC-relative targets as word displacements.
  // Printing a resolved address would change the meaning when reassembled.
  printOperand(MI, OpNo, O);
}
