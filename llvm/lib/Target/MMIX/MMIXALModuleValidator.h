//===-- MMIXALModuleValidator.h - Validate MMIXAL modules ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXALMODULEVALIDATOR_H
#define LLVM_LIB_TARGET_MMIX_MMIXALMODULEVALIDATOR_H

#include "llvm/Support/Error.h"

namespace llvm {

class Function;
class Module;
class ModulePass;

Expected<const Function *> validateMMIXALRawEntry(const Module &M);
Expected<const Function *> validateMMIXALModule(const Module &M);
ModulePass *createMMIXALModuleValidatorPass();

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXALMODULEVALIDATOR_H
