//===--- MMIXPlatform.cpp - MMIX execution-platform policy ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXPlatform.h"
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

mmix::ExecutionPlatformInputs
mmix::getExecutionPlatformInputs(ExecutionPlatform Platform,
                                 llvm::StringRef LibraryPath) {
  switch (Platform) {
  case ExecutionPlatform::QEMU:
    return {
        getInputPath(LibraryPath, "mmix-qemu.ld"),
        getInputPath(LibraryPath, "crt0.o"),
        getInputPath(LibraryPath, "trip-vectors.o"),
        getInputPath(LibraryPath, "crti.o"),
        getInputPath(LibraryPath, "crtn.o"),
        getInputPath(LibraryPath, "libgloss.a"),
    };
  }
  llvm_unreachable("unhandled MMIX execution platform");
}
