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
#include "llvm/IR/GlobalIFunc.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

StringRef getMMIXALLinkageName(GlobalValue::LinkageTypes Linkage) {
  switch (Linkage) {
  case GlobalValue::ExternalLinkage:
    return "external";
  case GlobalValue::AvailableExternallyLinkage:
    return "available_externally";
  case GlobalValue::LinkOnceAnyLinkage:
    return "linkonce";
  case GlobalValue::LinkOnceODRLinkage:
    return "linkonce_odr";
  case GlobalValue::WeakAnyLinkage:
    return "weak";
  case GlobalValue::WeakODRLinkage:
    return "weak_odr";
  case GlobalValue::AppendingLinkage:
    return "appending";
  case GlobalValue::InternalLinkage:
    return "internal";
  case GlobalValue::PrivateLinkage:
    return "private";
  case GlobalValue::ExternalWeakLinkage:
    return "extern_weak";
  case GlobalValue::CommonLinkage:
    return "common";
  }
  llvm_unreachable("unknown LLVM linkage");
}

bool isMMIXALRuntimeRegistrationSection(StringRef Section) {
  for (StringRef Prefix :
       {".init_array", ".fini_array", ".preinit_array", ".ctors", ".dtors"})
    if (Section.starts_with(Prefix))
      return true;
  return false;
}

Error validateMMIXALSymbolSemantics(const GlobalValue &GV) {
  if (isa<GlobalIFunc>(GV))
    return createStringError(
        Twine("MMIXAL output variant 1 does not support GlobalIFunc '") +
        GV.getName() + "'");

  switch (GV.getLinkage()) {
  case GlobalValue::ExternalLinkage:
  case GlobalValue::InternalLinkage:
  case GlobalValue::PrivateLinkage:
    break;
  default:
    return createStringError(
        Twine("MMIXAL output variant 1 does not support linkage '") +
        getMMIXALLinkageName(GV.getLinkage()) + "' for symbol '" +
        GV.getName() + "'");
  }

  if (GV.hasComdat())
    return createStringError(
        Twine(
            "MMIXAL output variant 1 does not support COMDAT membership for ") +
        "symbol '" + GV.getName() + "'");

  if (!GV.hasDefaultVisibility()) {
    StringRef Visibility = GV.hasHiddenVisibility() ? "hidden" : "protected";
    return createStringError(
        Twine("MMIXAL output variant 1 does not support ") + Visibility +
        " visibility for symbol '" + GV.getName() + "'");
  }

  if (GV.getDLLStorageClass() != GlobalValue::DefaultStorageClass) {
    StringRef Storage =
        GV.hasDLLImportStorageClass() ? "dllimport" : "dllexport";
    return createStringError(
        Twine("MMIXAL output variant 1 does not support ") + Storage +
        " storage for symbol '" + GV.getName() + "'");
  }

  if (GV.hasPartition())
    return createStringError(
        Twine("MMIXAL output variant 1 does not support partition '") +
        GV.getPartition() + "' for symbol '" + GV.getName() + "'");

  return Error::success();
}

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

  if (const NamedMDNode *Symvers = M.getNamedMetadata("symvers");
      Symvers && Symvers->getNumOperands() != 0) {
    const MDNode *Version = Symvers->getOperand(0);
    const auto *Symbol = Version->getNumOperands() == 0
                             ? nullptr
                             : dyn_cast<MDString>(Version->getOperand(0));
    if (Symbol)
      return createStringError(
          Twine(
              "MMIXAL output variant 1 does not support ELF symbol version ") +
          "for '" + Symbol->getString() + "'");
    return createStringError(
        "MMIXAL output variant 1 does not support ELF symbol-version metadata");
  }

  for (const GlobalVariable &Global : M.globals()) {
    if (Global.getName() == "llvm.global_ctors" ||
        Global.getName() == "llvm.global_dtors")
      return createStringError(Twine("MMIXAL output variant 1 does not support "
                                     "runtime registration ") +
                               "symbol '" + Global.getName() + "'");
    if (Global.hasSection() &&
        isMMIXALRuntimeRegistrationSection(Global.getSection()))
      return createStringError(Twine("MMIXAL output variant 1 does not support "
                                     "runtime registration ") +
                               "section '" + Global.getSection() +
                               "' for symbol '" + Global.getName() + "'");
  }

  for (const GlobalValue &GV : M.global_values()) {
    if (const auto *F = dyn_cast<Function>(&GV); F && F->isIntrinsic())
      continue;
    if (Error Err = validateMMIXALSymbolSemantics(GV))
      return std::move(Err);
    if (!GV.isDeclaration() || GV.use_empty())
      continue;
    return createStringError(
        Twine("MMIXAL output variant 1 cannot resolve referenced symbol '") +
        GV.getName() + "'");
  }

  return *Entry;
}
