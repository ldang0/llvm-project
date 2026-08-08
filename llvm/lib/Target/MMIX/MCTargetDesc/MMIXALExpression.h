//===-- MMIXALExpression.h - MMIXAL expression support --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALEXPRESSION_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALEXPRESSION_H

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

namespace llvm {

class MCAsmInfo;
class MCExpr;
class MCSymbol;
class raw_ostream;

struct MMIXALSymbolPrintInfo {
  StringRef Name;
  bool IsDefined;
};

Error validateMMIXALExpression(const MCExpr &Expr);
const MCSymbol *getBareMMIXALSymbol(const MCExpr &Expr);
bool referencesFutureMMIXALSymbol(
    const MCExpr &Expr,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver);
Error printMMIXALExpression(
    const MCExpr &Expr, const MCAsmInfo &MAI,
    function_ref<MMIXALSymbolPrintInfo(const MCSymbol &)> SymbolResolver,
    raw_ostream &OS);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALEXPRESSION_H
