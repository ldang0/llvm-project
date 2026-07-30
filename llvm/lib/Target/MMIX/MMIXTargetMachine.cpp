//===-- MMIXTargetMachine.cpp - Define TargetMachine for MMIX -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXTargetMachine.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

static CodeModel::Model
getEffectiveCodeModel(std::optional<CodeModel::Model> CM) {
  return CM.value_or(CodeModel::Small);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMMIXTarget() {
  RegisterTargetMachine<MMIXTargetMachine> X(getTheMMIXTarget());
}

MMIXTargetMachine::MMIXTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : TargetMachine(T, TT.computeDataLayout(), TT, CPU, FS, Options),
      Subtarget(TT, CPU, FS) {
  this->RM = getEffectiveRelocModel(RM);
  this->CMModel = getEffectiveCodeModel(CM);
  this->OptLevel = OL;

  MRI.reset(createMMIXMCRegisterInfo(TT));
  MII.reset(createMMIXMCInstrInfo());
  STI.reset(createMMIXMCSubtargetInfo(TT, CPU, FS));
  AsmInfo.reset(T.createMCAsmInfo(*MRI, TT, Options.MCOptions));
}
