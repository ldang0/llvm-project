//===--- MMIXPlatform.h - MMIX execution-platform policy -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H

#include "llvm/ADT/StringRef.h"

#include <string>

namespace clang {
namespace driver {
namespace toolchains {
namespace mmix {

enum class ExecutionPlatform { QEMU };

struct ExecutionPlatformInputs {
  std::string DefaultLinkerScript;
  std::string StartupObject;
  std::string TripVectorObject;
  std::string InitObject;
  std::string FiniObject;
  std::string ServiceLibrary;
};

ExecutionPlatform getExecutionPlatform();
ExecutionPlatformInputs getExecutionPlatformInputs(ExecutionPlatform Platform,
                                                   llvm::StringRef LibraryPath);

} // namespace mmix
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXPLATFORM_H
