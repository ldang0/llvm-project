//===-- MMIXISelLowering.cpp - MMIX DAG lowering implementation ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXISelLowering.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXSubtarget.h"

using namespace llvm;

MMIXTargetLowering::MMIXTargetLowering(const TargetMachine &TM,
                                       const MMIXSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i64, &MMIX::GPR64RegClass);
  addRegisterClass(MVT::f64, &MMIX::FPR64RegClass);
  computeRegisterProperties(STI.getRegisterInfo());
  setStackPointerRegisterToSaveRestore(MMIX::R254);
}
