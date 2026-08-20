//===--- MMIX.cpp - MMIX ToolChain Implementations ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIX.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Job.h"
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

static bool diagnoseUnsupportedLinkMode(Compilation &C, const ArgList &Args) {
  const Driver &D = C.getDriver();
  auto Diagnose = [&](StringRef Mode) {
    D.Diag(diag::err_drv_clang_unsupported) << Mode;
    return true;
  };

  if (Args.hasArg(options::OPT_r))
    return Diagnose("relocatable linking for MMIX");
  if (Args.hasArg(options::OPT_shared))
    return Diagnose("shared linking for MMIX");
  if (Args.hasArg(options::OPT_dynamic, options::OPT_rdynamic))
    return Diagnose("dynamic linking for MMIX");
  if (Args.hasArg(options::OPT_pie, options::OPT_static_pie))
    return Diagnose("PIE linking for MMIX");
  if (const Arg *A = Args.getLastArg(
          options::OPT_fPIC, options::OPT_fno_PIC, options::OPT_fpic,
          options::OPT_fno_pic, options::OPT_fPIE, options::OPT_fno_PIE,
          options::OPT_fpie, options::OPT_fno_pie)) {
    if (A->getOption().matches(options::OPT_fPIC) ||
        A->getOption().matches(options::OPT_fpic) ||
        A->getOption().matches(options::OPT_fPIE) ||
        A->getOption().matches(options::OPT_fpie))
      return Diagnose("position-independent linking for MMIX");
  }
  if (Args.hasArg(options::OPT_flto, options::OPT_flto_EQ))
    return Diagnose("LTO linking for MMIX");
  if (!D.SysRoot.empty())
    return Diagnose("sysroot selection for MMIX freestanding linking");
  if (Args.hasArg(options::OPT_rtlib_EQ, options::OPT_unwindlib_EQ))
    return Diagnose("runtime library selection for MMIX freestanding linking");
  if (Args.hasArg(options::OPT_ld_path_EQ))
    return Diagnose("custom linker selection for MMIX");
  if (const Arg *A = Args.getLastArg(options::OPT_fuse_ld_EQ)) {
    if (StringRef(A->getValue()) != "lld")
      return Diagnose("non-lld linker selection for MMIX");
  }
  if (!Args.hasArg(options::OPT_ffreestanding))
    return Diagnose("implicit hosted linking for MMIX");
  if (!Args.hasArg(options::OPT_nostdlib) ||
      !Args.hasArg(options::OPT_nostartfiles) ||
      !Args.hasArg(options::OPT_nodefaultlibs))
    return Diagnose("implicit runtime files for MMIX freestanding linking");

  return false;
}

class StaticLinker final : public Tool {
public:
  StaticLinker(const ToolChain &TC)
      : Tool("MMIX::StaticLinker", "MMIX static linker", TC) {}

  bool hasIntegratedCPP() const override { return false; }
  bool isLinkJob() const override { return true; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const ArgList &Args, const char *) const override {
    if (diagnoseUnsupportedLinkMode(C, Args))
      return;

    const ToolChain &TC = getToolChain();
    ArgStringList CmdArgs;
    CmdArgs.push_back("-m");
    CmdArgs.push_back("elf64mmix");
    CmdArgs.push_back("-static");
    Args.addAllArgs(CmdArgs, {options::OPT_L, options::OPT_T_Group,
                              options::OPT_s, options::OPT_t,
                              options::OPT_u_Group});
    tools::AddLinkerInputs(TC, Inputs, Args, CmdArgs, JA);
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());

    const char *Exec = Args.MakeArgString(TC.GetProgramPath("ld.lld"));
    C.addCommand(std::make_unique<Command>(
        JA, *this, ResponseFileSupport::AtFileCurCP(), Exec, CmdArgs, Inputs,
        Output));
  }
};

} // namespace

MMIXToolChain::MMIXToolChain(const Driver &D, const llvm::Triple &Triple,
                             const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  getProgramPaths().push_back(getDriver().Dir);
}

ToolChain::RuntimeLibType
MMIXToolChain::GetRuntimeLibType(const ArgList &Args) const {
  if (const Arg *A = Args.getLastArg(options::OPT_rtlib_EQ)) {
    if (StringRef(A->getValue()) != "compiler-rt")
      getDriver().Diag(diag::err_drv_clang_unsupported)
          << "non-compiler-rt runtime selection for MMIX";
  }
  return ToolChain::RLT_CompilerRT;
}

std::string
MMIXToolChain::ComputeEffectiveClangTriple(const ArgList &Args, BoundArch BA,
                                           types::ID InputType) const {
  StringRef RequestedTriple = getDriver().getTargetTriple();
  if (const Arg *A = Args.getLastArg(options::OPT_target))
    RequestedTriple = A->getValue();
  if (RequestedTriple == "mmix-unknown-elf")
    return "mmix-unknown-unknown";
  return ComputeLLVMTriple(Args, BA, InputType);
}

std::string MMIXToolChain::getCompilerRTPath() const {
  SmallString<128> Path(getDriver().ResourceDir);
  llvm::sys::path::append(Path, "lib", "mmix-unknown-unknown");
  return std::string(Path);
}

std::string MMIXToolChain::getCompilerRT(const ArgList &Args,
                                         StringRef Component, FileType Type,
                                         bool IsFortran) const {
  if (Type != ToolChain::FT_Static || IsFortran)
    return ToolChain::getCompilerRT(Args, Component, Type, IsFortran);

  SmallString<128> Path(getCompilerRTPath());
  llvm::sys::path::append(
      Path, buildCompilerRTBasename(Args, Component, Type,
                                    /*AddArch=*/false, IsFortran));
  if (!getVFS().exists(Path))
    getDriver().Diag(diag::err_drv_no_such_file) << Path;
  return std::string(Path);
}

void MMIXToolChain::addClangTargetOptions(const ArgList &DriverArgs,
                                          ArgStringList &, BoundArch,
                                          Action::OffloadKind) const {
  if (const Arg *A = DriverArgs.getLastArg(
          options::OPT_mstack_protector_guard_symbol_EQ))
    getDriver().Diag(diag::err_drv_unsupported_opt_for_target)
        << A->getAsString(DriverArgs) << getTripleString();
}

Tool *MMIXToolChain::buildAssembler() const {
  return new UnsupportedAssembler(*this);
}

Tool *MMIXToolChain::buildLinker() const {
  return new StaticLinker(*this);
}
