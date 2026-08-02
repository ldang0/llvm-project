//===-- MMIXMCTargetDesc.cpp - MMIX target descriptions -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXMCTargetDesc.h"
#include "MMIXMCAsmInfo.h"
#include "MMIXInstPrinter.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include <array>
#include <cstdint>

using namespace llvm;

#define GET_SUBTARGETINFO_MC_DESC
#include "MMIXGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MMIXGenRegisterInfo.inc"

#define GET_INSTRINFO_MC_DESC
#include "MMIXGenInstrInfo.inc"

MCInstrInfo *llvm::createMMIXMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitMMIXMCInstrInfo(X);
  return X;
}

MCRegisterInfo *llvm::createMMIXMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitMMIXMCRegisterInfo(X, 0);
  return X;
}

MCSubtargetInfo *llvm::createMMIXMCSubtargetInfo(const Triple &TT,
                                                 StringRef CPU, StringRef FS) {
  const StringRef CPUName = CPU.empty() ? "generic" : CPU;
  return createMMIXMCSubtargetInfoImpl(TT, CPUName, CPUName, FS);
}

static MCInstPrinter *createMMIXMCInstPrinter(
    const Triple &, unsigned SyntaxVariant, const MCAsmInfo &MAI,
    const MCInstrInfo &MII, const MCRegisterInfo &MRI) {
  if (SyntaxVariant != 0)
    return nullptr;
  return new MMIXInstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMMIXTargetMC() {
  Target &T = getTheMMIXTarget();
  RegisterMCAsmInfo<MMIXMCAsmInfo> X(T);
  TargetRegistry::RegisterMCInstrInfo(T, createMMIXMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createMMIXMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createMMIXMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createMMIXMCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createMMIXMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createMMIXAsmBackend);
}
