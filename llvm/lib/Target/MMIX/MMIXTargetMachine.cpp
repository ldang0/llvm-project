//===-- MMIXTargetMachine.cpp - Define TargetMachine for MMIX -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXTargetMachine.h"
#include "MMIX.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMMIXTarget() {
  RegisterTargetMachine<MMIXTargetMachine> X(getTheMMIXTarget());
  initializeMMIXDAGToDAGISelLegacyPass(*PassRegistry::getPassRegistry());
}

MMIXTargetMachine::MMIXTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(
          T, TT.computeDataLayout(), TT, CPU, FS, Options,
          getEffectiveRelocModel(RM),
          llvm::getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();
}

MMIXTargetMachine::~MMIXTargetMachine() = default;

namespace {

class MMIXPassConfig final : public TargetPassConfig {
public:
  MMIXPassConfig(MMIXTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  MMIXTargetMachine &getMMIXTargetMachine() const {
    return getTM<MMIXTargetMachine>();
  }

  bool addInstSelector() override {
    addPass(createMMIXISelDag(getMMIXTargetMachine()));
    return false;
  }
};

} // namespace

TargetPassConfig *MMIXTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MMIXPassConfig(*this, PM);
}
