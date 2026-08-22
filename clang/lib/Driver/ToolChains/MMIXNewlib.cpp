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
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace llvm::opt;

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
