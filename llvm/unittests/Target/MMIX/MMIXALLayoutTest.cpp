//===- MMIXALLayoutTest.cpp - MMIXAL layout unit tests -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALLayout.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"
#include <limits>
#include <string>
#include <utility>

using namespace llvm;

namespace {

size_t groupIndex(MMIXALLogicalGroup Group) {
  return static_cast<size_t>(Group);
}

MMIXALBufferedItem makeItem(MMIXALLogicalGroup Group, uint64_t SourceOrder,
                            uint64_t Size, uint64_t Alignment = 1) {
  MMIXALBufferedItem Item{Group, nullptr, SourceOrder};
  Item.RequiredAlignment = Alignment;
  Item.KnownSize = Size;
  Item.HasPayload = Size != 0;
  return Item;
}

void addItem(MMIXALItemGroups &Groups, MMIXALBufferedItem Item) {
  Groups[groupIndex(Item.Group)].push_back(std::move(Item));
}

std::string expectLayoutError(Expected<MMIXALLayoutPlan> Result) {
  if (Result) {
    ADD_FAILURE() << "layout unexpectedly succeeded";
    return {};
  }
  return toString(Result.takeError());
}

TEST(MMIXALLayoutTest, PlacesGroupsBySourceOrderAndAccountsForPadding) {
  MMIXALItemGroups Groups;
  addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 3, 4, 16));
  addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 1, 4));
  addItem(Groups, makeItem(MMIXALLogicalGroup::ReadOnly, 5, 1, 8));
  addItem(Groups, makeItem(MMIXALLogicalGroup::ConstantPool, 4, 8, 8));

  Expected<MMIXALLayoutPlan> Result = planMMIXALBareMetalLayout(Groups);
  ASSERT_TRUE(static_cast<bool>(Result)) << toString(Result.takeError());
  ArrayRef<MMIXALPlacedItem> Items = Result->getItems();
  ASSERT_EQ(Items.size(), 4u);

  EXPECT_EQ(Items[0].Input->SourceOrder, 1u);
  EXPECT_EQ(Items[0].Begin, MMIXALBareMetalProfile::TextStart);
  EXPECT_EQ(Items[0].End, MMIXALBareMetalProfile::TextStart + 4);

  EXPECT_EQ(Items[1].Input->SourceOrder, 3u);
  EXPECT_EQ(Items[1].Begin, MMIXALBareMetalProfile::TextStart + 16);
  EXPECT_EQ(Items[1].End, MMIXALBareMetalProfile::TextStart + 20);
  ASSERT_EQ(Items[1].Padding.size(), 1u);
  EXPECT_EQ(Items[1].Padding[0].Begin, MMIXALBareMetalProfile::TextStart + 4);
  EXPECT_EQ(Items[1].Padding[0].End, MMIXALBareMetalProfile::TextStart + 16);

  EXPECT_EQ(Items[2].Input->Group, MMIXALLogicalGroup::ReadOnly);
  EXPECT_EQ(Items[2].Begin, MMIXALBareMetalProfile::DataStart);
  EXPECT_EQ(Items[2].End, MMIXALBareMetalProfile::DataStart + 1);

  EXPECT_EQ(Items[3].Input->Group, MMIXALLogicalGroup::ConstantPool);
  EXPECT_EQ(Items[3].Begin, MMIXALBareMetalProfile::DataStart + 8);
  EXPECT_EQ(Items[3].End, MMIXALBareMetalProfile::DataStart + 16);
  ASSERT_EQ(Items[3].Padding.size(), 1u);
  EXPECT_EQ(Items[3].Padding[0].Begin, MMIXALBareMetalProfile::DataStart + 1);
  EXPECT_EQ(Items[3].Padding[0].End, MMIXALBareMetalProfile::DataStart + 8);
}

TEST(MMIXALLayoutTest, AppliesEveryMaximumPaddingConstraintInSequence) {
  MMIXALItemGroups Groups;
  addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 0, 1));
  MMIXALBufferedItem Aligned = makeItem(MMIXALLogicalGroup::Text, 1, 4, 8);
  Aligned.Alignments.push_back({4, 0, 1, 2, true});
  Aligned.Alignments.push_back({8, 0, 1, 7, true});
  addItem(Groups, std::move(Aligned));

  EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
            "MMIXAL layout item at source order 1: alignment requires 3 bytes "
            "of padding, exceeding the maximum of 2");
}

