//===--- MMIXPlatform.cpp - MMIX execution-platform policy ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXPlatform.h"
#include "MMIXQEMU.h"
#include "llvm/Support/ErrorHandling.h"

using namespace clang::driver::toolchains;

mmix::ExecutionPlatform mmix::getExecutionPlatform() {
  return ExecutionPlatform::QEMU;
}

std::optional<mmix::ExecutionPlatformInputs> mmix::getExecutionPlatformInputs(
    ExecutionPlatform Platform, const ToolChain &TC,
    const llvm::opt::ArgList &Args, llvm::StringRef LibraryPath) {
  switch (Platform) {
  case ExecutionPlatform::QEMU:
    return qemu::getInputs(TC, Args, LibraryPath);
  }
  llvm_unreachable("unhandled MMIX execution platform");
}
