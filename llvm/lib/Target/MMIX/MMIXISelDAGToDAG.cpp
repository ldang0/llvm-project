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
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

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

  void selectSpecialArithmetic(SDNode *Node) {
    SDLoc DL(Node);
    bool IsMultiply = Node->getOpcode() == MMIXISD::UMUL_LOHI;
    bool IsUnsignedDivide = Node->getOpcode() == MMIXISD::UDIVREM;
    SDValue RHS = Node->getOperand(1);
    bool HasImmediate = false;
    if (auto *Constant = dyn_cast<ConstantSDNode>(RHS)) {
      HasImmediate = isUInt<8>(Constant->getZExtValue());
      if (HasImmediate)
        RHS = CurDAG->getTargetConstant(Constant->getZExtValue(), DL, MVT::i64);
    }

    unsigned Opcode;
    MCRegister SpecialResult;
    if (IsMultiply) {
      Opcode = HasImmediate ? MMIX::MULUI : MMIX::MULU;
      SpecialResult = MMIX::RH;
    } else if (IsUnsignedDivide) {
      Opcode = HasImmediate ? MMIX::DIVUI : MMIX::DIVU;
      SpecialResult = MMIX::RR;
    } else {
      Opcode = HasImmediate ? MMIX::DIVI : MMIX::DIV;
      SpecialResult = MMIX::RR;
    }

    SmallVector<SDValue, 3> Ops = {Node->getOperand(0), RHS};
    if (IsUnsignedDivide) {
      // A 64-bit LLVM dividend has no high half, so rD must be zero.
      SDNode *SetRD = CurDAG->getMachineNode(MMIX::SET_RD_ZERO, DL, MVT::Glue);
      Ops.push_back(SDValue(SetRD, 0));
    }

    // Glue prevents another special-register writer from being scheduled
    // between the arithmetic instruction and a required GET.
    SDNode *Arithmetic = CurDAG->getMachineNode(
        Opcode, DL, CurDAG->getVTList(MVT::i64, MVT::Glue), Ops);

    if (!SDValue(Node, 0).use_empty())
      ReplaceUses(SDValue(Node, 0), SDValue(Arithmetic, 0));
    if (!SDValue(Node, 1).use_empty()) {
      SDValue SpecialReg = CurDAG->getRegister(SpecialResult, MVT::i64);
      SDValue GetOps[] = {SpecialReg, SDValue(Arithmetic, 1)};
      SDNode *Get = CurDAG->getMachineNode(MMIX::GET, DL, MVT::i64, GetOps);
      ReplaceUses(SDValue(Node, 1), SDValue(Get, 0));
    }
    CurDAG->RemoveDeadNode(Node);
  }

  void Select(SDNode *Node) override {
    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }

    if (auto *Mem = dyn_cast<MemSDNode>(Node); Mem && Mem->getAddressSpace())
      report_fatal_error("MMIX does not support nonzero address spaces");

    if (Node->getOpcode() == MMIXISD::UMUL_LOHI ||
        Node->getOpcode() == MMIXISD::SDIVREM ||
        Node->getOpcode() == MMIXISD::UDIVREM) {
      selectSpecialArithmetic(Node);
      return;
    }

    if (Node->getOpcode() == ISD::ATOMIC_CMP_SWAP &&
        cast<AtomicSDNode>(Node)->getAlign() < Align(8))
      report_fatal_error(
          "MMIX requires naturally aligned octabyte compare-and-swap");

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

    if (Node->getOpcode() == MMIXISD::CALL) {
      SmallVector<SDValue, 20> Ops;
      bool HasGlue =
          Node->getNumOperands() > 1 &&
          Node->getOperand(Node->getNumOperands() - 1).getValueType() ==
              MVT::Glue;
      unsigned LastDataOperand = Node->getNumOperands() - HasGlue;
      for (unsigned I = 1; I != LastDataOperand; ++I)
        Ops.push_back(Node->getOperand(I));
      Ops.push_back(Node->getOperand(0));
      if (HasGlue)
        Ops.push_back(Node->getOperand(Node->getNumOperands() - 1));
      CurDAG->SelectNodeTo(Node, MMIX::CALL_STATE, MVT::Other, MVT::Glue, Ops);
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
