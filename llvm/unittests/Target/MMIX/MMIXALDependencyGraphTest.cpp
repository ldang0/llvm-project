//===- MMIXALDependencyGraphTest.cpp - MMIXAL scheduling tests -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALDependencyGraph.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(MMIXALDependencyGraphTest, PreservesOrderWithoutDependencies) {
  SmallVector<MMIXALItemDependencies, 0> Dependencies(4);
  Expected<SmallVector<size_t, 0>> Order = scheduleMMIXALItems(Dependencies);
  ASSERT_TRUE(static_cast<bool>(Order));
  EXPECT_EQ(*Order, SmallVector<size_t>({0, 1, 2, 3}));
}

TEST(MMIXALDependencyGraphTest, UsesStableTopologicalOrder) {
  SmallVector<MMIXALItemDependencies, 0> Dependencies(4);
  Dependencies[0].push_back(3);
  Dependencies[2].push_back(1);
  Expected<SmallVector<size_t, 0>> Order = scheduleMMIXALItems(Dependencies);
  ASSERT_TRUE(static_cast<bool>(Order));
  EXPECT_EQ(*Order, SmallVector<size_t>({1, 2, 3, 0}));
}

TEST(MMIXALDependencyGraphTest, RejectsCyclesAndInvalidItems) {
  SmallVector<MMIXALItemDependencies, 0> Cyclic(2);
  Cyclic[0].push_back(1);
  Cyclic[1].push_back(0);
  Expected<SmallVector<size_t, 0>> Cycle = scheduleMMIXALItems(Cyclic);
  ASSERT_FALSE(static_cast<bool>(Cycle));
  EXPECT_EQ(toString(Cycle.takeError()), "MMIXAL definition dependency cycle");

  SmallVector<MMIXALItemDependencies, 0> Invalid(1);
  Invalid[0].push_back(1);
  Expected<SmallVector<size_t, 0>> BadItem = scheduleMMIXALItems(Invalid);
  ASSERT_FALSE(static_cast<bool>(BadItem));
  EXPECT_EQ(toString(BadItem.takeError()),
            "MMIXAL item dependency references an invalid item");
}

} // namespace
