//===- MMIXLLVMlibcTest.cpp - MMIX LLVM libc policy tests ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../../lib/Driver/ToolChains/MMIXLLVMlibc.h"
#include "SimpleDiagnosticConsumer.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/ToolChain.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "gtest/gtest.h"

using namespace clang::driver::toolchains;

namespace {

static std::string normalized(llvm::StringRef Path) {
  return llvm::sys::path::convert_to_slash(Path);
}

static clang::driver::ToolChain::CStdlibType
selectedCStdlib(llvm::ArrayRef<const char *> SelectionArgs) {
  auto FileSystem = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
  FileSystem->addFile("/input.c", 0,
                      llvm::MemoryBuffer::getMemBuffer("int value;\n"));

  clang::DiagnosticOptions DiagnosticOptions;
  clang::DiagnosticsEngine Diagnostics(clang::DiagnosticIDs::create(),
                                       DiagnosticOptions,
                                       new SimpleDiagnosticConsumer);
  clang::driver::Driver Driver("/bin/clang", "mmix-unknown-unknown",
                               Diagnostics, "clang LLVM compiler", FileSystem);
  llvm::SmallVector<const char *, 8> Args = {"-fsyntax-only", "/input.c"};
  Args.append(SelectionArgs.begin(), SelectionArgs.end());
  std::unique_ptr<clang::driver::Compilation> Compilation(
      Driver.BuildCompilation(Args));
  EXPECT_TRUE(Compilation);
  if (!Compilation)
    return clang::driver::ToolChain::CST_System;
  return Compilation->getDefaultToolChain().GetCStdlibType(
      Compilation->getArgs());
}

static mmix::ExecutionPlatformInputs
llvmLibcPlatformInputs(llvm::ArrayRef<const char *> PlatformArgs) {
  auto FileSystem = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
  FileSystem->addFile("/input.c", 0,
                      llvm::MemoryBuffer::getMemBuffer("int value;\n"));

  clang::DiagnosticOptions DiagnosticOptions;
  clang::DiagnosticsEngine Diagnostics(clang::DiagnosticIDs::create(),
                                       DiagnosticOptions,
                                       new SimpleDiagnosticConsumer);
  clang::driver::Driver Driver("/bin/clang", "mmix-unknown-unknown",
                               Diagnostics, "clang LLVM compiler", FileSystem);
  llvm::SmallVector<const char *, 8> Args = {"-fsyntax-only", "/input.c",
                                             "--cstdlib=llvm-libc"};
  Args.append(PlatformArgs.begin(), PlatformArgs.end());
  std::unique_ptr<clang::driver::Compilation> Compilation(
      Driver.BuildCompilation(Args));
  EXPECT_TRUE(Compilation);
  if (!Compilation)
    return {};
  return mmix::getLLVMlibcExecutionPlatformInputs(
      mmix::ExecutionPlatform::QEMU, Compilation->getArgs(),
      mmix::getLLVMlibcInstallation("/sysroot"));
}

TEST(MMIXLLVMlibcTest, CanonicalInstallationPaths) {
  mmix::LLVMlibcInstallation Installation =
      mmix::getLLVMlibcInstallation("/sysroot");

  EXPECT_EQ(normalized(Installation.IncludeDirectory),
            "/sysroot/include/mmix-unknown-unknown");
  EXPECT_EQ(normalized(Installation.LibraryDirectory),
            "/sysroot/lib/mmix-unknown-unknown");
  EXPECT_EQ(normalized(Installation.CRT),
            "/sysroot/lib/mmix-unknown-unknown/crt1.o");
  EXPECT_EQ(normalized(Installation.LinkerScript),
            "/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld");
  EXPECT_EQ(normalized(Installation.LibC),
            "/sysroot/lib/mmix-unknown-unknown/libc.a");
  EXPECT_EQ(normalized(Installation.LibM),
            "/sysroot/lib/mmix-unknown-unknown/libm.a");
  EXPECT_EQ(normalized(Installation.PlatformLibrary),
            "/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a");
}

TEST(MMIXLLVMlibcTest, TypedResources) {
  mmix::LLVMlibcInstallation Installation =
      mmix::getLLVMlibcInstallation("/sysroot");

  EXPECT_EQ(Installation.getResource(mmix::LLVMlibcResource::CRT),
            Installation.CRT);
  EXPECT_EQ(Installation.getResource(mmix::LLVMlibcResource::LinkerScript),
            Installation.LinkerScript);
  EXPECT_EQ(Installation.getResource(mmix::LLVMlibcResource::LibC),
            Installation.LibC);
  EXPECT_EQ(Installation.getResource(mmix::LLVMlibcResource::LibM),
            Installation.LibM);
  EXPECT_EQ(Installation.getResource(mmix::LLVMlibcResource::PlatformLibrary),
            Installation.PlatformLibrary);
}

TEST(MMIXLLVMlibcTest, QEMUInputsOwnProviderResourcePaths) {
  mmix::LLVMlibcQEMUInputs Inputs =
      mmix::getLLVMlibcQEMUInputs(mmix::getLLVMlibcInstallation("/sysroot"));

  EXPECT_EQ(normalized(Inputs.LinkerScript),
            "/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld");
  EXPECT_EQ(normalized(Inputs.StartFile),
            "/sysroot/lib/mmix-unknown-unknown/crt1.o");
  EXPECT_EQ(normalized(Inputs.PlatformLibrary),
            "/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a");
}

TEST(MMIXLLVMlibcTest, ComposesProviderSpecificPlatformInputs) {
  mmix::ExecutionPlatformInputs Inputs = llvmLibcPlatformInputs({});
  EXPECT_EQ(normalized(Inputs.DefaultLinkerScript),
            "/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld");
  ASSERT_EQ(Inputs.StartFiles.size(), 1u);
  EXPECT_EQ(normalized(Inputs.StartFiles.front()),
            "/sysroot/lib/mmix-unknown-unknown/crt1.o");
  EXPECT_TRUE(Inputs.TerminationFile.empty());
  ASSERT_EQ(Inputs.ServiceLibraries.size(), 1u);
  EXPECT_EQ(normalized(Inputs.ServiceLibraries.front()),
            "/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a");

  Inputs = llvmLibcPlatformInputs({"-T", "/custom.ld"});
  EXPECT_TRUE(Inputs.DefaultLinkerScript.empty());
  Inputs = llvmLibcPlatformInputs({"-nostartfiles"});
  EXPECT_TRUE(Inputs.StartFiles.empty());
  Inputs = llvmLibcPlatformInputs({"-nodefaultlibs"});
  EXPECT_TRUE(Inputs.ServiceLibraries.empty());
  Inputs = llvmLibcPlatformInputs({"-nostdlib", "-T", "/custom.ld"});
  EXPECT_TRUE(Inputs.DefaultLinkerScript.empty());
  EXPECT_TRUE(Inputs.StartFiles.empty());
  EXPECT_TRUE(Inputs.ServiceLibraries.empty());
}

TEST(MMIXLLVMlibcTest, SelectsExplicitLLVMlibc) {
  using ToolChain = clang::driver::ToolChain;

  EXPECT_EQ(selectedCStdlib({}), ToolChain::CST_Newlib);
  EXPECT_EQ(selectedCStdlib({"--cstdlib=newlib"}), ToolChain::CST_Newlib);
  EXPECT_EQ(selectedCStdlib({"--cstdlib=llvm-libc"}), ToolChain::CST_LLVMLibC);
  EXPECT_EQ(selectedCStdlib({"--cstdlib=picolibc", "--cstdlib=llvm-libc"}),
            ToolChain::CST_LLVMLibC);
  EXPECT_EQ(selectedCStdlib({"--cstdlib=llvm-libc", "--cstdlib=newlib"}),
            ToolChain::CST_Newlib);
}

TEST(MMIXLLVMlibcTest, ValidatesResourcesThroughVFS) {
  auto FileSystem = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
  FileSystem->addFile("/input.c", 0,
                      llvm::MemoryBuffer::getMemBuffer("int value;\n"));
  FileSystem->addFile("/sysroot/include/mmix-unknown-unknown/.keep", 0,
                      llvm::MemoryBuffer::getMemBuffer(""));
  FileSystem->addFile("/sysroot/lib/mmix-unknown-unknown/libc.a", 0,
                      llvm::MemoryBuffer::getMemBuffer(""));

  auto *DiagnosticConsumer = new SimpleDiagnosticConsumer;
  clang::DiagnosticOptions DiagnosticOptions;
  clang::DiagnosticsEngine Diagnostics(clang::DiagnosticIDs::create(),
                                       DiagnosticOptions, DiagnosticConsumer);
  clang::driver::Driver Driver("/bin/clang", "mmix-unknown-unknown",
                               Diagnostics, "clang LLVM compiler", FileSystem);
  std::unique_ptr<clang::driver::Compilation> Compilation(
      Driver.BuildCompilation({"-fsyntax-only", "/input.c"}));
  ASSERT_TRUE(Compilation);

  const clang::driver::ToolChain &ToolChain =
      Compilation->getDefaultToolChain();
  DiagnosticConsumer->clear();
  mmix::LLVMlibcInstallation Installation =
      mmix::getLLVMlibcInstallation("/sysroot");
  EXPECT_TRUE(mmix::validateLLVMlibcIncludeDirectory(ToolChain, Installation));
  EXPECT_TRUE(mmix::validateLLVMlibcResource(ToolChain, Installation,
                                             mmix::LLVMlibcResource::LibC));
  EXPECT_TRUE(DiagnosticConsumer->Errors.empty());

  EXPECT_FALSE(mmix::validateLLVMlibcResource(ToolChain, Installation,
                                              mmix::LLVMlibcResource::LibM));
  ASSERT_EQ(DiagnosticConsumer->Errors.size(), 1u);
  EXPECT_EQ(DiagnosticConsumer->Errors.front(),
            "no such file or directory: "
            "'/sysroot/lib/mmix-unknown-unknown/libm.a'");
}

} // namespace
