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
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_CALLING_CONV_IMPL
#include "MMIXGenCallingConv.inc"

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
      ISD::MUL,  ISD::SDIV, ISD::UDIV,  ISD::SREM,   ISD::UREM,
      ISD::AND,  ISD::OR,   ISD::XOR,   ISD::SHL,    ISD::SRA,
      ISD::SRL,  ISD::ROTL, ISD::ROTR,  ISD::BSWAP,  ISD::CTPOP,
      ISD::CTLZ, ISD::CTTZ, ISD::SETCC, ISD::SELECT, ISD::SELECT_CC};
  for (unsigned Opcode : IntegerOperations)
    RejectOperation(Opcode, MVT::i64);

  // Overflow-producing nodes must be expanded rather than selected as MMIX's
  // trapping signed arithmetic instructions. LLVM's nsw flag is poison
  // semantics and likewise does not authorize a hardware trap.
  for (unsigned Opcode : {ISD::SADDO, ISD::UADDO, ISD::SSUBO, ISD::USUBO})
    setOperationAction(Opcode, MVT::i64, Expand);

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

static bool hasUnsupportedArgumentFlags(const ISD::ArgFlagsTy &Flags) {
  return Flags.isInReg() || Flags.isSRet() || Flags.isByVal() ||
         Flags.isByRef() || Flags.isNest() || Flags.isInAlloca() ||
         Flags.isPreallocated() || Flags.isSwiftSelf() ||
         Flags.isSwiftAsync() || Flags.isSwiftError() ||
         Flags.isCFGuardTarget() || Flags.isHva() || Flags.isHvaStart() ||
         Flags.isSecArgPass() || Flags.isInConsecutiveRegs() ||
         Flags.isCopyElisionCandidate() || Flags.isSplit();
}

SDValue MMIXTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (CallConv != CallingConv::C)
    report_fatal_error("MMIX supports only the C calling convention");
  if (IsVarArg)
    report_fatal_error("MMIX does not support variadic functions");
  for (const ISD::InputArg &Arg : Ins) {
    if (hasUnsupportedArgumentFlags(Arg.Flags))
      report_fatal_error(
          "MMIX does not support aggregate or special formal arguments");
    if (Arg.Flags.isPointer() && Arg.Flags.getPointerAddrSpace() != 0)
      report_fatal_error(
          "MMIX does not support nonzero-address-space formal arguments");
  }

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_MMIX);

  for (const CCValAssign &VA : ArgLocs) {
    SDValue Arg;
    if (VA.isRegLoc()) {
      Register VReg = MRI.createVirtualRegister(getRegClassFor(VA.getLocVT()));
      MRI.addLiveIn(VA.getLocReg(), VReg);
      Arg = DAG.getCopyFromReg(Chain, DL, VReg, VA.getLocVT());
    } else {
      int FI = MFI.CreateFixedObject(8, VA.getLocMemOffset(), true);
      SDValue FrameIndex = DAG.getTargetFrameIndex(FI, MVT::i64);
      SDValue StackArg = DAG.getNode(MMIXISD::LOAD_STACK_ARG, DL,
                                     DAG.getVTList(VA.getLocVT(), MVT::Other),
                                     Chain, FrameIndex);
      Arg = StackArg;
    }

    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::AssertSext, DL, VA.getLocVT(), Arg,
                        DAG.getValueType(VA.getValVT()));
      Arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::AssertZext, DL, VA.getLocVT(), Arg,
                        DAG.getValueType(VA.getValVT()));
      Arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Arg);
      break;
    case CCValAssign::BCvt:
      Arg = DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), Arg);
      break;
    default:
      report_fatal_error("MMIX does not support this argument extension");
    }
    InVals.push_back(Arg);
  }

  return Chain;
}

bool MMIXTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  bool SupportedType =
      RetTy->isVoidTy() ||
      (RetTy->isIntegerTy() && RetTy->getIntegerBitWidth() <= 64) ||
      (RetTy->isPointerTy() && RetTy->getPointerAddressSpace() == 0) ||
      RetTy->isFloatTy() || RetTy->isDoubleTy();
  if (CallConv != CallingConv::C || IsVarArg || Outs.size() > 1 ||
      !SupportedType)
    return false;

  SmallVector<CCValAssign, 1> RetLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RetLocs, Context);
  return CCInfo.CheckReturn(Outs, RetCC_MMIX);
}

SDValue
MMIXTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                bool IsVarArg,
                                const SmallVectorImpl<ISD::OutputArg> &Outs,
                                const SmallVectorImpl<SDValue> &OutVals,
                                const SDLoc &DL, SelectionDAG &DAG) const {
  if (CallConv != CallingConv::C)
    report_fatal_error("MMIX supports only the C calling convention");
  if (IsVarArg)
    report_fatal_error("MMIX does not support variadic functions");

  SmallVector<CCValAssign, 1> RetLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RetLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_MMIX);

  SDValue Glue;
  SmallVector<SDValue, 2> RetOps(1, Chain);
  for (unsigned I = 0; I != RetLocs.size(); ++I) {
    const CCValAssign &VA = RetLocs[I];
    SDValue Val = OutVals[I];
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Val = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Val);
      break;
    case CCValAssign::ZExt:
      Val = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Val);
      break;
    case CCValAssign::AExt:
      Val = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Val);
      break;
    case CCValAssign::BCvt:
      Val = DAG.getNode(ISD::BITCAST, DL, VA.getLocVT(), Val);
      break;
    default:
      report_fatal_error("MMIX does not support this return extension");
    }

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Val, Glue);
    Glue = Chain.getValue(1);
  }

  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  unsigned Opcode =
      RetLocs.empty() ? MMIXISD::RET_GLUE : MMIXISD::RET_VALUE_GLUE;
  return DAG.getNode(Opcode, DL, MVT::Other, RetOps);
}
