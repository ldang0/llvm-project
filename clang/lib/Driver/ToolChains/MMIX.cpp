//===--- MMIX.cpp - MMIX ToolChain Implementations ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIX.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "llvm/Option/ArgList.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

namespace {

class UnsupportedAssembler final : public Tool {
public:
  UnsupportedAssembler(const ToolChain &TC)
      : Tool("MMIX::UnsupportedAssembler", "MMIX external assembler", TC) {}

  bool hasIntegratedCPP() const override { return false; }

  void ConstructJob(Compilation &C, const JobAction &, const InputInfo &,
                    const InputInfoList &, const ArgList &,
                    const char *) const override {
    C.getDriver().Diag(diag::err_drv_clang_unsupported)
        << "external assembly for MMIX";
  }
};

class UnsupportedLinker final : public Tool {
public:
  UnsupportedLinker(const ToolChain &TC)
      : Tool("MMIX::UnsupportedLinker", "MMIX linker", TC) {}

  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &, const InputInfo &,
                    const InputInfoList &, const ArgList &,
                    const char *) const override {
    C.getDriver().Diag(diag::err_drv_clang_unsupported) << "linking for MMIX";
  }
};

} // namespace

MMIXToolChain::MMIXToolChain(const Driver &D, const llvm::Triple &Triple,
                             const ArgList &Args)
    : ToolChain(D, Triple, Args) {}

Tool *MMIXToolChain::buildAssembler() const {
  return new UnsupportedAssembler(*this);
}

Tool *MMIXToolChain::buildLinker() const {
  return new UnsupportedLinker(*this);
}
