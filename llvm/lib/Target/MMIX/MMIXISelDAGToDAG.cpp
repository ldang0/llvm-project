//===-- MMIXISelDAGToDAG.cpp - MMIX DAG instruction selection -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIX.h"
#include "MMIXTargetMachine.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "mmix-isel"
#define PASS_NAME "MMIX DAG->DAG Pattern Instruction Selection"

namespace {

class MMIXDAGToDAGISel final : public SelectionDAGISel {
public:
  explicit MMIXDAGToDAGISel(MMIXTargetMachine &TM) : SelectionDAGISel(TM) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    MF.getFunction().getContext().emitError(
        "MMIX instruction selection is not implemented");
    return false;
  }

private:
  void Select(SDNode *) override {
    llvm_unreachable("MMIX instruction selection is not implemented");
  }
};

class MMIXDAGToDAGISelLegacy final : public SelectionDAGISelLegacy {
public:
  static char ID;

  explicit MMIXDAGToDAGISelLegacy(MMIXTargetMachine &TM)
      : SelectionDAGISelLegacy(ID, std::make_unique<MMIXDAGToDAGISel>(TM)) {}
};

} // namespace

char MMIXDAGToDAGISelLegacy::ID = 0;

INITIALIZE_PASS(MMIXDAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createMMIXISelDag(MMIXTargetMachine &TM) {
  return new MMIXDAGToDAGISelLegacy(TM);
}
