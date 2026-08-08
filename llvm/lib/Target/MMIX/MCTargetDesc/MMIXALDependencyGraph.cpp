//===-- MMIXALDependencyGraph.cpp - Schedule MMIXAL items ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALDependencyGraph.h"
#include <optional>

using namespace llvm;

Expected<SmallVector<size_t, 0>>
llvm::scheduleMMIXALItems(ArrayRef<MMIXALItemDependencies> Dependencies) {
  const size_t ItemCount = Dependencies.size();
  for (size_t Item = 0; Item != ItemCount; ++Item)
    for (size_t Dependency : Dependencies[Item])
      if (Dependency >= ItemCount)
        return createStringError(
            "MMIXAL item dependency references an invalid item");

  SmallVector<bool, 0> Scheduled(ItemCount, false);
  SmallVector<size_t, 0> Order;
  Order.reserve(ItemCount);
  while (Order.size() != ItemCount) {
    std::optional<size_t> Next;
    for (size_t Item = 0; Item != ItemCount; ++Item) {
      if (Scheduled[Item])
        continue;
      bool Ready = true;
      for (size_t Dependency : Dependencies[Item])
        if (!Scheduled[Dependency]) {
          Ready = false;
          break;
        }
      if (Ready) {
        Next = Item;
        break;
      }
    }
    if (!Next)
      return createStringError("MMIXAL definition dependency cycle");
    Scheduled[*Next] = true;
    Order.push_back(*Next);
  }
  return Order;
}
