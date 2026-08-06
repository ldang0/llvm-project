//===-- MMIXSubtarget.cpp - MMIX Subtarget Information --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXSubtarget.h"

using namespace llvm;

#define DEBUG_TYPE "mmix-subtarget"

#define GET_SUBTARGETINFO_ENUM
#include "MMIXGenSubtargetInfo.inc"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "MMIXGenSubtargetInfo.inc"

void MMIXSubtarget::anchor() {}

MMIXSubtarget &MMIXSubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                              StringRef FS) {
  StringRef CPUName = CPU.empty() ? "generic" : CPU;
  ParseSubtargetFeatures(CPUName, CPUName, FS);
  return *this;
}

MMIXSubtarget::MMIXSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                             const TargetMachine &TM)
    : MMIXGenSubtargetInfo(TT, CPU.empty() ? "generic" : CPU,
                           /*TuneCPU=*/CPU.empty() ? "generic" : CPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), FrameLowering(),
      TLInfo(TM, *this) {}
