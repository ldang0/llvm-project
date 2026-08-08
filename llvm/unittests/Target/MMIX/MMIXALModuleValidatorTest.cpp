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
#include <utility>
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

  const Function *expectModule(StringRef IR) {
    Module *M = parseModule(IR);
    if (!M)
      return nullptr;
    Expected<const Function *> Entry = validateMMIXALModule(*M);
    if (!Entry) {
      ADD_FAILURE() << toString(Entry.takeError());
      return nullptr;
    }
    return *Entry;
  }

  void expectModuleError(StringRef IR, StringRef ExpectedDiagnostic) {
    Module *M = parseModule(IR);
    if (!M)
      return;
    Expected<const Function *> Entry = validateMMIXALModule(*M);
    if (Entry) {
      ADD_FAILURE() << "module validation unexpectedly succeeded";
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

TEST_F(MMIXALModuleValidatorTest, AcceptsModulesWithoutOpaqueAssembly) {
  const Function *Entry = expectModule(R"(
    module asm ""

    define void @Main() {
      br label %loop
    loop:
      br label %loop
    }

    define void @ordinary() {
      ret void
    }
  )");
  ASSERT_NE(Entry, nullptr);
  EXPECT_EQ(Entry->getName(), "Main");
}

TEST_F(MMIXALModuleValidatorTest, RejectsModuleInlineAssembly) {
  expectModuleError(
      R"(
    module asm "SWYM 0, 0, 0"

    define void @Main() {
      br label %loop
    loop:
      br label %loop
    }
  )",
      "MMIXAL output variant 1 does not support module-level inline assembly");
}

TEST_F(MMIXALModuleValidatorTest, RejectsFunctionInlineAssembly) {
  for (StringRef Body : {
           R"(call void asm "", ""())",
           R"(call void asm sideeffect "SWYM 0, 0, 0", ""())",
           R"(%value = call i64 asm "OR $0, $1, 0", "=r,r"(i64 0))",
       }) {
    SCOPED_TRACE(Body);
    std::string IR = (Twine(R"(
      define void @Main() {
        br label %loop
      loop:
        br label %loop
      }

      define void @opaque() {
    )") + Body + R"(
        ret void
      }
    )")
                         .str();
    expectModuleError(
        IR, "MMIXAL output variant 1 does not support inline assembly in "
            "function 'opaque'");
  }

  expectModuleError(
      R"(
    define void @Main() {
      br label %loop
    loop:
      br label %loop
    }

    define void @opaque_goto() {
    entry:
      callbr void asm sideeffect "", "!i"()
          to label %fallthrough [label %target]
    fallthrough:
      ret void
    target:
      ret void
    }
  )",
      "MMIXAL output variant 1 does not support inline assembly in function "
      "'opaque_goto'");
}

TEST_F(MMIXALModuleValidatorTest, AcceptsClosedModuleDefinitions) {
  const Function *Entry = expectModule(R"(
    @external_data = global i64 1
    @internal_data = internal global i64 2
    @private_data = private global i64 3
    @data_alias = alias i64, ptr @external_data

    declare void @unused()
    declare i64 @llvm.ctpop.i64(i64)

    define void @Main() {
      %value = call i64 @llvm.ctpop.i64(i64 7)
      store volatile i64 %value, ptr @data_alias
      br label %loop
    loop:
      br label %loop
    }

    define internal void @internal_function() {
      ret void
    }

    define private void @private_function() {
      ret void
    }

    define void @external_function() {
      ret void
    }

    @function_alias = alias void (), ptr @external_function
  )");
  ASSERT_NE(Entry, nullptr);
  EXPECT_EQ(Entry->getName(), "Main");
}

TEST_F(MMIXALModuleValidatorTest, RejectsReferencedDeclarations) {
  for (auto [Reference, Symbol] : {
           std::pair{R"(
             declare void @external_function()
             define void @owner() {
               call void @external_function()
               ret void
             }
           )",
                     "external_function"},
           std::pair{R"(
             @external_data = external global i64
             @address = global ptr @external_data
           )",
                     "external_data"},
       }) {
    SCOPED_TRACE(Symbol);
    std::string IR = (Twine(R"(
      define void @Main() {
        br label %loop
      loop:
        br label %loop
      }
    )") + Reference)
                         .str();
    expectModuleError(
        IR, (Twine("MMIXAL output variant 1 cannot resolve referenced symbol '") +
             Symbol + "'")
                .str());
  }
}

} // namespace
