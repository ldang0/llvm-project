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

// These features describe implementation capabilities. Privilege remains
// dynamic architectural state and is not implied by enabling a feature.
def FeatureMMIXBase : SubtargetFeature<"base", "HasMMIXBase", "true",
                                        "MMIX base ISA">;
def FeatureMMIXSystem : SubtargetFeature<"system", "HasMMIXSystem", "true",
                                          "MMIX system instructions">;
def FeatureMMIXCache : SubtargetFeature<"cache", "HasMMIXCache", "true",
                                         "MMIX cache-management instructions">;
def FeatureMMIXVirtualMemory : SubtargetFeature<"virtual-memory",
                                                 "HasMMIXVirtualMemory", "true",
                                                 "MMIX virtual-memory instructions">;

def HasMMIXBase : Predicate<"Subtarget->hasMMIXBase()">,
                  AssemblerPredicate<(all_of FeatureMMIXBase), "base">;
def HasMMIXSystem : Predicate<"Subtarget->hasMMIXSystem()">,
                    AssemblerPredicate<(all_of FeatureMMIXSystem), "system">;
def HasMMIXCache : Predicate<"Subtarget->hasMMIXCache()">,
                   AssemblerPredicate<(all_of FeatureMMIXCache), "cache">;
def HasMMIXVirtualMemory
    : Predicate<"Subtarget->hasMMIXVirtualMemory()">,
      AssemblerPredicate<(all_of FeatureMMIXVirtualMemory), "virtual-memory">;

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
