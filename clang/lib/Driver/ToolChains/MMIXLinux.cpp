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
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

namespace {
bool diagnoseMissing(const ToolChain &TC, StringRef Path, bool Directory) {
  auto Status = TC.getVFS().status(Path);
  if (Status && (Directory ? Status->isDirectory() : Status->isRegularFile()))
    return false;
  TC.getDriver().Diag(diag::err_drv_no_such_file) << Path;
  return true;
}

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
  // FIXME: Add native MMIX Linux system paths without implicit host fallback
  // when cross-compiling, GCC discovery, or changes to LLVM provider defaults.
  getFilePaths().clear();
  getLibraryPaths().clear();
  getProgramPaths().push_back(D.Dir);
  getLibraryPaths().push_back(getCompilerRTPath());
  if (!D.SysRoot.empty()) {
    SmallString<128> Path(D.SysRoot);
    llvm::sys::path::append(Path, "usr", "lib");
    getFilePaths().push_back(std::string(Path));
  }

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
  if (Args.hasArg(options::OPT_nostdinc))
    return;
  const Driver &D = getDriver();
  if (!Args.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> Path(D.ResourceDir);
    llvm::sys::path::append(Path, "include");
    if (!diagnoseMissing(*this, Path, /*Directory=*/true))
      addSystemInclude(Args, CC1Args, Path);
  }
  // Without a sysroot, freestanding compilation can still use builtin headers.
  if (D.SysRoot.empty() || Args.hasArg(options::OPT_nostdlibinc))
    return;
  SmallString<128> Path(D.SysRoot);
  llvm::sys::path::append(Path, "usr", "include");
  if (!diagnoseMissing(*this, Path, /*Directory=*/true))
    addExternCSystemInclude(Args, CC1Args, Path);
}

void MMIXLinuxToolChain::AddClangCXXStdlibIncludeArgs(
    const ArgList &Args, ArgStringList &CC1Args) const {
  const Driver &D = getDriver();
  if (D.SysRoot.empty() ||
      Args.hasArg(options::OPT_nostdinc, options::OPT_nostdlibinc,
                  options::OPT_nostdincxx))
    return;
  SmallString<128> Path(D.SysRoot);
  llvm::sys::path::append(Path, "usr", "include", "c++", "v1");
  SmallString<128> Config(Path);
  llvm::sys::path::append(Config, "__config_site");
  // The generated configuration belongs to this libc++ installation, not to
  // a host or bare-metal resource directory with otherwise matching headers.
  if (!diagnoseMissing(*this, Config, /*Directory=*/false))
    addSystemInclude(Args, CC1Args, Path);
}

std::string MMIXLinuxToolChain::getCompilerRTPath() const {
  SmallString<128> Path(getDriver().ResourceDir);
  llvm::sys::path::append(Path, "lib", "mmix-unknown-linux");
  return std::string(Path);
}

std::string MMIXLinuxToolChain::getCompilerRT(const ArgList &Args,
                                           StringRef Component, FileType Type,
                                           bool IsFortran) const {
  SmallString<128> Path(getCompilerRTPath());
  llvm::sys::path::append(
      Path, buildCompilerRTBasename(Args, Component, Type,
                                   /*AddArch=*/false, IsFortran));
  diagnoseMissing(*this, Path, /*Directory=*/false);
  return std::string(Path);
}

Tool *MMIXLinuxToolChain::buildAssembler() const {
  return new UnavailableTool(*this, "external assembly for MMIX Linux");
}

Tool *MMIXLinuxToolChain::buildLinker() const {
  // FIXME: Validate required inputs and compose the Linux CRT/static link;
  // generic GetFilePath queries are not a validated resource selection API.
  return new UnavailableTool(*this, "linking for MMIX Linux");
}
