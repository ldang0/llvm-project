//===--- MMIXPlatform.h - MMIX execution-platform policy -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H

#include "llvm/ADT/SmallVector.h"

#include <string>

namespace clang {
namespace driver {
namespace toolchains {
namespace mmix {

enum class ExecutionPlatform { QEMU };

struct ExecutionPlatformInputs {
  std::string DefaultLinkerScript;
  llvm::SmallVector<std::string, 3> StartFiles;
  std::string TerminationFile;
  llvm::SmallVector<std::string, 1> ServiceLibraries;
};

ExecutionPlatform getExecutionPlatform();

} // namespace mmix
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H
