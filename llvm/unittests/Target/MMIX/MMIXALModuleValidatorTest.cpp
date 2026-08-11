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

TEST_F(MMIXALModuleValidatorTest, KeepsCallerCopyAggregateABIIsolated) {
  EXPECT_NE(expectModule(R"(
    %pair = type { i64, i64 }

    define void @Main() {
      br label %loop
    loop:
      br label %loop
    }

    define void @owner(ptr %source) {
      call void @target(ptr byval(%pair) align 8 %source)
      ret void
    }

    define void @target(ptr byval(%pair) align 8 %value) {
      ret void
    }
  )"),
            nullptr);
}

TEST_F(MMIXALModuleValidatorTest, RejectsUnreviewedAggregateABIForms) {
  expectModuleError(R"(
    %small = type { i32 }
    define void @Main() { unreachable }
    define void @direct_argument(%small %value) { ret void }
  )",
                    "MMIXAL output variant 1 does not support direct aggregate "
                    "arguments in function 'direct_argument'");

  expectModuleError(R"(
    %small = type { i32 }
    define void @Main() { unreachable }
    define %small @direct_result() { ret %small zeroinitializer }
  )",
                    "MMIXAL output variant 1 does not support direct aggregate "
                    "results in function 'direct_result'");

  expectModuleError(R"(
    %large = type { i64, i64 }
    define void @Main() { unreachable }
    define void @indirect_result(ptr sret(%large) align 8 %out) { ret void }
  )",
                    "MMIXAL output variant 1 does not support indirect "
                    "aggregate results in function 'indirect_result'");

  expectModuleError(R"(
    %small = type { i32 }
    define void @Main() { unreachable }
    define void @indirect_argument_call(ptr %callee) {
      call void %callee(%small zeroinitializer)
      ret void
    }
  )",
                    "MMIXAL output variant 1 does not support direct aggregate "
                    "call arguments in function 'indirect_argument_call'");

  expectModuleError(R"(
    %large = type { i64, i64 }
    define void @Main() { unreachable }
    define void @indirect_sret_call(ptr %callee, ptr %out) {
      call void %callee(ptr sret(%large) align 8 %out)
      ret void
    }
  )",
                    "MMIXAL output variant 1 does not support indirect "
                    "aggregate call results in function 'indirect_sret_call'");
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
        IR,
        (Twine("MMIXAL output variant 1 cannot resolve referenced symbol '") +
         Symbol + "'")
            .str());
  }
}

TEST_F(MMIXALModuleValidatorTest, AcceptsExactSymbolIdentity) {
  EXPECT_NE(expectModule(R"(
    @data = dso_local unnamed_addr global i64 0

    define void @Main() {
      br label %loop
    loop:
      br label %loop
    }

    define dso_local void @function() unnamed_addr {
      ret void
    }
  )"),
            nullptr);
}

TEST_F(MMIXALModuleValidatorTest, RejectsLinkerSelectedLinkages) {
  for (auto [Definition, Linkage] : {
           std::pair{R"(define available_externally void @selected() {
                          ret void
                        })",
                     "available_externally"},
           std::pair{R"(define linkonce void @selected() { ret void })",
                     "linkonce"},
           std::pair{R"(define linkonce_odr void @selected() { ret void })",
                     "linkonce_odr"},
           std::pair{R"(define weak void @selected() { ret void })", "weak"},
           std::pair{R"(define weak_odr void @selected() { ret void })",
                     "weak_odr"},
           std::pair{R"(@selected = appending global [1 x i64] [i64 0])",
                     "appending"},
           std::pair{R"(declare extern_weak void @selected())", "extern_weak"},
           std::pair{R"(@selected = common global i64 0)", "common"},
       }) {
    SCOPED_TRACE(Linkage);
    std::string IR = (Twine(R"(
      define void @Main() {
        br label %loop
      loop:
        br label %loop
      }
    )") + Definition)
                         .str();
    expectModuleError(
        IR, (Twine("MMIXAL output variant 1 does not support linkage '") +
             Linkage + "' for symbol 'selected'")
                .str());
  }
}

