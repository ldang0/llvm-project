//===--- MMIXNewlib.cpp - MMIX newlib policy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXNewlib.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "clang/Driver/Driver.h"
#include "clang/Options/Options.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

namespace {

static bool isRegularFile(const ToolChain &TC, StringRef Path) {
  auto Status = TC.getVFS().status(Path);
  return Status && Status->isRegularFile();
}

static std::string getLibraryPath(StringRef LibraryPath, StringRef Name) {
  SmallString<128> Path(LibraryPath);
  llvm::sys::path::append(Path, Name);
  return std::string(Path);
}

} // namespace

void mmix::addNewlibSystemIncludeArgs(const ToolChain &TC,
                                      const ArgList &DriverArgs,
                                      ArgStringList &CC1Args) {
  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

  const Driver &D = TC.getDriver();
  SmallString<128> IncludePath(D.SysRoot);
  llvm::sys::path::append(IncludePath, "usr", "include");
  auto Status = TC.getVFS().status(IncludePath);
  if (!Status || !Status->isDirectory()) {
    D.Diag(diag::err_drv_no_such_file) << IncludePath;
    return;
  }
  ToolChain::addSystemInclude(DriverArgs, CC1Args, IncludePath);
}

std::optional<std::string> mmix::getNewlibLibCPath(const ToolChain &TC,
                                                   StringRef LibraryPath) {
  std::string Path = getLibraryPath(LibraryPath, "libc.a");
  if (isRegularFile(TC, Path))
    return Path;
  TC.getDriver().Diag(diag::err_drv_no_such_file) << Path;
  return std::nullopt;
}

bool mmix::validateNewlibExplicitLibraries(const ToolChain &TC,
                                           const ArgList &Args,
                                           StringRef LibraryPath) {
  if (!llvm::is_contained(Args.getAllArgValues(options::OPT_l), "m"))
    return true;

  for (StringRef SearchPath : Args.getAllArgValues(options::OPT_L)) {
    if (isRegularFile(TC, getLibraryPath(SearchPath, "libm.a")))
      return true;
  }

  std::string Path = getLibraryPath(LibraryPath, "libm.a");
  if (isRegularFile(TC, Path))
    return true;
  TC.getDriver().Diag(diag::err_drv_no_such_file) << Path;
  return false;
}
