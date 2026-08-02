//===-- MMIXInstrInfo.cpp - MMIX instruction information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXInstrInfo.h"
#include "MMIXSubtarget.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "MMIXGenInstrInfo.inc"

MMIXInstrInfo::MMIXInstrInfo(const MMIXSubtarget &STI)
    : MMIXGenInstrInfo(STI, RI), RI() {}
