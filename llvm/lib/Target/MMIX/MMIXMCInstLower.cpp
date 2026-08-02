//===-- MMIXMCInstLower.cpp - Lower MMIX MachineInstr to MCInst ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXMCInstLower.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

const MCExpr *MMIXMCInstLower::lowerSymbolOperand(const MachineOperand &MO,
                                                  MCSymbol *Symbol) const {
  if (MO.getTargetFlags())
    report_fatal_error("MMIX symbol operand has unsupported target flags");

  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Ctx);
  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
  return Expr;
}

MCOperand MMIXMCInstLower::lowerOperand(const MachineOperand &MO) const {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    if (MO.isImplicit())
      return MCOperand();
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
    if (MO.getTargetFlags())
      report_fatal_error("MMIX basic-block operand has unsupported target "
                         "flags");
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Ctx));
  case MachineOperand::MO_GlobalAddress:
    return MCOperand::createExpr(
        lowerSymbolOperand(MO, Printer.getSymbol(MO.getGlobal())));
  case MachineOperand::MO_ExternalSymbol:
    return MCOperand::createExpr(lowerSymbolOperand(
        MO, Printer.GetExternalSymbolSymbol(MO.getSymbolName())));
  case MachineOperand::MO_BlockAddress:
    return MCOperand::createExpr(lowerSymbolOperand(
        MO, Printer.GetBlockAddressSymbol(MO.getBlockAddress())));
  case MachineOperand::MO_ConstantPoolIndex:
    return MCOperand::createExpr(
        lowerSymbolOperand(MO, Printer.GetCPISymbol(MO.getIndex())));
  case MachineOperand::MO_JumpTableIndex:
    return MCOperand::createExpr(
        lowerSymbolOperand(MO, Printer.GetJTISymbol(MO.getIndex())));
  case MachineOperand::MO_MCSymbol:
    return MCOperand::createExpr(lowerSymbolOperand(MO, MO.getMCSymbol()));
  case MachineOperand::MO_RegisterMask:
  case MachineOperand::MO_RegisterLiveOut:
    return MCOperand();
  default:
    report_fatal_error("MMIX cannot lower this machine operand to MC");
  }
}

void MMIXMCInstLower::lower(const MachineInstr &MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI.getOpcode());
  for (const MachineOperand &MO : MI.operands()) {
    MCOperand MCOp = lowerOperand(MO);
    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}
