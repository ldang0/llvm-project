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
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <iterator>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "MMIXGenMMIXALAsmWriter.inc"

namespace {

enum MMIXALExprPrecedence : unsigned {
  WeakPrecedence = 1,
  StrongPrecedence,
  UnaryPrecedence,
  PrimaryPrecedence,
};

static unsigned getMMIXALPrecedence(const MCExpr &Expr) {
  if (isa<MCConstantExpr, MCSymbolRefExpr>(Expr))
    return PrimaryPrecedence;
  if (isa<MCUnaryExpr>(Expr))
    return UnaryPrecedence;

  const auto *Binary = dyn_cast<MCBinaryExpr>(&Expr);
  if (!Binary)
    report_fatal_error("unsupported MMIXAL expression kind");

  switch (Binary->getOpcode()) {
  case MCBinaryExpr::Add:
  case MCBinaryExpr::Sub:
  case MCBinaryExpr::Or:
  case MCBinaryExpr::Xor:
    return WeakPrecedence;
  case MCBinaryExpr::And:
  case MCBinaryExpr::LShr:
  case MCBinaryExpr::Mul:
  case MCBinaryExpr::Shl:
    return StrongPrecedence;
  default:
    report_fatal_error("unsupported MMIXAL binary expression operator");
  }
}

static StringRef getMMIXALBinaryOperator(MCBinaryExpr::Opcode Opcode) {
  switch (Opcode) {
  case MCBinaryExpr::Add:
    return "+";
  case MCBinaryExpr::Sub:
    return "-";
  case MCBinaryExpr::Mul:
    return "*";
  case MCBinaryExpr::And:
    return "&";
  case MCBinaryExpr::Or:
    return "|";
  case MCBinaryExpr::Xor:
    return "^";
  case MCBinaryExpr::Shl:
    return "<<";
  case MCBinaryExpr::LShr:
    return ">>";
  default:
    report_fatal_error("unsupported MMIXAL binary expression operator");
  }
}

static char getMMIXALUnaryOperator(MCUnaryExpr::Opcode Opcode) {
  switch (Opcode) {
  case MCUnaryExpr::Minus:
    return '-';
  case MCUnaryExpr::Not:
    return '~';
  case MCUnaryExpr::Plus:
    return '+';
  case MCUnaryExpr::LNot:
    report_fatal_error("unsupported MMIXAL unary expression operator");
  }
  llvm_unreachable("invalid MC unary expression operator");
}

static void validateMMIXALExpression(const MCExpr &Expr) {
  switch (Expr.getKind()) {
  case MCExpr::Constant:
    return;
  case MCExpr::SymbolRef:
    if (cast<MCSymbolRefExpr>(Expr).getKind() != 0)
      report_fatal_error("unsupported MMIXAL symbol reference variant");
    return;
  case MCExpr::Unary: {
    const auto &Unary = cast<MCUnaryExpr>(Expr);
    (void)getMMIXALUnaryOperator(Unary.getOpcode());
    validateMMIXALExpression(*Unary.getSubExpr());
    return;
  }
  case MCExpr::Binary: {
    const auto &Binary = cast<MCBinaryExpr>(Expr);
    (void)getMMIXALBinaryOperator(Binary.getOpcode());
    validateMMIXALExpression(*Binary.getLHS());
    validateMMIXALExpression(*Binary.getRHS());
    return;
  }
  case MCExpr::Specifier:
    report_fatal_error("unsupported MMIXAL relocation specifier");
  case MCExpr::Target:
    report_fatal_error("unsupported MMIXAL target expression");
  }
  llvm_unreachable("invalid MC expression kind");
}

static bool referencesFutureSymbol(
    const MCExpr &Expr,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver) {
  switch (Expr.getKind()) {
  case MCExpr::Constant:
    return false;
  case MCExpr::SymbolRef:
    return SymbolResolver
               ? !SymbolResolver(cast<MCSymbolRefExpr>(Expr).getSymbol())
                      .IsDefined
               : cast<MCSymbolRefExpr>(Expr).getSymbol().isUndefined();
  case MCExpr::Unary:
    return referencesFutureSymbol(*cast<MCUnaryExpr>(Expr).getSubExpr(),
                                  SymbolResolver);
  case MCExpr::Binary: {
    const auto &Binary = cast<MCBinaryExpr>(Expr);
    return referencesFutureSymbol(*Binary.getLHS(), SymbolResolver) ||
           referencesFutureSymbol(*Binary.getRHS(), SymbolResolver);
  }
  case MCExpr::Specifier:
  case MCExpr::Target:
    llvm_unreachable("unsupported expressions must be validated first");
  }
  llvm_unreachable("invalid MC expression kind");
}

static bool isBareFutureSymbol(const MCExpr &Expr) {
  if (const auto *Symbol = dyn_cast<MCSymbolRefExpr>(&Expr))
    return Symbol->getSymbol().isUndefined();
  if (const auto *Unary = dyn_cast<MCUnaryExpr>(&Expr))
    return Unary->getOpcode() == MCUnaryExpr::Plus &&
           isBareFutureSymbol(*Unary->getSubExpr());
  return false;
}

static void printMMIXALExpressionImpl(
    const MCExpr &Expr, const MCAsmInfo &MAI, unsigned ParentPrecedence,
    bool IsRightOperand,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver,
    raw_ostream &O) {
  const unsigned Precedence = getMMIXALPrecedence(Expr);
  const bool Parenthesize = Precedence < ParentPrecedence ||
                            (IsRightOperand && Precedence == ParentPrecedence);
  if (Parenthesize)
    O << '(';

  switch (Expr.getKind()) {
  case MCExpr::Constant:
    O << static_cast<uint64_t>(cast<MCConstantExpr>(Expr).getValue());
    break;
  case MCExpr::SymbolRef:
    if (SymbolResolver)
      O << SymbolResolver(cast<MCSymbolRefExpr>(Expr).getSymbol()).Name;
    else
      cast<MCSymbolRefExpr>(Expr).getSymbol().print(O, &MAI);
    break;
  case MCExpr::Unary: {
    const auto &Unary = cast<MCUnaryExpr>(Expr);
    O << getMMIXALUnaryOperator(Unary.getOpcode());
    printMMIXALExpressionImpl(*Unary.getSubExpr(), MAI, UnaryPrecedence, true,
                              SymbolResolver, O);
    break;
  }
  case MCExpr::Binary: {
    const auto &Binary = cast<MCBinaryExpr>(Expr);
    printMMIXALExpressionImpl(*Binary.getLHS(), MAI, Precedence, false,
                              SymbolResolver, O);
    O << getMMIXALBinaryOperator(Binary.getOpcode());
    printMMIXALExpressionImpl(*Binary.getRHS(), MAI, Precedence, true,
                              SymbolResolver, O);
    break;
  }
  case MCExpr::Specifier:
  case MCExpr::Target:
    llvm_unreachable("unsupported expressions must be validated first");
  }

  if (Parenthesize)
    O << ')';
}

static void printMMIXALExpression(
    const MCExpr &Expr, const MCAsmInfo &MAI, bool AllowBareFutureSymbol,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver,
    raw_ostream &O) {
  validateMMIXALExpression(Expr);
  if (referencesFutureSymbol(Expr, SymbolResolver) &&
      (!AllowBareFutureSymbol || !isBareFutureSymbol(Expr)))
    report_fatal_error("unsupported MMIXAL future instruction expression");
  printMMIXALExpressionImpl(Expr, MAI, 0, false, SymbolResolver, O);
}

} // namespace

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

