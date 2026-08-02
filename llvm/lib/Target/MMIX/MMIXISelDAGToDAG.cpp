//===-- MMIXISelDAGToDAG.cpp - MMIX DAG instruction selection -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIX.h"
#include "MMIXISelLowering.h"
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

    if (Node->getOpcode() == MMIXISD::LOAD_STACK_ARG) {
      SDLoc DL(Node);
      auto *FIN = cast<FrameIndexSDNode>(Node->getOperand(1));
      SDValue FrameIndex =
          CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
      SDValue Offset = CurDAG->getTargetConstant(0, DL, MVT::i64);
      SDValue Chain = Node->getOperand(0);
      SDValue Ops[] = {FrameIndex, Offset, Chain};
      CurDAG->SelectNodeTo(Node, MMIX::LDOUI, Node->getValueType(0), MVT::Other,
                           Ops);
      return;
    }

    if (Node->getOpcode() == MMIXISD::RET_GLUE) {
      CurDAG->SelectNodeTo(Node, MMIX::RET, MVT::Other, Node->getOperand(0));
      return;
    }

    if (Node->getOpcode() == MMIXISD::RET_VALUE_GLUE) {
      CurDAG->SelectNodeTo(Node, MMIX::RET_VALUE, MVT::Other,
                           Node->getOperand(0), Node->getOperand(1));
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
