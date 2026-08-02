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
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

MMIXTargetLowering::MMIXTargetLowering(const TargetMachine &TM,
                                       const MMIXSubtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i64, &MMIX::GPR64CodeGenRegClass);
  addRegisterClass(MVT::f64, &MMIX::FPR64CodeGenRegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(MMIX::R254);
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setMinFunctionAlignment(Align(4));
  setPrefFunctionAlignment(Align(4));
  setMaxAtomicSizeInBitsSupported(0);

  auto RejectOperation = [this](unsigned Opcode, MVT VT) {
    setOperationAction(Opcode, VT, Custom);
  };

  // Only i64 and f64 have ABI register classes. Type legalization promotes
  // narrower scalar values and splits wider scalar and vector values. Keep
  // legal-width operations unavailable until their dedicated lowering task
  // defines the exact MMIX semantics.
  static constexpr unsigned IntegerOperations[] = {
      ISD::Constant, ISD::ADD,    ISD::SUB,      ISD::MUL,  ISD::SDIV,
      ISD::UDIV,     ISD::SREM,   ISD::UREM,     ISD::AND,  ISD::OR,
      ISD::XOR,      ISD::SHL,    ISD::SRA,      ISD::SRL,  ISD::ROTL,
      ISD::ROTR,     ISD::BSWAP,  ISD::CTPOP,    ISD::CTLZ, ISD::CTTZ,
      ISD::SETCC,    ISD::SELECT, ISD::SELECT_CC};
  for (unsigned Opcode : IntegerOperations)
    RejectOperation(Opcode, MVT::i64);

  static constexpr unsigned FloatingOperations[] = {
      ISD::ConstantFP, ISD::FADD,  ISD::FSUB,   ISD::FMUL,      ISD::FDIV,
      ISD::FREM,       ISD::FNEG,  ISD::FABS,   ISD::FCOPYSIGN, ISD::FSQRT,
      ISD::FMA,        ISD::SETCC, ISD::SELECT, ISD::SELECT_CC};
  for (unsigned Opcode : FloatingOperations)
    RejectOperation(Opcode, MVT::f64);

  static constexpr unsigned AddressOperations[] = {
      ISD::GlobalAddress, ISD::ExternalSymbol, ISD::BlockAddress,
      ISD::ConstantPool,  ISD::JumpTable,      ISD::FrameIndex,
      ISD::FRAMEADDR,     ISD::RETURNADDR,     ISD::DYNAMIC_STACKALLOC};
  for (unsigned Opcode : AddressOperations)
    RejectOperation(Opcode, MVT::i64);

  RejectOperation(ISD::LOAD, MVT::i64);
  RejectOperation(ISD::STORE, MVT::i64);
  RejectOperation(ISD::LOAD, MVT::f64);
  RejectOperation(ISD::STORE, MVT::f64);
  RejectOperation(ISD::BR_CC, MVT::i64);
  RejectOperation(ISD::BR_CC, MVT::f64);
  RejectOperation(ISD::BRCOND, MVT::Other);
  RejectOperation(ISD::BR_JT, MVT::Other);
  RejectOperation(ISD::STACKSAVE, MVT::Other);
  RejectOperation(ISD::STACKRESTORE, MVT::Other);
}

SDValue MMIXTargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  report_fatal_error(
      Twine("MMIX SelectionDAG operation is not implemented by this lowering "
            "stage: ") +
      Op->getOperationName(&DAG));
}

SDValue MMIXTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &, SelectionDAG &,
    SmallVectorImpl<SDValue> &) const {
  if (CallConv != CallingConv::C || IsVarArg || !Ins.empty())
    report_fatal_error(
        "MMIX formal argument lowering is not implemented by this stage");
  return Chain;
}
