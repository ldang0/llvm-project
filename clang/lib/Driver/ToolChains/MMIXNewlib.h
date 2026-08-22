//===--- MMIXNewlib.h - MMIX newlib policy ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXNEWLIB_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXNEWLIB_H

#include "clang/Driver/ToolChain.h"

#include <optional>
#include <string>

namespace clang {
namespace driver {
namespace toolchains {
namespace mmix {

void addNewlibSystemIncludeArgs(const ToolChain &TC,
                                const llvm::opt::ArgList &DriverArgs,
                                llvm::opt::ArgStringList &CC1Args);
std::optional<std::string> getNewlibLibCPath(const ToolChain &TC,
                                             StringRef LibraryPath);
bool validateNewlibExplicitLibraries(const ToolChain &TC,
                                     const llvm::opt::ArgList &Args,
                                     StringRef LibraryPath);

} // namespace mmix
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXNEWLIB_H
