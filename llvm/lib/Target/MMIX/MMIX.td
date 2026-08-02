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

// These features describe implementation capabilities.  They are metadata
// only for now; instruction availability predicates will be added when the
// target has subtarget-aware lowering and encoding support.
def FeatureMMIXBase : SubtargetFeature<"base", "HasMMIXBase", "true",
                                        "MMIX base ISA">;
def FeatureMMIXSystem : SubtargetFeature<"system", "HasMMIXSystem", "true",
                                          "MMIX system instructions">;
def FeatureMMIXCache : SubtargetFeature<"cache", "HasMMIXCache", "true",
                                         "MMIX cache-management instructions">;
def FeatureMMIXVirtualMemory : SubtargetFeature<"virtual-memory",
                                                 "HasMMIXVirtualMemory", "true",
                                                 "MMIX virtual-memory instructions">;

def : Processor<"generic", NoItineraries,
                [FeatureMMIXBase, FeatureMMIXSystem, FeatureMMIXCache,
                 FeatureMMIXVirtualMemory]>;

//===----------------------------------------------------------------------===//
// Registers
//===----------------------------------------------------------------------===//

include "MMIXRegisterInfo.td"
include "MMIXCallingConv.td"

defm : RemapAllTargetPseudoPointerOperands<GPR64CodeGen>;

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
