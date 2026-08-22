//===--- MMIXPlatform.h - MMIX execution-platform policy -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H

#include "clang/Driver/ToolChain.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Option/ArgList.h"

#include <optional>
#include <string>

namespace clang {
namespace driver {
namespace toolchains {
namespace mmix {

enum class ExecutionPlatform { QEMU };

struct ExecutionPlatformLinkerInputs {
  std::string DefaultLinkerScript;
  llvm::SmallVector<std::string, 3> StartFiles;
  std::string TerminationFile;
};

ExecutionPlatform getExecutionPlatform();
std::optional<ExecutionPlatformLinkerInputs> getExecutionPlatformLinkerInputs(
    ExecutionPlatform Platform, const ToolChain &TC,
    const llvm::opt::ArgList &Args, llvm::StringRef LibraryPath);
std::string getExecutionPlatformServiceLibrary(ExecutionPlatform Platform,
                                               llvm::StringRef LibraryPath);

} // namespace mmix
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H
