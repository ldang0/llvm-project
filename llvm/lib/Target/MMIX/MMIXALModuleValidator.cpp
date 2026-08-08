//===-- MMIXALModuleValidator.cpp - Validate MMIXAL modules --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALModuleValidator.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class MMIXALModuleValidatorLegacy final : public ModulePass {
public:
  static char ID;

  MMIXALModuleValidatorLegacy() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    Expected<const Function *> Entry = validateMMIXALModule(M);
    if (!Entry) {
      std::string Message = toString(Entry.takeError());
      reportFatalUsageError(StringRef(Message));
    }
    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }
};

} // namespace

char MMIXALModuleValidatorLegacy::ID = 0;

ModulePass *llvm::createMMIXALModuleValidatorPass() {
  return new MMIXALModuleValidatorLegacy();
}

Expected<const Function *> llvm::validateMMIXALRawEntry(const Module &M) {
  const GlobalValue *Main = M.getNamedValue("Main");
  if (!Main)
    return createStringError(
        "MMIXAL bare-metal module has no entry named 'Main'");

  const auto *Entry = dyn_cast<Function>(Main);
  if (!Entry)
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must be a function definition");
  if (Entry->isDeclaration())
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must be a definition");
  if (!Entry->isStrongDefinitionForLinker())
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must be a strong definition");
  if (Entry->getCallingConv() != CallingConv::C)
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must use the C calling convention");

  const FunctionType *EntryType = Entry->getFunctionType();
  if (EntryType->isVarArg())
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must not be variadic");
  if (!EntryType->getReturnType()->isVoidTy() || EntryType->getNumParams() != 0)
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must have type 'void ()'");

  for (const BasicBlock *BB : depth_first(&Entry->getEntryBlock()))
    if (isa<ReturnInst>(BB->getTerminator()))
      return createStringError(
          "MMIXAL bare-metal entry 'Main' must not have a reachable return");

  return Entry;
}

Expected<const Function *> llvm::validateMMIXALModule(const Module &M) {
  Expected<const Function *> Entry = validateMMIXALRawEntry(M);
  if (!Entry)
    return Entry.takeError();

  for (const Module::GlobalAsmFragment &Fragment : M.getModuleInlineAsm())
    if (!StringRef(Fragment.Asm).trim().empty())
      return createStringError(
          "MMIXAL output variant 1 does not support module-level inline "
          "assembly");

  for (const Function &F : M)
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB)
        if (const auto *Call = dyn_cast<CallBase>(&I);
            Call && Call->isInlineAsm())
          return createStringError(
              Twine("MMIXAL output variant 1 does not support inline assembly "
                    "in function '") +
              F.getName() + "'");

  for (const GlobalValue &GV : M.global_values()) {
    if (!GV.isDeclaration() || GV.use_empty())
      continue;
    if (const auto *F = dyn_cast<Function>(&GV); F && F->isIntrinsic())
      continue;
    return createStringError(
        Twine("MMIXAL output variant 1 cannot resolve referenced symbol '") +
        GV.getName() + "'");
  }

  return *Entry;
}