TEST(MMIXALLayoutTest, RejectsInvalidAlignments) {
  for (uint64_t Alignment : {0ULL, 3ULL}) {
    SCOPED_TRACE(Alignment);
    MMIXALItemGroups Groups;
    MMIXALBufferedItem Item = makeItem(MMIXALLogicalGroup::Text, 0, 4);
    Item.Alignments.push_back({Alignment});
    addItem(Groups, std::move(Item));

    const std::string Diagnostic =
        expectLayoutError(planMMIXALBareMetalLayout(Groups));
    EXPECT_NE(Diagnostic.find(Alignment == 0 ? "must not be zero"
                                             : "must be a power of two"),
              std::string::npos)
        << Diagnostic;
  }
}

TEST(MMIXALLayoutTest, RejectsUnknownOverflowingAndOutOfWindowSizes) {
  {
    MMIXALItemGroups Groups;
    MMIXALBufferedItem Item = makeItem(MMIXALLogicalGroup::ReadOnly, 0, 0);
    Item.SizeIsKnown = false;
    addItem(Groups, std::move(Item));
    EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
              "MMIXAL layout item at source order 0: size is not known before "
              "placement");
  }
  {
    MMIXALItemGroups Groups;
    addItem(Groups, makeItem(MMIXALLogicalGroup::ReadOnly, 0,
                             std::numeric_limits<uint64_t>::max()));
    EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
              "MMIXAL layout item at source order 0: size overflows the "
              "64-bit address");
  }
  {
    MMIXALItemGroups Groups;
    addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 0,
                             MMIXALBareMetalProfile::TextEnd -
                                 MMIXALBareMetalProfile::TextStart + 1));
    EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
              "MMIXAL layout item at source order 0: interval crosses its "
              "allocation window");
  }
}

TEST(MMIXALLayoutTest, RejectsAlignmentOutsideTheAllocationWindow) {
  MMIXALItemGroups Groups;
  addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 0, 4, UINT64_C(0x20000)));

  EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
            "MMIXAL layout item at source order 0: alignment crosses its "
            "allocation window");
}

TEST(MMIXALLayoutTest, RejectsOverlappingAllocationWindows) {
  MMIXALItemGroups Groups;
  addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 0, 16));
  addItem(Groups, makeItem(MMIXALLogicalGroup::ReadOnly, 1, 8));

  EXPECT_EQ(expectLayoutError(
                planMMIXALLayout(Groups, {0x100, 0x110}, {0x108, 0x118})),
            "MMIXAL layout item at source order 1: assigned interval overlaps "
            "a prior item");
}

TEST(MMIXALLayoutTest, AcceptsExactAllocationWindowBoundaries) {
  MMIXALItemGroups Groups;
  addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 0,
                           MMIXALBareMetalProfile::TextEnd -
                               MMIXALBareMetalProfile::TextStart));
  addItem(Groups, makeItem(MMIXALLogicalGroup::ZeroStorage, 1,
                           MMIXALBareMetalProfile::DataEnd -
                               MMIXALBareMetalProfile::DataStart));

  Expected<MMIXALLayoutPlan> Result = planMMIXALBareMetalLayout(Groups);
  ASSERT_TRUE(static_cast<bool>(Result)) << toString(Result.takeError());
  ASSERT_EQ(Result->getItems().size(), 2u);
  EXPECT_EQ(Result->getItems()[0].End, MMIXALBareMetalProfile::TextEnd);
  EXPECT_EQ(Result->getItems()[1].End, MMIXALBareMetalProfile::DataEnd);
  EXPECT_EQ(MMIXALBareMetalProfile::RegisterStackStart,
            MMIXALBareMetalProfile::TextEnd);
  EXPECT_EQ(MMIXALBareMetalProfile::MemoryStackStart,
            MMIXALBareMetalProfile::DataEnd);
}

TEST(MMIXALLayoutTest, RejectsAmbiguousSourceOrderAndGroupMembership) {
  {
    MMIXALItemGroups Groups;
    addItem(Groups, makeItem(MMIXALLogicalGroup::Text, 0, 4));
    addItem(Groups, makeItem(MMIXALLogicalGroup::ReadOnly, 0, 4));
    EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
              "MMIXAL layout item at source order 0: source order is "
              "duplicated and placement is ambiguous");
  }
  {
    MMIXALItemGroups Groups;
    Groups[groupIndex(MMIXALLogicalGroup::ReadOnly)].push_back(
        makeItem(MMIXALLogicalGroup::WritableData, 0, 4));
    EXPECT_EQ(expectLayoutError(planMMIXALBareMetalLayout(Groups)),
              "MMIXAL layout item at source order 0: item is stored in the "
              "wrong logical group");
  }
}

} // namespace
