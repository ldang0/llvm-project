//===- MMIXAggregateABITest.cpp - MMIX aggregate ABI tests ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXAggregateABI.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(MMIXAggregateABITest, LeavesScalarsUnclassified) {
  MMIXAggregateABIValue Value;
  MMIXAggregateABIClassification Classification =
      classifyMMIXAggregateABI(Value);

  EXPECT_TRUE(Classification.isValid());
  EXPECT_FALSE(Classification.isAggregate());
  EXPECT_EQ(Classification.Kind, MMIXAggregateABIKind::Scalar);

  Value.HasUnsupportedFlags = true;
  Classification = classifyMMIXAggregateABI(Value);
  EXPECT_EQ(Classification.Kind, MMIXAggregateABIKind::Scalar);
  EXPECT_EQ(Classification.Error, MMIXAggregateABIError::UnsupportedFlags);
}

TEST(MMIXAggregateABITest, ClassifiesArgumentForms) {
  MMIXAggregateABIValue Value;
  Value.IsAggregate = true;

  Value.Size = 0;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Kind,
            MMIXAggregateABIKind::Empty);

  for (uint64_t Size = 1; Size <= 8; ++Size) {
    Value.Size = Size;
    EXPECT_EQ(classifyMMIXAggregateABI(Value).Kind,
              MMIXAggregateABIKind::DirectArgument);
  }

  Value.IsByVal = true;
  Value.Size = 24;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Kind,
            MMIXAggregateABIKind::CallerCopyArgument);
}

TEST(MMIXAggregateABITest, ClassifiesResultForms) {
  MMIXAggregateABIValue Value;
  Value.Role = MMIXAggregateABIRole::Result;
  Value.IsAggregate = true;
  Value.Size = 8;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Kind,
            MMIXAggregateABIKind::DirectResult);

  Value.IsSRet = true;
  Value.Size = 24;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Kind,
            MMIXAggregateABIKind::IndirectResult);
}

TEST(MMIXAggregateABITest, RejectsUnsupportedAggregateShapes) {
  MMIXAggregateABIValue Value;
  Value.IsAggregate = true;

  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::VariableSize);

  Value.Size = 8;
  Value.Alignment = Align(16);
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::OverAligned);

  Value.Alignment = Align(8);
  Value.AddressSpace = 1;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::NonZeroAddressSpace);

  Value.AddressSpace = 0;
  Value.IsSplit = true;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::Split);

  Value.IsSplit = false;
  Value.NumParts = 2;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::MultiRegister);

  Value.NumParts = 1;
  Value.IsInConsecutiveRegs = true;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::MultiRegister);

  Value.IsInConsecutiveRegs = false;
  Value.HasUnsupportedFlags = true;
  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::UnsupportedFlags);
}

TEST(MMIXAggregateABITest, RejectsWideDirectAggregates) {
  MMIXAggregateABIValue Value;
  Value.IsAggregate = true;
  Value.Size = 9;

  EXPECT_EQ(classifyMMIXAggregateABI(Value).Error,
            MMIXAggregateABIError::MultiRegister);
}

} // namespace
