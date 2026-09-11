//===--- MMIXLinux.cpp - MMIX Linux ToolChain -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXLinux.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

namespace {
class UnavailableTool final : public Tool {
  const char *Operation;

public:
  UnavailableTool(const ToolChain &TC, const char *Operation)
      : Tool("MMIXLinux::Unavailable", Operation, TC), Operation(Operation) {}

  bool hasIntegratedCPP() const override { return false; }
  void ConstructJob(Compilation &C, const JobAction &, const InputInfo &,
                    const InputInfoList &, const ArgList &,
                    const char *) const override {
    C.getDriver().Diag(diag::err_drv_clang_unsupported) << Operation;
  }
};
} // namespace

MMIXLinuxToolChain::MMIXLinuxToolChain(
    const Driver &D, const llvm::Triple &Triple, const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  // Linux's constructor probes GCC and installs distribution search paths.
  // Keep Linux resource discovery isolated until target-owned inputs exist.
  // FIXME: Honor explicit sysroots and allow native MMIX Linux system paths
  // without implicit host fallback when cross-compiling. Native system paths
  // must not require GCC discovery or change the LLVM provider defaults.
  getFilePaths().clear();
  getLibraryPaths().clear();
  getProgramPaths().push_back(D.Dir);

  auto Reject = [&](const Arg *A) {
    D.Diag(diag::err_drv_unsupported_opt_for_target)
        << A->getAsString(Args) << getTripleString();
  };
  auto RequireProvider = [&](OptSpecifier Option, StringRef Provider,
                             bool AllowPlatform = false) {
    if (const Arg *A = Args.getLastArg(Option)) {
      A->claim();
      StringRef Value = A->getValue();
      if (Value != Provider && !(AllowPlatform && Value == "platform"))
        Reject(A);
    }
  };
  RequireProvider(options::OPT_cstdlib_EQ, "llvm-libc");
  RequireProvider(options::OPT_rtlib_EQ, "compiler-rt", true);
  RequireProvider(options::OPT_stdlib_EQ, "libc++", true);
  RequireProvider(options::OPT_fuse_ld_EQ, "lld");
  if (const Arg *A = Args.getLastArg(options::OPT_unwindlib_EQ)) {
    A->claim();
    StringRef Value = A->getValue();
    if (Value != "none" && Value != "libunwind" && Value != "platform")
      Reject(A);
  }
  if (const Arg *A = Args.getLastArg(
          options::OPT_fPIC, options::OPT_fpic, options::OPT_fPIE,
          options::OPT_fpie, options::OPT_fno_PIC, options::OPT_fno_pic,
          options::OPT_fno_PIE, options::OPT_fno_pie)) {
    if (A->getOption().matches(options::OPT_fPIC) ||
        A->getOption().matches(options::OPT_fpic) ||
        A->getOption().matches(options::OPT_fPIE) ||
        A->getOption().matches(options::OPT_fpie))
      Reject(A);
  }
  for (const Arg *A : Args.filtered(
           options::OPT_shared, options::OPT_dynamic, options::OPT_rdynamic,
           options::OPT_pie, options::OPT_static_pie, options::OPT_pthread,
           options::OPT_gcc_toolchain, options::OPT_gcc_install_dir_EQ,
           options::OPT_gcc_triple_EQ))
    Reject(A);
}

ToolChain::UnwindLibType
MMIXLinuxToolChain::GetUnwindLibType(const ArgList &Args) const {
  const Arg *A = Args.getLastArg(options::OPT_unwindlib_EQ);
  if (A && StringRef(A->getValue()) == "libunwind")
    return UNW_CompilerRT;
  return UNW_None;
}

void MMIXLinuxToolChain::AddClangSystemIncludeArgs(
    const ArgList &Args, ArgStringList &CC1Args) const {
  if (Args.hasArg(options::OPT_nostdinc, options::OPT_nobuiltininc))
    return;
  llvm::SmallString<128> Path(getDriver().ResourceDir);
  llvm::sys::path::append(Path, "include");
  addSystemInclude(Args, CC1Args, Path);
}

Tool *MMIXLinuxToolChain::buildAssembler() const {
  return new UnavailableTool(*this, "external assembly for MMIX Linux");
}

Tool *MMIXLinuxToolChain::buildLinker() const {
  // FIXME: Enable linking after Linux resource discovery and CRT composition
  // are implemented; no host or bare-metal fallback is valid here.
  return new UnavailableTool(*this, "linking for MMIX Linux");
}
