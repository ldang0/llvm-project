//===-- MMIXALLayout.h - Plan MMIXAL bare-metal locations -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALLAYOUT_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALLAYOUT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace llvm {

class MCSection;
class MCSymbol;

enum class MMIXALLogicalGroup : uint8_t {
  Text,
  ReadOnly,
  ConstantPool,
  JumpTable,
  WritableData,
  ZeroStorage,
};

inline constexpr size_t MMIXALLogicalGroupCount = 6;

struct MMIXALAlignmentRequest {
  uint64_t Alignment = 1;
  int64_t Fill = 0;
  uint8_t FillLength = 1;
  unsigned MaxBytesToEmit = 0;
  bool IsCodeAlignment = false;
};

struct MMIXALBufferedItem {
  MMIXALLogicalGroup Group;
  const MCSection *Section = nullptr;
  uint64_t SourceOrder = 0;
  SmallVector<size_t, 4> EventIndices;
  SmallVector<const MCSymbol *, 2> OwningSymbols;
  SmallVector<const MCSymbol *, 2> Dependencies;
  SmallVector<MMIXALAlignmentRequest, 1> Alignments;
  uint64_t RequiredAlignment = 1;
  uint64_t KnownSize = 0;
  bool SizeIsKnown = true;
  bool HasPayload = false;
};

using MMIXALItemGroups =
    std::array<SmallVector<MMIXALBufferedItem, 0>, MMIXALLogicalGroupCount>;

struct MMIXALBareMetalProfile {
  static constexpr uint64_t TextStart = 0x0000000000000100ULL;
  static constexpr uint64_t TextEnd = 0x0000000000010000ULL;
  static constexpr uint64_t RegisterStackStart = TextEnd;
  static constexpr uint64_t RegisterStackEnd = 0x0000000004000000ULL;
  static constexpr uint64_t DataStart = 0x2000000000000000ULL;
  static constexpr uint64_t DataEnd = 0x2000000003FF0000ULL;
  static constexpr uint64_t MemoryStackStart = DataEnd;
  static constexpr uint64_t MemoryStackEnd = 0x2000000004000000ULL;
};

struct MMIXALPaddingInterval {
  uint64_t Begin = 0;
  uint64_t End = 0;
  MMIXALAlignmentRequest Request;
};

struct MMIXALPlacedItem {
  const MMIXALBufferedItem *Input = nullptr;
  uint64_t Begin = 0;
  uint64_t End = 0;
  SmallVector<MMIXALPaddingInterval, 1> Padding;
};

class MMIXALLayoutPlan {
  SmallVector<MMIXALPlacedItem, 0> Items;

  friend Expected<MMIXALLayoutPlan>
  planMMIXALBareMetalLayout(const MMIXALItemGroups &Groups);

public:
  ArrayRef<MMIXALPlacedItem> getItems() const { return Items; }
};

Expected<MMIXALLayoutPlan>
planMMIXALBareMetalLayout(const MMIXALItemGroups &Groups);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALLAYOUT_H
