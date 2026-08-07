//===-- MMIXSelectionDAGInfo.cpp - MMIX SelectionDAG info ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXSelectionDAGInfo.h"
#include "MMIXISelLowering.h"

using namespace llvm;

bool MMIXSelectionDAGInfo::isTargetMemoryOpcode(unsigned Opcode) const {
  return Opcode == MMIXISD::UNCACHED_LOAD || Opcode == MMIXISD::UNCACHED_STORE;
}
