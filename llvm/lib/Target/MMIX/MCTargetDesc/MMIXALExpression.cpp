//===-- MMIXALExpression.cpp - Validate and print MMIXAL expressions -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALExpression.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

enum MMIXALExprPrecedence : unsigned {
  WeakPrecedence = 1,
  StrongPrecedence,
  UnaryPrecedence,
  PrimaryPrecedence,
};

unsigned getMMIXALPrecedence(const MCExpr &Expr) {
  if (isa<MCConstantExpr, MCSymbolRefExpr>(Expr))
    return PrimaryPrecedence;
  if (isa<MCUnaryExpr>(Expr))
    return UnaryPrecedence;

  switch (cast<MCBinaryExpr>(Expr).getOpcode()) {
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
    llvm_unreachable("unsupported expression passed MMIXAL validation");
  }
}

StringRef getMMIXALBinaryOperator(MCBinaryExpr::Opcode Opcode) {
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
    llvm_unreachable("unsupported expression passed MMIXAL validation");
  }
}

char getMMIXALUnaryOperator(MCUnaryExpr::Opcode Opcode) {
  switch (Opcode) {
  case MCUnaryExpr::Minus:
    return '-';
  case MCUnaryExpr::Not:
    return '~';
  case MCUnaryExpr::Plus:
    return '+';
  case MCUnaryExpr::LNot:
    llvm_unreachable("unsupported expression passed MMIXAL validation");
  }
  llvm_unreachable("invalid MC unary expression operator");
}

void printMMIXALExpressionImpl(
    const MCExpr &Expr, const MCAsmInfo &MAI, unsigned ParentPrecedence,
    bool IsRightOperand,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver,
    raw_ostream &OS) {
  const unsigned Precedence = getMMIXALPrecedence(Expr);
  const bool Parenthesize = Precedence < ParentPrecedence ||
                            (IsRightOperand && Precedence == ParentPrecedence);
  if (Parenthesize)
    OS << '(';

  switch (Expr.getKind()) {
  case MCExpr::Constant:
    OS << static_cast<uint64_t>(cast<MCConstantExpr>(Expr).getValue());
    break;
  case MCExpr::SymbolRef:
    if (SymbolResolver)
      OS << SymbolResolver(cast<MCSymbolRefExpr>(Expr).getSymbol()).Name;
    else
      cast<MCSymbolRefExpr>(Expr).getSymbol().print(OS, &MAI);
    break;
  case MCExpr::Unary: {
    const auto &Unary = cast<MCUnaryExpr>(Expr);
    OS << getMMIXALUnaryOperator(Unary.getOpcode());
    printMMIXALExpressionImpl(*Unary.getSubExpr(), MAI, UnaryPrecedence, true,
                              SymbolResolver, OS);
    break;
  }
  case MCExpr::Binary: {
    const auto &Binary = cast<MCBinaryExpr>(Expr);
    printMMIXALExpressionImpl(*Binary.getLHS(), MAI, Precedence, false,
                              SymbolResolver, OS);
    OS << getMMIXALBinaryOperator(Binary.getOpcode());
    printMMIXALExpressionImpl(*Binary.getRHS(), MAI, Precedence, true,
                              SymbolResolver, OS);
    break;
  }
  case MCExpr::Specifier:
  case MCExpr::Target:
    llvm_unreachable("unsupported expression passed MMIXAL validation");
  }

  if (Parenthesize)
    OS << ')';
}

} // namespace

Error llvm::validateMMIXALExpression(const MCExpr &Expr) {
  switch (Expr.getKind()) {
  case MCExpr::Constant:
    return Error::success();
  case MCExpr::SymbolRef:
    if (cast<MCSymbolRefExpr>(Expr).getKind() != 0)
      return createStringError("unsupported MMIXAL symbol reference variant");
    return Error::success();
  case MCExpr::Unary: {
    const auto &Unary = cast<MCUnaryExpr>(Expr);
    if (Unary.getOpcode() == MCUnaryExpr::LNot)
      return createStringError("unsupported MMIXAL unary expression operator");
    return validateMMIXALExpression(*Unary.getSubExpr());
  }
  case MCExpr::Binary: {
    const auto &Binary = cast<MCBinaryExpr>(Expr);
    switch (Binary.getOpcode()) {
    case MCBinaryExpr::Add:
    case MCBinaryExpr::Sub:
    case MCBinaryExpr::Mul:
    case MCBinaryExpr::And:
    case MCBinaryExpr::Or:
    case MCBinaryExpr::Xor:
    case MCBinaryExpr::Shl:
    case MCBinaryExpr::LShr:
      break;
    default:
      return createStringError("unsupported MMIXAL binary expression operator");
    }
    if (Error Err = validateMMIXALExpression(*Binary.getLHS()))
      return Err;
    return validateMMIXALExpression(*Binary.getRHS());
  }
  case MCExpr::Specifier:
    return createStringError("unsupported MMIXAL relocation specifier");
  case MCExpr::Target:
    return createStringError("unsupported MMIXAL target expression");
  }
  llvm_unreachable("invalid MC expression kind");
}

const MCSymbol *llvm::getBareMMIXALSymbol(const MCExpr &Expr) {
  if (const auto *Symbol = dyn_cast<MCSymbolRefExpr>(&Expr))
    return Symbol->getKind() == 0 ? &Symbol->getSymbol() : nullptr;
  if (const auto *Unary = dyn_cast<MCUnaryExpr>(&Expr))
    if (Unary->getOpcode() == MCUnaryExpr::Plus)
      return getBareMMIXALSymbol(*Unary->getSubExpr());
  return nullptr;
}

bool llvm::referencesFutureMMIXALSymbol(
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
    return referencesFutureMMIXALSymbol(*cast<MCUnaryExpr>(Expr).getSubExpr(),
                                        SymbolResolver);
  case MCExpr::Binary: {
    const auto &Binary = cast<MCBinaryExpr>(Expr);
    return referencesFutureMMIXALSymbol(*Binary.getLHS(), SymbolResolver) ||
           referencesFutureMMIXALSymbol(*Binary.getRHS(), SymbolResolver);
  }
  case MCExpr::Specifier:
  case MCExpr::Target:
    llvm_unreachable("unsupported expression passed MMIXAL validation");
  }
  llvm_unreachable("invalid MC expression kind");
}

Error llvm::printMMIXALExpression(
    const MCExpr &Expr, const MCAsmInfo &MAI,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver,
    raw_ostream &OS) {
  if (Error Err = validateMMIXALExpression(Expr))
    return Err;
  printMMIXALExpressionImpl(Expr, MAI, 0, false, SymbolResolver, OS);
  return Error::success();
}
