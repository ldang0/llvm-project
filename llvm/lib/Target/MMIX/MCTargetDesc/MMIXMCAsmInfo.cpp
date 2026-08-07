//===-- MMIXMCAsmInfo.cpp - MMIX asm properties ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXMCAsmInfo.h"
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
}
