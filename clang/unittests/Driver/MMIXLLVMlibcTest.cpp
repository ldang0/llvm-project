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
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "gtest/gtest.h"

using namespace clang::driver::toolchains;

namespace {

static std::string normalized(llvm::StringRef Path) {
  return llvm::sys::path::convert_to_slash(Path);
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