TEST_F(MMIXALModuleValidatorTest, RejectsAliasAndIFuncSelection) {
  expectModuleError(
      R"(
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
        @target = global i64 0
        @selected = weak alias i64, ptr @target
      )",
      "MMIXAL output variant 1 does not support linkage 'weak' for symbol "
      "'selected'");

  expectModuleError(
      R"(
        @selected = ifunc void (), ptr @resolver
        define ptr @resolver() { ret ptr @implementation }
        define void @implementation() { ret void }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support GlobalIFunc 'selected'");
}

TEST_F(MMIXALModuleValidatorTest, RejectsComdatAndObjectIdentity) {
  expectModuleError(
      R"(
        $group = comdat any
        define void @selected() comdat($group) { ret void }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support COMDAT membership for symbol "
      "'selected'");

  for (auto [Definition, Diagnostic] : {
           std::pair{R"(@selected = hidden global i64 0)", "hidden visibility"},
           std::pair{R"(@selected = protected global i64 0)",
                     "protected visibility"},
           std::pair{R"(@selected = external dllimport global i64)",
                     "dllimport storage"},
           std::pair{R"(@selected = dllexport global i64 0)",
                     "dllexport storage"},
       }) {
    SCOPED_TRACE(Diagnostic);
    std::string IR = (Twine(R"(
      define void @Main() {
        br label %loop
      loop:
        br label %loop
      }
    )") + Definition)
                         .str();
    expectModuleError(IR, (Twine("MMIXAL output variant 1 does not support ") +
                           Diagnostic + " for symbol 'selected'")
                              .str());
  }

  expectModuleError(
      R"(
        @selected = global i64 0, partition "partition_name"
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support partition 'partition_name' "
      "for symbol 'selected'");
}

TEST_F(MMIXALModuleValidatorTest, RejectsRuntimeRegistration) {
  expectModuleError(
      R"(
        @llvm.global_ctors = appending global [1 x { i32, ptr, ptr }]
          [{ i32, ptr, ptr } { i32 65535, ptr @constructor, ptr null }]
        define void @constructor() { ret void }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support runtime registration symbol "
      "'llvm.global_ctors'");

  expectModuleError(
      R"(
        @registration = global ptr @initializer, section ".init_array.100"
        define void @initializer() { ret void }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support runtime registration section "
      "'.init_array.100' for symbol 'registration'");

  expectModuleError(
      R"(
        !symvers = !{!0}
        !0 = !{!"versioned", !"versioned@VERSION_1"}
        define void @versioned() { ret void }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support ELF symbol version for "
      "'versioned'");
}

TEST_F(MMIXALModuleValidatorTest, AcceptsAddressSpaceZeroStorageAndPointers) {
  EXPECT_NE(expectModule(R"(
    @data = global ptr null
    @alias = alias ptr, ptr @data

    define void @Main() {
    entry:
      %slot = alloca ptr
      store ptr @data, ptr %slot
      %value = load ptr, ptr %slot
      br label %loop
    loop:
      br label %loop
    }

    define ptr @identity(ptr %value) {
      ret ptr %value
    }
  )"),
            nullptr);
}

TEST_F(MMIXALModuleValidatorTest, RejectsNonzeroGlobalAddressSpaces) {
  for (auto [Definition, Symbol] : {
           std::pair{R"(@nonzero_global = addrspace(1) global i64 0)",
                     "nonzero_global"},
           std::pair{R"(@pointer_value = global ptr addrspace(1) null)",
                     "pointer_value"},
           std::pair{
               R"(@aggregate = global { ptr addrspace(1) } zeroinitializer)",
               "aggregate"},
           std::pair{R"(
             @target = global i64 0
             @alias = alias i64, ptr addrspace(1) addrspacecast
                 (ptr @target to ptr addrspace(1))
           )",
                     "alias"},
       }) {
    SCOPED_TRACE(Symbol);
    std::string IR = (Twine(R"(
      define void @Main() {
        br label %loop
      loop:
        br label %loop
      }
    )") + Definition)
                         .str();
    expectModuleError(
        IR,
        (Twine("MMIXAL output variant 1 does not support nonzero address ") +
         "space in symbol '" + Symbol + "'")
            .str());
  }
}

TEST_F(MMIXALModuleValidatorTest, RejectsNonzeroFunctionAddressSpaces) {
  expectModuleError(
      R"(
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
        define ptr addrspace(1) @pointer_result() {
          ret ptr addrspace(1) null
        }
      )",
      "MMIXAL output variant 1 does not support nonzero address space in "
      "symbol 'pointer_result'");

  expectModuleError(
      R"(
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
        define void @pointer_argument(ptr addrspace(1) %value) {
          ret void
        }
      )",
      "MMIXAL output variant 1 does not support nonzero address space in "
      "symbol 'pointer_argument'");

  expectModuleError(
      R"(
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
        define void @nonzero_function() addrspace(1) {
          ret void
        }
      )",
      "MMIXAL output variant 1 does not support nonzero address space in "
      "symbol 'nonzero_function'");
}

