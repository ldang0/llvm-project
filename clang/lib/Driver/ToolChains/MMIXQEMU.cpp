//===--- MMIXQEMU.cpp - MMIX QEMU platform policy -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXQEMU.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/ToolChain.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

namespace {

static std::string getInputPath(StringRef LibraryPath, StringRef Name) {
  SmallString<128> Path(LibraryPath);
  llvm::sys::path::append(Path, Name);
  return std::string(Path);
}

static bool requireFile(const ToolChain &TC, StringRef Path) {
  auto Status = TC.getVFS().status(Path);
  if (Status && Status->isRegularFile())
    return true;
  TC.getDriver().Diag(diag::err_drv_no_such_file) << Path;
  return false;
}

} // namespace

std::optional<mmix::ExecutionPlatformInputs>
mmix::qemu::getInputs(const ToolChain &TC, const ArgList &Args,
                      StringRef LibraryPath) {
  ExecutionPlatformInputs Inputs;
  bool InputsValid = true;

  if (!Args.hasArg(options::OPT_T_Group)) {
    Inputs.DefaultLinkerScript = getInputPath(LibraryPath, "mmix-qemu.ld");
    InputsValid &= requireFile(TC, Inputs.DefaultLinkerScript);
  }

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
    std::string CRT0 = getInputPath(LibraryPath, "crt0.o");
    std::string TripVectors = getInputPath(LibraryPath, "trip-vectors.o");
    InputsValid &= requireFile(TC, CRT0);
    InputsValid &= requireFile(TC, TripVectors);
    Inputs.StartFiles.push_back(std::move(CRT0));
    Inputs.StartFiles.push_back(std::move(TripVectors));

    std::string CRTI = getInputPath(LibraryPath, "crti.o");
    if (TC.getVFS().exists(CRTI))
      Inputs.StartFiles.push_back(std::move(CRTI));

    std::string CRTN = getInputPath(LibraryPath, "crtn.o");
    if (TC.getVFS().exists(CRTN))
      Inputs.TerminationFile = std::move(CRTN);
  }

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    std::string LibGloss = getInputPath(LibraryPath, "libgloss.a");
    InputsValid &= requireFile(TC, LibGloss);
    Inputs.ServiceLibraries.push_back(std::move(LibGloss));
  }

  if (!InputsValid)
    return std::nullopt;
  return Inputs;
}
