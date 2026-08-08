//===- MMIXALModuleValidatorTest.cpp - MMIXAL module validation tests ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALModuleValidator.h"
#include "llvm/ADT/Twine.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SourceMgr.h"
#include "gtest/gtest.h"
#include <memory>
#include <string>
#include <vector>

using namespace llvm;

namespace {

class MMIXALModuleValidatorTest : public testing::Test {
  LLVMContext Context;
  std::vector<std::unique_ptr<Module>> Modules;

protected:
  Module *parseModule(StringRef IR) {
    SMDiagnostic Diagnostic;
    std::unique_ptr<Module> M = parseAssemblyString(IR, Diagnostic, Context);
    EXPECT_TRUE(M) << Diagnostic.getMessage().str();
    if (!M)
      return nullptr;
    Modules.push_back(std::move(M));
    return Modules.back().get();
  }

  const Function *expectEntry(StringRef IR) {
    Module *M = parseModule(IR);
    if (!M)
      return nullptr;
    Expected<const Function *> Entry = validateMMIXALRawEntry(*M);
    if (!Entry) {
      ADD_FAILURE() << toString(Entry.takeError());
      return nullptr;
    }
    return *Entry;
  }

  void expectError(StringRef IR, StringRef ExpectedDiagnostic) {
    Module *M = parseModule(IR);
    if (!M)
      return;
    Expected<const Function *> Entry = validateMMIXALRawEntry(*M);
    if (Entry) {
      ADD_FAILURE() << "entry validation unexpectedly succeeded";
      return;
    }
    EXPECT_EQ(toString(Entry.takeError()), ExpectedDiagnostic);
  }
};

TEST_F(MMIXALModuleValidatorTest, AcceptsStrongLoopingEntry) {
  const Function *Entry = expectEntry(R"(
    define void @Main() {
    entry:
      br label %loop
    loop:
      br label %loop
    dead:
      ret void
    }
  )");
  ASSERT_NE(Entry, nullptr);
  EXPECT_EQ(Entry->getName(), "Main");
}

TEST_F(MMIXALModuleValidatorTest, AcceptsLocalStrongEntries) {
  for (StringRef Linkage : {"internal", "private"}) {
    SCOPED_TRACE(Linkage);
    std::string IR = (Twine("define ") + Linkage + R"( void @Main() {
      unreachable
    })")
                         .str();
    EXPECT_NE(expectEntry(IR), nullptr);
  }
}

TEST_F(MMIXALModuleValidatorTest, RejectsMissingAndNonfunctionEntries) {
  expectError("define void @other() { unreachable }",
              "MMIXAL bare-metal module has no entry named 'Main'");

  for (StringRef IR : {"@Main = global i8 0", "@Main = common global i8 0",
                       R"(
             @Main = alias void (), ptr @target
             define void @target() { unreachable }
           )"}) {
    SCOPED_TRACE(IR);
    expectError(IR,
                "MMIXAL bare-metal entry 'Main' must be a function definition");
  }
}

TEST_F(MMIXALModuleValidatorTest, RejectsDeclarationsAndNonStrongDefinitions) {
  expectError("declare void @Main()",
              "MMIXAL bare-metal entry 'Main' must be a definition");

  for (StringRef Linkage : {"weak", "linkonce_odr", "available_externally"}) {
    SCOPED_TRACE(Linkage);
    std::string IR = (Twine("define ") + Linkage + R"( void @Main() {
      unreachable
    })")
                         .str();
    expectError(IR,
                "MMIXAL bare-metal entry 'Main' must be a strong definition");
  }
}

TEST_F(MMIXALModuleValidatorTest, RejectsInvalidSignatureAndCallingConvention) {
  expectError(R"(
    define i64 @Main() {
      ret i64 0
    }
  )",
              "MMIXAL bare-metal entry 'Main' must have type 'void ()'");
  expectError(R"(
    define void @Main(i64 %argument) {
      unreachable
    }
  )",
              "MMIXAL bare-metal entry 'Main' must have type 'void ()'");
  expectError(R"(
    define void @Main(...) {
      unreachable
    }
  )",
              "MMIXAL bare-metal entry 'Main' must not be variadic");
  expectError(
      R"(
    define fastcc void @Main() {
      unreachable
    }
  )",
      "MMIXAL bare-metal entry 'Main' must use the C calling convention");
}

TEST_F(MMIXALModuleValidatorTest, RejectsReachableReturnRegardlessOfAttribute) {
  for (StringRef Attribute : {"", "noreturn"}) {
    SCOPED_TRACE(Attribute);
    std::string IR = (Twine("define void @Main() ") + Attribute + R"( {
      ret void
    })")
                         .str();
    expectError(
        IR, "MMIXAL bare-metal entry 'Main' must not have a reachable return");
  }
}

} // namespace
