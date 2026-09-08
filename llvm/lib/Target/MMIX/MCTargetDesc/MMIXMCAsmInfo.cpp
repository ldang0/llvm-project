//===-- MMIXMCAsmInfo.cpp - MMIX asm properties ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXMCAsmInfo.h"
#include "MMIXBaseInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

MMIXMCAsmInfo::MMIXMCAsmInfo(const Triple &TT, const MCTargetOptions &Options)
    : MCAsmInfoELF(Options) {
  CodePointerSize = 8;
  CalleeSaveStackSlotSize = 8;
  CommentString = "#";
  Data8bitsDirective = "\t.byte\t";
  Data16bitsDirective = "\t.2byte\t";
  Data32bitsDirective = "\t.4byte\t";
  Data64bitsDirective = "\t.8byte\t";
  ZeroDirective = "\t.space\t";
  MinInstAlignment = 4;
  AllowDigitAtStartOfIdentifier = true;
  UsesELFSectionDirectiveForBSS = true;
  IsLittleEndian = false;
  SupportsDebugInformation =
      Options.OutputAsmVariant.value_or(MMIXII::CanonicalAsmVariant) !=
      MMIXII::MMIXALAsmVariant;
  // Explicit unwind tables do not yet enable C++ exception transfers.
  UsesCFIWithoutEH = SupportsDebugInformation;
}

void MMIXMCAsmInfo::printSpecifierExpr(raw_ostream &OS,
                                       const MCSpecifierExpr &Expr) const {
  switch (Expr.getSpecifier()) {
  case MMIXII::S_GETA:
    OS << "%geta(";
    printExpr(OS, *Expr.getSubExpr());
    OS << ')';
    return;
  default:
    llvm_unreachable("unknown MMIX expression specifier");
  }
}
