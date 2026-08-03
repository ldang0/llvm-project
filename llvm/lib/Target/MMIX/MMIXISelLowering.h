//===-- MMIXISelLowering.h - MMIX DAG lowering interface ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXISELLOWERING_H
#define LLVM_LIB_TARGET_MMIX_MMIXISELLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class MMIXSubtarget;

namespace MMIXISD {

enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  LOAD_STACK_ARG,
  UMUL_LOHI,
  SDIVREM,
  UDIVREM,
  LOAD_ADDR,
  RET_GLUE,
  RET_VALUE_GLUE,
};

} // namespace MMIXISD

class MMIXTargetLowering final : public TargetLowering {
public:
  MMIXTargetLowering(const TargetMachine &TM, const MMIXSubtarget &STI);

  bool allowsMisalignedMemoryAccesses(
      EVT VT, unsigned AddrSpace, Align Alignment,
      MachineMemOperand::Flags Flags = MachineMemOperand::MONone,
      unsigned *Fast = nullptr) const override;
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context, const Type *RetTy) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXISELLOWERING_H