TEST_F(MMIXALModuleValidatorTest, RejectsNonzeroInstructionAddressSpaces) {
  for (auto [Instruction, Opcode] : {
           std::pair{R"(%value = inttoptr i64 1 to ptr addrspace(1))",
                     "inttoptr"},
           std::pair{R"(%value = addrspacecast ptr null to ptr addrspace(1))",
                     "addrspacecast"},
           std::pair{R"(%slot = alloca ptr addrspace(1))", "alloca"},
           std::pair{R"(%value = load i64, ptr addrspace(1) null)", "load"},
           std::pair{R"(
             call void @llvm.memcpy.p1.p0.i64(
                 ptr addrspace(1) null, ptr null, i64 0, i1 false)
           )",
                     "call"},
       }) {
    SCOPED_TRACE(Opcode);
    std::string IR = (Twine(R"(
      define void @Main() {
        br label %loop
      loop:
        br label %loop
      }
      declare void @llvm.memcpy.p1.p0.i64(
          ptr addrspace(1), ptr, i64, i1 immarg)
      define void @owner() {
    )") + Instruction +
                      R"(
        ret void
      }
    )")
                         .str();
    expectModuleError(
        IR,
        (Twine("MMIXAL output variant 1 does not support nonzero address ") +
         "space in instruction '" + Opcode + "' in function 'owner'")
            .str());
  }
}

