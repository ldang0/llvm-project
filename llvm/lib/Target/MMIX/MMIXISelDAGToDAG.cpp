//===-- MMIXISelDAGToDAG.cpp - MMIX DAG instruction selection -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIX.h"
#include "MMIXTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "mmix-isel"
#define PASS_NAME "MMIX DAG->DAG Pattern Instruction Selection"

namespace {

class MMIXDAGToDAGISel final : public SelectionDAGISel {
public:
  explicit MMIXDAGToDAGISel(MMIXTargetMachine &TM) : SelectionDAGISel(TM) {}

private:
// Include the TableGen pattern matcher. Family-specific patterns are added by
// the lowering tasks that own their semantics.
#include "MMIXGenDAGISel.inc"

  void Select(SDNode *Node) override {
    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }
    SelectCode(Node);
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
