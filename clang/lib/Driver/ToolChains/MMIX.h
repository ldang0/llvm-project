//===--- MMIX.h - MMIX ToolChain Implementations -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIX_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIX_H

#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY MMIXToolChain final : public ToolChain {
public:
  MMIXToolChain(const Driver &D, const llvm::Triple &Triple,
                const llvm::opt::ArgList &Args);

  bool isPICDefault() const override { return false; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return false;
  }
  bool isPICDefaultForced() const override { return false; }
  bool SupportsProfiling() const override { return false; }
  bool HasNativeLLVMSupport() const override { return true; }
  llvm::ExceptionHandling
  GetExceptionModel(const llvm::opt::ArgList &Args) const override;
  const char *getDefaultLinker() const override { return "ld.lld"; }
  RuntimeLibType GetDefaultRuntimeLibType() const override {
    return ToolChain::RLT_CompilerRT;
  }
  RuntimeLibType
  GetRuntimeLibType(const llvm::opt::ArgList &Args) const override;
  CStdlibType GetCStdlibType(const llvm::opt::ArgList &Args) const override;
  CXXStdlibType GetDefaultCXXStdlibType() const override { return CST_Libcxx; }
  CXXStdlibType GetCXXStdlibType(const llvm::opt::ArgList &) const override {
    return CST_Libcxx;
  }
  std::string ComputeEffectiveClangTriple(
      const llvm::opt::ArgList &Args, BoundArch BA = {},
      types::ID InputType = types::TY_INVALID) const override;
  std::string getCompilerRTPath() const override;
  std::string getCompilerRT(const llvm::opt::ArgList &Args,
                            llvm::StringRef Component,
                            FileType Type = ToolChain::FT_Static,
                            bool IsFortran = false) const override;
  void
  AddClangCXXStdlibIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                             llvm::opt::ArgStringList &CC1Args) const override;
  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;
  void addClangTargetOptions(const llvm::opt::ArgList &DriverArgs,
                             llvm::opt::ArgStringList &CC1Args, BoundArch BA,
                             Action::OffloadKind DeviceOffloadKind) const override;

protected:
  Tool *buildAssembler() const override;
  Tool *buildLinker() const override;

private:
  mutable bool DiagnosedUnsupportedCStdlib = false;
};

} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIX_H
