//===--- MMIXLLVMlibc.cpp - MMIX LLVM libc policy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXLLVMlibc.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/ToolChain.h"
#include "clang/Options/Options.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

namespace {

static std::string appendPath(StringRef Root, ArrayRef<StringRef> Components) {
  SmallString<128> Path(Root);
  for (StringRef Component : Components)
    llvm::sys::path::append(Path, Component);
  return std::string(Path);
}

static bool diagnoseMissing(const ToolChain &TC, StringRef Path,
                            bool RequireDirectory) {
  auto Status = TC.getVFS().status(Path);
  bool IsValid = Status && (RequireDirectory ? Status->isDirectory()
                                             : Status->isRegularFile());
  if (!IsValid)
    TC.getDriver().Diag(diag::err_drv_no_such_file) << Path;
  return IsValid;
}

static bool isRegularFile(const ToolChain &TC, StringRef Path) {
  auto Status = TC.getVFS().status(Path);
  return Status && Status->isRegularFile();
}

static std::string getLibraryPath(StringRef LibraryPath, StringRef Name) {
  return appendPath(LibraryPath, {Name});
}

} // namespace

StringRef
mmix::LLVMlibcInstallation::getResource(LLVMlibcResource Resource) const {
  switch (Resource) {
  case LLVMlibcResource::CRT:
    return CRT;
  case LLVMlibcResource::LinkerScript:
    return LinkerScript;
  case LLVMlibcResource::LibC:
    return LibC;
  case LLVMlibcResource::LibM:
    return LibM;
  case LLVMlibcResource::PlatformLibrary:
    return PlatformLibrary;
  }
  llvm_unreachable("unhandled MMIX LLVM libc resource");
}

mmix::LLVMlibcInstallation mmix::getLLVMlibcInstallation(StringRef SysRoot) {
  LLVMlibcInstallation Installation;
  Installation.IncludeDirectory =
      appendPath(SysRoot, {"include", "mmix-unknown-unknown"});
  Installation.LibraryDirectory =
      appendPath(SysRoot, {"lib", "mmix-unknown-unknown"});
  Installation.CRT = appendPath(Installation.LibraryDirectory, {"crt1.o"});
  Installation.LinkerScript =
      appendPath(Installation.LibraryDirectory, {"mmix-qemu.ld"});
  Installation.LibC = appendPath(Installation.LibraryDirectory, {"libc.a"});
  Installation.LibM = appendPath(Installation.LibraryDirectory, {"libm.a"});
  Installation.PlatformLibrary =
      appendPath(Installation.LibraryDirectory, {"libmmixplatform.a"});
  return Installation;
}

mmix::LLVMlibcQEMUInputs
mmix::getLLVMlibcQEMUInputs(const LLVMlibcInstallation &Installation) {
  return {Installation.LinkerScript, Installation.CRT,
          Installation.PlatformLibrary};
}

void mmix::addLLVMlibcSystemIncludeArgs(
    const ToolChain &TC, const ArgList &DriverArgs, ArgStringList &CC1Args,
    const LLVMlibcInstallation &Installation) {
  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;
  if (!validateLLVMlibcIncludeDirectory(TC, Installation))
    return;
  ToolChain::addSystemInclude(DriverArgs, CC1Args,
                              Installation.IncludeDirectory);
}

bool mmix::validateLLVMlibcIncludeDirectory(
    const ToolChain &TC, const LLVMlibcInstallation &Installation) {
  return diagnoseMissing(TC, Installation.IncludeDirectory,
                         /*RequireDirectory=*/true);
}

bool mmix::validateLLVMlibcResource(const ToolChain &TC,
                                    const LLVMlibcInstallation &Installation,
                                    LLVMlibcResource Resource) {
  return diagnoseMissing(TC, Installation.getResource(Resource),
                         /*RequireDirectory=*/false);
}

bool mmix::validateLLVMlibcRuntimeInputs(
    const ToolChain &TC, const ArgList &Args,
    const LLVMlibcInstallation &Installation) {
  const bool NeedsLinkerScript = !Args.hasArg(options::OPT_T_Group);
  const bool NeedsCRT =
      !Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles);
  const bool NeedsDefaultLibraries =
      !Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs);
  bool NeedsLibM =
      llvm::is_contained(Args.getAllArgValues(options::OPT_l), "m");
  if (NeedsLibM) {
    for (StringRef SearchPath : Args.getAllArgValues(options::OPT_L)) {
      if (isRegularFile(TC, getLibraryPath(SearchPath, "libm.a"))) {
        NeedsLibM = false;
        break;
      }
    }
  }

  if (!NeedsLinkerScript && !NeedsCRT && !NeedsDefaultLibraries && !NeedsLibM)
    return true;
  if (!diagnoseMissing(TC, Installation.LibraryDirectory,
                       /*RequireDirectory=*/true))
    return false;

  bool Valid = true;
  if (NeedsLinkerScript)
    Valid &= validateLLVMlibcResource(TC, Installation,
                                      LLVMlibcResource::LinkerScript);
  if (NeedsCRT)
    Valid &= validateLLVMlibcResource(TC, Installation, LLVMlibcResource::CRT);
  if (NeedsDefaultLibraries) {
    Valid &= validateLLVMlibcResource(TC, Installation, LLVMlibcResource::LibC);
    Valid &= validateLLVMlibcResource(TC, Installation,
                                      LLVMlibcResource::PlatformLibrary);
  }
  if (NeedsLibM)
    Valid &= validateLLVMlibcResource(TC, Installation, LLVMlibcResource::LibM);
  return Valid;
}
