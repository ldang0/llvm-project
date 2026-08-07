//===-- MMIXALInstPrinter.cpp - Print MMIXAL assembly syntax -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALInstPrinter.h"
#include "MMIXBaseInfo.h"
#include "MMIXMCTargetDesc.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include <iterator>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "MMIXGenMMIXALAsmWriter.inc"

void MMIXALInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                  StringRef Annot, const MCSubtargetInfo &STI,
                                  raw_ostream &O) {
  const MCInstrDesc &Desc = MII.get(MI->getOpcode());
  const MMIXII::MMIXALSelectionKind Selection =
      MMIXII::getMMIXALSelection(Desc.TSFlags);
  if (Selection == MMIXII::MMIXALSelectionRegister ||
      Selection == MMIXII::MMIXALSelectionImmediate) {
    const unsigned SelectableOp = Desc.getNumOperands() - 1;
    if (SelectableOp >= MI->getNumOperands())
      report_fatal_error("missing MMIXAL selectable operand");

    const MCOperand &Operand = MI->getOperand(SelectableOp);
    if (Selection == MMIXII::MMIXALSelectionRegister && !Operand.isReg())
      report_fatal_error(
          "MMIXAL register-form instruction requires a register operand");
    if (Selection == MMIXII::MMIXALSelectionImmediate && !Operand.isImm())
      report_fatal_error(
          "MMIXAL immediate-form instruction requires a byte operand");
  }

  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void MMIXALInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O) {
  if (OpNo >= MI->getNumOperands())
    report_fatal_error("invalid MMIXAL operand index");

  const MCInstrDesc &Desc = MII.get(MI->getOpcode());
  if (OpNo >= Desc.getNumOperands())
    report_fatal_error("invalid MMIXAL instruction operand index");

  const MCOperand &Op = MI->getOperand(OpNo);
  const MCOperandInfo &OpInfo = Desc.operands()[OpNo];

  auto PrintUnsigned = [&](uint64_t Max, StringRef Kind) {
    if (!Op.isImm() || Op.getImm() < 0 ||
        static_cast<uint64_t>(Op.getImm()) > Max)
      report_fatal_error(Twine("invalid MMIXAL ") + Kind);
    O << static_cast<uint64_t>(Op.getImm());
  };

  auto PrintGeneralRegister = [&] {
    if (!Op.isReg())
      report_fatal_error("invalid MMIXAL general register operand");
    if (!MRI.getRegClass(MMIX::GPR64RegClassID).contains(Op.getReg()))
      report_fatal_error("invalid MMIXAL general register operand");
    const unsigned Encoding = MRI.getEncodingValue(Op.getReg());
    if (Encoding > 255)
      report_fatal_error("invalid MMIXAL general register encoding");
    O << '$' << Encoding;
  };

  switch (OpInfo.OperandType) {
  case MMIXII::OPERAND_UIMM8:
    PrintUnsigned(0xff, "byte operand");
    return;
  case MMIXII::OPERAND_UIMM16:
    PrintUnsigned(0xffff, "wyde operand");
    return;
  case MMIXII::OPERAND_ROUNDING_MODE: {
    static constexpr const char *Names[] = {
        "ROUND_CURRENT", "ROUND_OFF", "ROUND_UP", "ROUND_DOWN", "ROUND_NEAR"};
    if (!Op.isImm() || Op.getImm() < 0 ||
        static_cast<uint64_t>(Op.getImm()) >= std::size(Names))
      report_fatal_error("invalid MMIXAL rounding mode");
    O << Names[Op.getImm()];
    return;
  }
  case MMIXII::OPERAND_RESUME_MODE:
    PrintUnsigned(1, "resume mode");
    return;
  case MMIXII::OPERAND_SYNC_MODE:
    PrintUnsigned(7, "synchronization mode");
    return;
  case MMIXII::OPERAND_REG_OR_IMM8:
    if (Op.isReg())
      PrintGeneralRegister();
    else
      PrintUnsigned(0xff, "register-or-byte operand");
    return;
  default:
    break;
  }

  if (Op.isReg()) {
    if (OpInfo.RegClass == MMIX::GPR64RegClassID ||
        OpInfo.RegClass == MMIX::FPR64RegClassID) {
      PrintGeneralRegister();
      return;
    }

    if (OpInfo.RegClass == MMIX::SPR64RegClassID) {
      if (!MRI.getRegClass(MMIX::SPR64RegClassID).contains(Op.getReg()))
        report_fatal_error("invalid MMIXAL special register operand");
      static constexpr const char *Names[] = {
          "rB", "rD", "rE", "rH",  "rJ", "rM", "rR",  "rBB", "rC",  "rN", "rO",
          "rS", "rI", "rT", "rTT", "rK", "rQ", "rU",  "rV",  "rG",  "rL", "rA",
          "rF", "rP", "rW", "rX",  "rY", "rZ", "rWW", "rXX", "rYY", "rZZ"};
      const unsigned Encoding = MRI.getEncodingValue(Op.getReg());
      if (Encoding >= std::size(Names))
        report_fatal_error("invalid MMIXAL special register encoding");
      O << Names[Encoding];
      return;
    }

    report_fatal_error("invalid MMIXAL register operand");
  }

  if (Op.isImm()) {
    O << formatImm(Op.getImm());
  } else if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
  } else {
    report_fatal_error("invalid MMIXAL operand");
  }
}

void MMIXALInstPrinter::printOperand(const MCInst *MI, uint64_t Address,
                                     unsigned OpNo, raw_ostream &O) {
  const MCInstrDesc &Desc = MII.get(MI->getOpcode());
  const MMIXII::MMIXALSelectionKind Selection =
      MMIXII::getMMIXALSelection(Desc.TSFlags);
  if (Selection == MMIXII::MMIXALSelectionForward ||
      Selection == MMIXII::MMIXALSelectionBackward) {
    if (OpNo >= MI->getNumOperands())
      report_fatal_error("invalid MMIXAL relative target index");

    const MCOperand &Op = MI->getOperand(OpNo);
    if (!Op.isExpr())
      report_fatal_error("MMIXAL relative target requires an expression");

    int64_t AbsoluteTarget;
    if (Op.getExpr()->evaluateAsAbsolute(AbsoluteTarget)) {
      const uint64_t Target = static_cast<uint64_t>(AbsoluteTarget);
      if ((Target & 3) != 0)
        report_fatal_error("MMIXAL relative target is not four-byte aligned");

      const bool IsBackward = Selection == MMIXII::MMIXALSelectionBackward;
      if ((!IsBackward && Target < Address) ||
          (IsBackward && Target >= Address))
        report_fatal_error(
            "MMIXAL relative target direction does not match instruction");

      const uint64_t Distance =
          IsBackward ? Address - Target : Target - Address;
      if ((Distance & 3) != 0)
        report_fatal_error("MMIXAL relative target is not four-byte aligned");

      const unsigned Width = MMIXII::getPCRelativeWidth(Desc.TSFlags);
      if (Width != 16 && Width != 24)
        report_fatal_error("invalid MMIXAL relative target width");
      const uint64_t MaxWords =
          IsBackward ? uint64_t(1) << Width : (uint64_t(1) << Width) - 1;
      if (Distance / 4 > MaxWords)
        report_fatal_error("MMIXAL relative target is out of range");
    }

    MAI.printExpr(O, *Op.getExpr());
    return;
  }

  printOperand(MI, OpNo, O);
}
