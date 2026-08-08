//===-- MMIXALLayout.cpp - Plan MMIXAL bare-metal locations --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALLayout.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CheckedArithmetic.h"
#include "llvm/Support/MathExtras.h"
#include <algorithm>
#include <optional>
#include <string>
#include <utility>

using namespace llvm;

namespace {

std::string describeItem(const MMIXALBufferedItem &Item) {
  if (!Item.OwningSymbols.empty())
    return (Twine("'") + Item.OwningSymbols.front()->getName() + "'").str();
  return (Twine("at source order ") + Twine(Item.SourceOrder)).str();
}

Error makeLayoutError(const MMIXALBufferedItem &Item, const Twine &Message) {
  return createStringError(Twine("MMIXAL layout item ") + describeItem(Item) +
                           ": " + Message);
}

Expected<uint64_t>
applyAlignment(uint64_t Cursor, const MMIXALAlignmentRequest &Request,
               const MMIXALBufferedItem &Item, uint64_t WindowEnd,
               SmallVectorImpl<MMIXALPaddingInterval> &Padding) {
  const uint64_t Alignment = Request.Alignment;
  if (Alignment == 0)
    return makeLayoutError(Item, "alignment must not be zero");
  if (!isPowerOf2_64(Alignment))
    return makeLayoutError(Item, "alignment must be a power of two");

  const uint64_t Misalignment = Cursor & (Alignment - 1);
  const uint64_t PaddingSize = Misalignment ? Alignment - Misalignment : 0;
  if (Request.MaxBytesToEmit != 0 && PaddingSize > Request.MaxBytesToEmit)
    return makeLayoutError(Item,
                           Twine("alignment requires ") + Twine(PaddingSize) +
                               " bytes of padding, exceeding the maximum of " +
                               Twine(Request.MaxBytesToEmit));

  std::optional<uint64_t> Aligned =
      checkedAddUnsigned<uint64_t>(Cursor, PaddingSize);
  if (!Aligned)
    return makeLayoutError(Item, "alignment overflows the 64-bit address");
  if (*Aligned > WindowEnd)
    return makeLayoutError(Item, "alignment crosses its allocation window");
  if (PaddingSize != 0)
    Padding.push_back({Cursor, *Aligned, Request});
  return *Aligned;
}

Expected<MMIXALPlacedItem> planItem(const MMIXALBufferedItem &Item,
                                    uint64_t Cursor, uint64_t WindowEnd) {
  if (!Item.SizeIsKnown)
    return makeLayoutError(Item, "size is not known before placement");

  MMIXALPlacedItem Placed;
  Placed.Input = &Item;

  MMIXALAlignmentRequest Required{Item.RequiredAlignment};
  if (Required.Alignment == 0)
    return makeLayoutError(Item, "required alignment must not be zero");
  if (!isPowerOf2_64(Required.Alignment))
    return makeLayoutError(Item, "required alignment must be a power of two");

  uint64_t MaximumExplicitAlignment = 1;
  for (const MMIXALAlignmentRequest &Request : Item.Alignments) {
    Expected<uint64_t> Aligned =
        applyAlignment(Cursor, Request, Item, WindowEnd, Placed.Padding);
    if (!Aligned)
      return Aligned.takeError();
    Cursor = *Aligned;
    MaximumExplicitAlignment =
        std::max(MaximumExplicitAlignment, Request.Alignment);
  }
  if (Required.Alignment > MaximumExplicitAlignment) {
    Expected<uint64_t> Aligned =
        applyAlignment(Cursor, Required, Item, WindowEnd, Placed.Padding);
    if (!Aligned)
      return Aligned.takeError();
    Cursor = *Aligned;
  }

  if (Cursor >= WindowEnd)
    return makeLayoutError(Item,
                           "start address is outside its allocation window");
  Placed.Begin = Cursor;
  std::optional<uint64_t> End =
      checkedAddUnsigned<uint64_t>(Cursor, Item.KnownSize);
  if (!End)
    return makeLayoutError(Item, "size overflows the 64-bit address");
  if (*End > WindowEnd)
    return makeLayoutError(Item, "interval crosses its allocation window");
  Placed.End = *End;
  return Placed;
}

Error validateSourceOrders(const MMIXALItemGroups &Groups) {
  DenseSet<uint64_t> SourceOrders;
  for (const auto &Group : Groups)
    for (const MMIXALBufferedItem &Item : Group)
      if (!SourceOrders.insert(Item.SourceOrder).second)
        return makeLayoutError(
            Item, "source order is duplicated and placement is ambiguous");
  return Error::success();
}

} // namespace

Expected<MMIXALLayoutPlan>
llvm::planMMIXALBareMetalLayout(const MMIXALItemGroups &Groups) {
  static_assert(static_cast<size_t>(MMIXALLogicalGroup::ZeroStorage) + 1 ==
                MMIXALLogicalGroupCount);
  static_assert(MMIXALBareMetalProfile::TextStart <
                MMIXALBareMetalProfile::TextEnd);
  static_assert(MMIXALBareMetalProfile::TextEnd ==
                MMIXALBareMetalProfile::RegisterStackStart);
  static_assert(MMIXALBareMetalProfile::RegisterStackStart <
                MMIXALBareMetalProfile::RegisterStackEnd);
  static_assert(MMIXALBareMetalProfile::RegisterStackEnd <=
                MMIXALBareMetalProfile::DataStart);
  static_assert(MMIXALBareMetalProfile::DataStart <
                MMIXALBareMetalProfile::DataEnd);
  static_assert(MMIXALBareMetalProfile::DataEnd ==
                MMIXALBareMetalProfile::MemoryStackStart);
  static_assert(MMIXALBareMetalProfile::MemoryStackStart <
                MMIXALBareMetalProfile::MemoryStackEnd);

  if (Error Err = validateSourceOrders(Groups))
    return std::move(Err);

  MMIXALLayoutPlan Plan;
  uint64_t TextCursor = MMIXALBareMetalProfile::TextStart;
  uint64_t DataCursor = MMIXALBareMetalProfile::DataStart;

  for (size_t GroupIndex = 0; GroupIndex != MMIXALLogicalGroupCount;
       ++GroupIndex) {
    const MMIXALLogicalGroup Group =
        static_cast<MMIXALLogicalGroup>(GroupIndex);
    SmallVector<const MMIXALBufferedItem *, 0> OrderedItems;
    for (const MMIXALBufferedItem &Item : Groups[GroupIndex]) {
      if (Item.Group != Group)
        return makeLayoutError(Item,
                               "item is stored in the wrong logical group");
      OrderedItems.push_back(&Item);
    }
    llvm::stable_sort(OrderedItems, [](const MMIXALBufferedItem *Left,
                                       const MMIXALBufferedItem *Right) {
      return Left->SourceOrder < Right->SourceOrder;
    });

    uint64_t &Cursor =
        Group == MMIXALLogicalGroup::Text ? TextCursor : DataCursor;
    const uint64_t WindowEnd = Group == MMIXALLogicalGroup::Text
                                   ? MMIXALBareMetalProfile::TextEnd
                                   : MMIXALBareMetalProfile::DataEnd;
    for (const MMIXALBufferedItem *Item : OrderedItems) {
      Expected<MMIXALPlacedItem> Placed = planItem(*Item, Cursor, WindowEnd);
      if (!Placed)
        return Placed.takeError();
      if (!Plan.Items.empty() && Placed->Begin < Plan.Items.back().End)
        return makeLayoutError(*Item,
                               "assigned interval overlaps a prior item");
      Cursor = Placed->End;
      Plan.Items.push_back(std::move(*Placed));
    }
  }
  return Plan;
}
