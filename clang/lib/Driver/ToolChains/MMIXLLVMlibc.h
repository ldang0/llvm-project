//===--- MMIXLLVMlibc.h - MMIX LLVM libc policy ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXLLVMLIBC_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXLLVMLIBC_H

#include "MMIXPlatform.h"
#include "clang/Driver/ToolChain.h"

#include <string>

namespace clang {
namespace driver {
namespace toolchains {
namespace mmix {

enum class LLVMlibcResource {
  CRT,
  LinkerScript,
  LibC,
  LibM,
  PlatformLibrary,
};

struct LLVMlibcInstallation {
  std::string IncludeDirectory;
  std::string LibraryDirectory;
  std::string CRT;
  std::string LinkerScript;
  std::string LibC;
  std::string LibM;
  std::string PlatformLibrary;

  llvm::StringRef getResource(LLVMlibcResource Resource) const;
};

struct LLVMlibcQEMUInputs {
  std::string LinkerScript;
  std::string StartFile;
  std::string PlatformLibrary;
};

LLVMlibcInstallation getLLVMlibcInstallation(llvm::StringRef SysRoot);
LLVMlibcQEMUInputs
getLLVMlibcQEMUInputs(const LLVMlibcInstallation &Installation);
ExecutionPlatformInputs
getLLVMlibcExecutionPlatformInputs(ExecutionPlatform Platform,
                                   const llvm::opt::ArgList &Args,
                                   const LLVMlibcInstallation &Installation);
void addLLVMlibcSystemIncludeArgs(const ToolChain &TC,
                                  const llvm::opt::ArgList &DriverArgs,
                                  llvm::opt::ArgStringList &CC1Args,
                                  const LLVMlibcInstallation &Installation);
bool validateLLVMlibcIncludeDirectory(const ToolChain &TC,
                                      const LLVMlibcInstallation &Installation);
bool validateLLVMlibcResource(const ToolChain &TC,
                              const LLVMlibcInstallation &Installation,
                              LLVMlibcResource Resource);
bool validateLLVMlibcRuntimeInputs(const ToolChain &TC,
                                   const llvm::opt::ArgList &Args,
                                   const LLVMlibcInstallation &Installation);

} // namespace mmix
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXLLVMLIBC_H
