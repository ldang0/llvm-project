//===-- MMIXALInstPrinter.cpp - Print MMIXAL assembly syntax -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALInstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "MMIXGenMMIXALAsmWriter.inc"

void MMIXALInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                  StringRef Annot, const MCSubtargetInfo &STI,
                                  raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void MMIXALInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
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

void MMIXALInstPrinter::printOperand(const MCInst *MI, uint64_t Address,
                                     unsigned OpNo, raw_ostream &O) {
  (void)Address;
  printOperand(MI, OpNo, O);
}
