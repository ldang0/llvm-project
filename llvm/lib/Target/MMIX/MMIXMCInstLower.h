//===-- MMIXMCInstLower.h - Lower MMIX MachineInstr to MCInst -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXMCINSTLOWER_H
#define LLVM_LIB_TARGET_MMIX_MMIXMCINSTLOWER_H

namespace llvm {

class AsmPrinter;
class MCContext;
class MCExpr;
class MCInst;
class MCOperand;
class MCSymbol;
class MachineInstr;
class MachineOperand;

class MMIXMCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

  const MCExpr *lowerSymbolOperand(const MachineOperand &MO,
                                   MCSymbol *Symbol) const;
  MCOperand lowerOperand(const MachineOperand &MO) const;

public:
  MMIXMCInstLower(MCContext &Ctx, AsmPrinter &Printer)
      : Ctx(Ctx), Printer(Printer) {}

  void lower(const MachineInstr &MI, MCInst &OutMI) const;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXMCINSTLOWER_H
