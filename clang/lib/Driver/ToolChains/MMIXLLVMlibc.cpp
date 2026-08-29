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
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::toolchains;

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
