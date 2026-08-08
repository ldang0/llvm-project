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
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

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
