//===--- MMIXPlatform.cpp - MMIX execution-platform policy ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXPlatform.h"
#include "MMIXQEMU.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"

using namespace clang::driver::toolchains;

namespace {

static std::string getInputPath(llvm::StringRef LibraryPath,
                                llvm::StringRef Name) {
  llvm::SmallString<128> Path(LibraryPath);
  llvm::sys::path::append(Path, Name);
  return std::string(Path);
}

} // namespace

mmix::ExecutionPlatform mmix::getExecutionPlatform() {
  return ExecutionPlatform::QEMU;
}

std::optional<mmix::ExecutionPlatformLinkerInputs>
mmix::getExecutionPlatformLinkerInputs(ExecutionPlatform Platform,
                                       const ToolChain &TC,
                                       const llvm::opt::ArgList &Args,
                                       llvm::StringRef LibraryPath) {
  switch (Platform) {
  case ExecutionPlatform::QEMU:
    return qemu::getLinkerInputs(TC, Args, LibraryPath);
  }
  llvm_unreachable("unhandled MMIX execution platform");
}

std::string
mmix::getExecutionPlatformServiceLibrary(ExecutionPlatform Platform,
                                         llvm::StringRef LibraryPath) {
  switch (Platform) {
  case ExecutionPlatform::QEMU:
    return getInputPath(LibraryPath, "libgloss.a");
  }
  llvm_unreachable("unhandled MMIX execution platform");
}