void MMIXALInstPrinter::printInstWithSymbolNames(
    const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> Resolver,
    raw_ostream &O) {
  assert(!SymbolResolver && "nested MMIXAL symbol resolver");
  SymbolResolver = Resolver;
  printInst(MI, Address, "", STI, O);
  SymbolResolver = nullptr;
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
    if (Op.isExpr()) {
      int64_t Value;
      if (Op.getExpr()->evaluateAsAbsolute(Value) &&
          (Value < 0 || static_cast<uint64_t>(Value) > Max))
        report_fatal_error(Twine("MMIXAL expression is not representable as ") +
                           Kind);
      printMMIXALExpression(*Op.getExpr(), MAI, false, SymbolResolver, O);
      return;
    }
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
    printMMIXALExpression(*Op.getExpr(), MAI, false, SymbolResolver, O);
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
    const bool IsBackward = Selection == MMIXII::MMIXALSelectionBackward;
    const unsigned Width = MMIXII::getPCRelativeWidth(Desc.TSFlags);
    if (Width != 16 && Width != 24)
      report_fatal_error("invalid MMIXAL relative target width");

    if (Op.isImm()) {
      const int64_t Displacement = Op.getImm();
      const int64_t MinWords = -(int64_t(1) << Width);
      const int64_t MaxWords = (int64_t(1) << Width) - 1;
      if ((!IsBackward && (Displacement < 0 || Displacement > MaxWords)) ||
          (IsBackward && (Displacement < MinWords || Displacement >= 0)))
        report_fatal_error(
            "MMIXAL decoded relative target does not match instruction");

      // MC disassembly does not provide an instruction address here. Preserve
      // the decoded word displacement in the instruction-listing form.
      O << Displacement;
      return;
    }

    if (!Op.isExpr())
      report_fatal_error("MMIXAL relative target requires an expression");

    int64_t AbsoluteTarget;
    if (Op.getExpr()->evaluateAsAbsolute(AbsoluteTarget)) {
      const uint64_t Target = static_cast<uint64_t>(AbsoluteTarget);
      if ((Target & 3) != 0)
        report_fatal_error("MMIXAL relative target is not four-byte aligned");

      if ((!IsBackward && Target < Address) ||
          (IsBackward && Target >= Address))
        report_fatal_error(
            "MMIXAL relative target direction does not match instruction");

      const uint64_t Distance =
          IsBackward ? Address - Target : Target - Address;
      if ((Distance & 3) != 0)
        report_fatal_error("MMIXAL relative target is not four-byte aligned");

      const uint64_t MaxWords =
          IsBackward ? uint64_t(1) << Width : (uint64_t(1) << Width) - 1;
      if (Distance / 4 > MaxWords)
        report_fatal_error("MMIXAL relative target is out of range");
    }

    printMMIXALExpression(*Op.getExpr(), MAI, true, SymbolResolver, O);
    return;
  }

  printOperand(MI, OpNo, O);
}

const MCInstrDesc &
MMIXALInstPrinter::getInstructionDesc(unsigned Opcode) const {
  return MII.get(Opcode);
}
