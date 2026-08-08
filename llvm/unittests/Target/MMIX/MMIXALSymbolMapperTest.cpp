//===- MMIXALSymbolMapperTest.cpp - MMIXAL symbol mapper tests -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALSymbolMapper.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"
#include <array>
#include <string>

using namespace llvm;

namespace {

void expectSuccess(Error Result) {
  if (Result)
    ADD_FAILURE() << toString(std::move(Result));
}

std::string lookup(const MMIXALSymbolMapper &Mapper, StringRef Name) {
  Expected<StringRef> Result = Mapper.getMappedUserSymbol(Name);
  if (!Result) {
    ADD_FAILURE() << toString(Result.takeError());
    return {};
  }
  return Result->str();
}

TEST(MMIXALSymbolMapperTest, PreservesValidNonreservedUserNames) {
  MMIXALSymbolMapper Mapper;
  for (StringRef Name :
       {"Main", "_start", "global_data", "ADD", "LOC", "__llvm_user", "ra"})
    expectSuccess(Mapper.registerUserSymbol(Name));
  expectSuccess(Mapper.finalize());

  EXPECT_EQ(lookup(Mapper, "Main"), "Main");
  EXPECT_EQ(lookup(Mapper, "_start"), "_start");
  EXPECT_EQ(lookup(Mapper, "global_data"), "global_data");
  EXPECT_EQ(lookup(Mapper, "ADD"), "ADD");
  EXPECT_EQ(lookup(Mapper, "LOC"), "LOC");
  EXPECT_EQ(lookup(Mapper, "__llvm_user"), "__llvm_user");
  EXPECT_EQ(lookup(Mapper, "ra"), "ra");
}

TEST(MMIXALSymbolMapperTest, EscapesInvalidNamesByRawByte) {
  const std::string EmbeddedNul("a\0b", 3);
  const std::string UTF8("\xc3\xa9", 2);
  const std::array<StringRef, 5> Names = {StringRef(), "9lives", "a.b",
                                          EmbeddedNul, UTF8};
  MMIXALSymbolMapper Mapper;
  for (StringRef Name : Names)
    expectSuccess(Mapper.registerUserSymbol(Name));
  expectSuccess(Mapper.finalize());

  EXPECT_EQ(lookup(Mapper, StringRef()), "__LLVM_U_0_");
  EXPECT_EQ(lookup(Mapper, "9lives"), "__LLVM_U_6_396C69766573");
  EXPECT_EQ(lookup(Mapper, "a.b"), "__LLVM_U_3_612E62");
  EXPECT_EQ(lookup(Mapper, EmbeddedNul), "__LLVM_U_3_610062");
  EXPECT_EQ(lookup(Mapper, UTF8), "__LLVM_U_2_C3A9");
}

TEST(MMIXALSymbolMapperTest, EscapesReservedImplementationPrefixOnce) {
  MMIXALSymbolMapper Mapper;
  expectSuccess(Mapper.registerUserSymbol("__LLVM_user"));
  expectSuccess(Mapper.registerUserSymbol("__LLVM_U_1_41"));
  expectSuccess(Mapper.finalize());

  EXPECT_EQ(lookup(Mapper, "__LLVM_user"),
            "__LLVM_U_11_5F5F4C4C564D5F75736572");
  EXPECT_EQ(lookup(Mapper, "__LLVM_U_1_41"),
            "__LLVM_U_13_5F5F4C4C564D5F555F315F3431");
}

TEST(MMIXALSymbolMapperTest, ReservesEveryMMIXALPredefinedSymbol) {
  static constexpr std::array<StringLiteral, 76> Names = {"rB",
                                                          "rD",
                                                          "rE",
                                                          "rH",
                                                          "rJ",
                                                          "rM",
                                                          "rR",
                                                          "rBB",
                                                          "rC",
                                                          "rN",
                                                          "rO",
                                                          "rS",
                                                          "rI",
                                                          "rT",
                                                          "rTT",
                                                          "rK",
                                                          "rQ",
                                                          "rU",
                                                          "rV",
                                                          "rG",
                                                          "rL",
                                                          "rA",
                                                          "rF",
                                                          "rP",
                                                          "rW",
                                                          "rX",
                                                          "rY",
                                                          "rZ",
                                                          "rWW",
                                                          "rXX",
                                                          "rYY",
                                                          "rZZ",
                                                          "ROUND_CURRENT",
                                                          "ROUND_OFF",
                                                          "ROUND_UP",
                                                          "ROUND_DOWN",
                                                          "ROUND_NEAR",
                                                          "Inf",
                                                          "Data_Segment",
                                                          "Pool_Segment",
                                                          "Stack_Segment",
                                                          "D_BIT",
                                                          "V_BIT",
                                                          "W_BIT",
                                                          "I_BIT",
                                                          "O_BIT",
                                                          "U_BIT",
                                                          "Z_BIT",
                                                          "X_BIT",
                                                          "D_Handler",
                                                          "V_Handler",
                                                          "W_Handler",
                                                          "I_Handler",
                                                          "O_Handler",
                                                          "U_Handler",
                                                          "Z_Handler",
                                                          "X_Handler",
                                                          "StdIn",
                                                          "StdOut",
                                                          "StdErr",
                                                          "TextRead",
                                                          "TextWrite",
                                                          "BinaryRead",
                                                          "BinaryWrite",
                                                          "BinaryReadWrite",
                                                          "Halt",
                                                          "Fopen",
                                                          "Fclose",
                                                          "Fread",
                                                          "Fgets",
                                                          "Fgetws",
                                                          "Fwrite",
                                                          "Fputs",
                                                          "Fputws",
                                                          "Fseek",
                                                          "Ftell"};

  MMIXALSymbolMapper Mapper;
  for (StringRef Name : Names) {
    EXPECT_TRUE(MMIXALSymbolMapper::isPredefinedSymbol(Name)) << Name.str();
    expectSuccess(Mapper.registerUserSymbol(Name));
  }
  expectSuccess(Mapper.finalize());

  for (StringRef Name : Names)
    EXPECT_EQ(lookup(Mapper, Name),
              MMIXALSymbolMapper::getEscapedUserSymbol(Name))
        << Name.str();
  EXPECT_FALSE(MMIXALSymbolMapper::isPredefinedSymbol("Main"));
  EXPECT_FALSE(MMIXALSymbolMapper::isPredefinedSymbol("ADD"));
}

TEST(MMIXALSymbolMapperTest, DiagnosesDuplicateSourceIdentity) {
  MMIXALSymbolMapper Mapper;
  expectSuccess(Mapper.registerUserSymbol("duplicate"));

  Error Duplicate = Mapper.registerUserSymbol("duplicate");
  ASSERT_TRUE(static_cast<bool>(Duplicate));
  EXPECT_EQ(toString(std::move(Duplicate)),
            "duplicate MMIXAL user symbol identity: "
            "__LLVM_U_9_6475706C6963617465");
}

TEST(MMIXALSymbolMapperTest, EnforcesRegistrationAndFinalizationBoundary) {
  MMIXALSymbolMapper Mapper;
  Expected<StringRef> EarlyLookup = Mapper.getMappedUserSymbol("symbol");
  ASSERT_FALSE(static_cast<bool>(EarlyLookup));
  EXPECT_EQ(toString(EarlyLookup.takeError()),
            "MMIXAL symbol mapper is not finalized");

  expectSuccess(Mapper.registerUserSymbol("symbol"));
  expectSuccess(Mapper.finalize());

  Error RepeatedFinalize = Mapper.finalize();
  ASSERT_TRUE(static_cast<bool>(RepeatedFinalize));
  EXPECT_EQ(toString(std::move(RepeatedFinalize)),
            "MMIXAL symbol mapper is already finalized");

  Error LateRegistration = Mapper.registerUserSymbol("late");
  ASSERT_TRUE(static_cast<bool>(LateRegistration));
  EXPECT_EQ(toString(std::move(LateRegistration)),
            "cannot register an MMIXAL user symbol after finalization");

  Expected<StringRef> Unknown = Mapper.getMappedUserSymbol("unknown");
  ASSERT_FALSE(static_cast<bool>(Unknown));
  EXPECT_EQ(toString(Unknown.takeError()),
            "unregistered MMIXAL user symbol identity: "
            "__LLVM_U_7_756E6B6E6F776E");
}

TEST(MMIXALSymbolMapperTest, MappingDoesNotDependOnRegistrationOrder) {
  static constexpr std::array<StringLiteral, 5> Names = {
      "Main", "a.b", "rA", "__LLVM_user", "ordinary"};
  MMIXALSymbolMapper Forward;
  MMIXALSymbolMapper Reverse;
  for (StringRef Name : Names)
    expectSuccess(Forward.registerUserSymbol(Name));
  for (StringRef Name : reverse(Names))
    expectSuccess(Reverse.registerUserSymbol(Name));
  expectSuccess(Forward.finalize());
  expectSuccess(Reverse.finalize());

  for (StringRef Name : Names)
    EXPECT_EQ(lookup(Forward, Name), lookup(Reverse, Name));
}

TEST(MMIXALSymbolMapperTest, EscapingIsInjectiveAtReservedBoundary) {
  MMIXALSymbolMapper Mapper;
  expectSuccess(Mapper.registerUserSymbol("A"));
  expectSuccess(Mapper.registerUserSymbol("__LLVM_U_1_41"));
  expectSuccess(Mapper.finalize());

  EXPECT_EQ(lookup(Mapper, "A"), "A");
  EXPECT_EQ(lookup(Mapper, "__LLVM_U_1_41"),
            "__LLVM_U_13_5F5F4C4C564D5F555F315F3431");
  EXPECT_NE(lookup(Mapper, "A"), lookup(Mapper, "__LLVM_U_1_41"));
}

} // namespace