TEST_F(MMIXALModuleValidatorTest, RejectsTLSState) {
  expectModuleError(
      R"(
        @tls = thread_local(localexec) global i64 0
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support thread-local symbol 'tls'");

  expectModuleError(
      R"(
        declare ptr @llvm.thread.pointer()
        define void @Main() {
        entry:
          %pointer = call ptr @llvm.thread.pointer()
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support TLS intrinsic "
      "'llvm.thread.pointer' in function 'Main'");

  expectModuleError(
      R"(
        declare ptr @llvm.threadlocal.address.p0(ptr)
        define void @Main() {
        entry:
          %pointer = call ptr @llvm.threadlocal.address.p0(ptr null)
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support TLS intrinsic "
      "'llvm.threadlocal.address' in function 'Main'");
}

TEST_F(MMIXALModuleValidatorTest, AcceptsAddressNeutralDebugInformation) {
  EXPECT_NE(expectModule(R"(
    define void @Main() !dbg !4 {
    entry:
      br label %loop, !dbg !7
    loop:
      br label %loop, !dbg !8
    }

    !llvm.dbg.cu = !{!0}
    !llvm.module.flags = !{!2}
    !llvm.ident = !{!3}
    !0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1,
        producer: "compiler", isOptimized: false, runtimeVersion: 0,
        emissionKind: FullDebug)
    !1 = !DIFile(filename: "input.c", directory: "/source")
    !2 = !{i32 2, !"Debug Info Version", i32 3}
    !3 = !{!"compiler identification"}
    !4 = distinct !DISubprogram(name: "Main", scope: !1, file: !1, line: 1,
        type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
    !5 = !DISubroutineType(types: !6)
    !6 = !{null}
    !7 = !DILocation(line: 2, column: 1, scope: !4)
    !8 = !DILocation(line: 3, column: 1, scope: !4)
  )"),
            nullptr);
}

TEST_F(MMIXALModuleValidatorTest, RejectsExceptionAndUnwindState) {
  expectModuleError(
      R"(
        declare i32 @personality(...)
        define void @exceptional() personality ptr @personality {
          ret void
        }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support an exception personality in "
      "function 'exceptional'");

  expectModuleError(
      R"(
        define void @unwindable() uwtable {
          ret void
        }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support unwind-table generation in "
      "function 'unwindable'");
}

TEST_F(MMIXALModuleValidatorTest, RejectsRuntimeFunctionState) {
  expectModuleError(
      R"(
        define void @managed() gc "statepoint-example" {
          ret void
        }
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support garbage-collection strategy "
      "'statepoint-example' in function 'managed'");

  for (StringRef Attribute : {
           "sanitize_address",
           "sanitize_thread",
           "sanitize_type",
           "sanitize_memory",
           "sanitize_hwaddress",
           "sanitize_memtag",
           "sanitize_numerical_stability",
           "sanitize_realtime",
           "sanitize_realtime_blocking",
           "sanitize_alloc_token",
       }) {
    SCOPED_TRACE(Attribute);
    std::string IR = (Twine("define void @instrumented() ") + Attribute + R"( {
           ret void
         }
         define void @Main() {
           br label %loop
         loop:
           br label %loop
         }
       )")
                         .str();
    expectModuleError(
        IR, (Twine("MMIXAL output variant 1 does not support runtime ") +
             "instrumentation attribute '" + Attribute +
             "' in function 'instrumented'")
                .str());
  }

  expectModuleError(
      R"(
        @tagged = global i64 0, sanitize_memtag
        define void @Main() {
          br label %loop
        loop:
          br label %loop
        }
      )",
      "MMIXAL output variant 1 does not support sanitizer allocation metadata "
      "for symbol 'tagged'");
}

TEST_F(MMIXALModuleValidatorTest, RejectsRuntimeMetadataIntrinsics) {
  for (
      auto [Declaration, Call, Name] : {
          std::tuple{
              R"(declare void @llvm.experimental.stackmap(i64, i32, ...))",
              R"(call void (i64, i32, ...) @llvm.experimental.stackmap(i64 1, i32 0))",
              "llvm.experimental.stackmap"},
          std::tuple{
              R"(declare void @llvm.experimental.patchpoint.void(i64, i32, ptr, i32, ...))",
              R"(call void (i64, i32, ptr, i32, ...) @llvm.experimental.patchpoint.void(i64 2, i32 4, ptr null, i32 0))",
              "llvm.experimental.patchpoint.void"},
          std::tuple{
              R"(declare void @llvm.instrprof.increment(ptr, i64, i32, i32))",
              R"(call void @llvm.instrprof.increment(ptr null, i64 3, i32 1, i32 0))",
              "llvm.instrprof.increment"},
          std::tuple{
              R"(declare void @llvm.pseudoprobe(i64, i64, i32, i64))",
              R"(call void @llvm.pseudoprobe(i64 4, i64 1, i32 0, i64 -1))",
              "llvm.pseudoprobe"},
      }) {
    SCOPED_TRACE(Name);
    std::string IR = (Twine(Declaration) + R"(
      define void @Main() {
      entry:
    )" + Call + R"(
        br label %loop
      loop:
        br label %loop
      }
    )")
                         .str();
    expectModuleError(
        IR,
        (Twine("MMIXAL output variant 1 does not support runtime metadata ") +
         "intrinsic '" + Name + "' in function 'Main'")
            .str());
  }
}

} // namespace
