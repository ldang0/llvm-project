//===-- MMIX.td - Describe the MMIX Target Machine ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This is the top level entry point for the MMIX target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// MMIX supported processors
//===----------------------------------------------------------------------===//

def : Processor<"generic", NoItineraries, []>;

//===----------------------------------------------------------------------===//
// Registers
//===----------------------------------------------------------------------===//

include "MMIXRegisterInfo.td"

defm : RemapAllTargetPseudoPointerOperands<GPR64>;

//===----------------------------------------------------------------------===//
// Instruction descriptions
//===----------------------------------------------------------------------===//

include "MMIXInstrInfo.td"

def MMIXInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Target declaration
//===----------------------------------------------------------------------===//

def MMIX : Target {
  let InstructionSet = MMIXInstrInfo;
}
