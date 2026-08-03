//===-- MMIXMCInstLower.cpp - Lower MMIX MachineInstr to MCInst ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXMCInstLower.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static bool branchesBackward(const MachineInstr &MI) {
  const MachineBasicBlock *Source = MI.getParent();
  const MachineBasicBlock *Target =
      MI.getOperand(MI.getOpcode() == MMIX::PseudoB ? 1 : 0).getMBB();
  if (Source == Target)
    return true;

  for (const MachineBasicBlock &MBB : *Source->getParent()) {
    if (&MBB == Target)
      return true;
    if (&MBB == Source)
      return false;
  }
  llvm_unreachable("branch target is outside its MachineFunction");
}

const MCExpr *MMIXMCInstLower::lowerSymbolOperand(const MachineOperand &MO,
                                                  MCSymbol *Symbol) const {
  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Ctx);
  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  unsigned Shift;
  switch (MO.getTargetFlags()) {
  case MMIXII::MO_None:
    return Expr;
  case MMIXII::MO_ABS_LO:
    Shift = 0;
    break;
  case MMIXII::MO_ABS_ML:
    Shift = 16;
    break;
  case MMIXII::MO_ABS_MH:
    Shift = 32;
    break;
  case MMIXII::MO_ABS_HI:
    Shift = 48;
    break;
  default:
    report_fatal_error("MMIX symbol operand has unsupported target flags");
  }
  if (Shift)
    Expr =
        MCBinaryExpr::createLShr(Expr, MCConstantExpr::create(Shift, Ctx), Ctx);
  Expr =
      MCBinaryExpr::createAnd(Expr, MCConstantExpr::create(0xffff, Ctx), Ctx);
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
  unsigned Opcode = MI.getOpcode();
  unsigned PredicateOperand = ~0U;
  if (Opcode == MMIX::PseudoB) {
    static constexpr unsigned BranchOpcodes[][2] = {
        {MMIX::BN, MMIX::BNB},     {MMIX::BZ, MMIX::BZB},
        {MMIX::BP, MMIX::BPB},     {MMIX::BOD, MMIX::BODB},
        {MMIX::BNN, MMIX::BNNB},   {MMIX::BNZ, MMIX::BNZB},
        {MMIX::BNP, MMIX::BNPB},   {MMIX::BEV, MMIX::BEVB},
    };
    PredicateOperand = 2;
    int64_t Predicate = MI.getOperand(PredicateOperand).getImm();
    if (Predicate < 0 || Predicate > 7)
      report_fatal_error("invalid MMIX branch predicate");
    Opcode = BranchOpcodes[Predicate][branchesBackward(MI)];
  } else if (Opcode == MMIX::PseudoJMP) {
    Opcode = branchesBackward(MI) ? MMIX::JMPB : MMIX::JMP;
  }
  OutMI.setOpcode(Opcode);
  for (unsigned I = 0; I != MI.getNumOperands(); ++I) {
    if (I == PredicateOperand)
      continue;
    const MachineOperand &MO = MI.getOperand(I);
    MCOperand MCOp = lowerOperand(MO);
    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}
