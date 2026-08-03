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
#include "llvm/Target/TargetMachine.h"
#include <limits>

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
  setMinimumJumpTableEntries(std::numeric_limits<unsigned>::max());

  auto RejectOperation = [this](unsigned Opcode, MVT VT) {
    setOperationAction(Opcode, VT, Custom);
  };

  // Only i64 and f64 have ABI register classes. Type legalization promotes
  // narrower scalar values and splits wider scalar and vector values. Keep
  // legal-width operations unavailable until their dedicated lowering task
  // defines the exact MMIX semantics.
  static constexpr unsigned IntegerOperations[] = {
      ISD::ROTL, ISD::ROTR,  ISD::BSWAP,  ISD::CTPOP,    ISD::CTLZ,
      ISD::CTTZ};
  for (unsigned Opcode : IntegerOperations)
    RejectOperation(Opcode, MVT::i64);

  setOperationAction(ISD::SETCC, MVT::i64, Legal);
  setOperationAction(ISD::SELECT, MVT::i64, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::i64, Expand);

  setOperationAction(ISD::MULHU, MVT::i64, Expand);
  setOperationAction(ISD::MULHS, MVT::i64, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i64, Custom);
  setOperationAction(ISD::SMUL_LOHI, MVT::i64, Custom);
  setOperationAction(ISD::UMULO, MVT::i64, Expand);
  setOperationAction(ISD::SMULO, MVT::i64, Expand);

  // MMIX's divide-by-zero and INT_MIN/-1 exceptional cases correspond to
  // poison-producing LLVM inputs. All defined signed inputs need the quotient
  // and remainder correction implemented by the custom SDIVREM lowering.
  for (unsigned Opcode : {ISD::SDIV, ISD::UDIV, ISD::SREM, ISD::UREM})
    setOperationAction(Opcode, MVT::i64, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i64, Custom);
  setOperationAction(ISD::UDIVREM, MVT::i64, Custom);

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

  static constexpr unsigned SymbolicAddressOperations[] = {
      ISD::GlobalAddress, ISD::ExternalSymbol, ISD::BlockAddress,
      ISD::ConstantPool, ISD::JumpTable};
  for (unsigned Opcode : SymbolicAddressOperations)
    setOperationAction(Opcode, MVT::i64, Custom);

  static constexpr unsigned AddressOperations[] = {
      ISD::FRAMEADDR, ISD::RETURNADDR, ISD::DYNAMIC_STACKALLOC};
  for (unsigned Opcode : AddressOperations)
    RejectOperation(Opcode, MVT::i64);

  setOperationAction(ISD::FrameIndex, MVT::i64, Legal);
  setOperationAction(ISD::LOAD, MVT::i64, Legal);
  setOperationAction(ISD::STORE, MVT::i64, Legal);
  for (MVT MemVT : {MVT::i8, MVT::i16, MVT::i32}) {
    setLoadExtAction(ISD::EXTLOAD, MVT::i64, MemVT, Legal);
    setLoadExtAction(ISD::SEXTLOAD, MVT::i64, MemVT, Legal);
    setLoadExtAction(ISD::ZEXTLOAD, MVT::i64, MemVT, Legal);
    setTruncStoreAction(MVT::i64, MemVT, Legal);
  }
  RejectOperation(ISD::LOAD, MVT::f64);
  RejectOperation(ISD::STORE, MVT::f64);
  setOperationAction(ISD::BR_CC, MVT::i64, Expand);
  RejectOperation(ISD::BR_CC, MVT::f64);
  setOperationAction(ISD::BRCOND, MVT::Other, Legal);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  RejectOperation(ISD::STACKSAVE, MVT::Other);
  RejectOperation(ISD::STACKRESTORE, MVT::Other);
}

bool MMIXTargetLowering::allowsMisalignedMemoryAccesses(
    EVT, unsigned, Align, MachineMemOperand::Flags, unsigned *) const {
  // MMIX rounds a misaligned multi-byte address down instead of performing
  // the byte sequence required by LLVM semantics. Let SelectionDAG expand it.
  return false;
}

SDValue MMIXTargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::GlobalAddress:
  case ISD::ExternalSymbol:
  case ISD::BlockAddress:
  case ISD::ConstantPool:
  case ISD::JumpTable:
  case ISD::UMUL_LOHI:
  case ISD::SMUL_LOHI:
  case ISD::UDIVREM:
  case ISD::SDIVREM:
    break;
  default:
    report_fatal_error(
        Twine("MMIX SelectionDAG operation is not implemented by this "
              "lowering stage: ") +
        Op->getOperationName(&DAG));
  }

  SDLoc DL(Op);
  if (Op.getOpcode() == ISD::GlobalAddress ||
      Op.getOpcode() == ISD::ExternalSymbol ||
      Op.getOpcode() == ISD::BlockAddress ||
      Op.getOpcode() == ISD::ConstantPool || Op.getOpcode() == ISD::JumpTable) {
    if (getTargetMachine().getRelocationModel() != Reloc::Static)
      report_fatal_error(
          "MMIX symbolic addresses require the static relocation model");

    SDValue Target;
    if (auto *GA = dyn_cast<GlobalAddressSDNode>(Op))
      Target = DAG.getTargetGlobalAddress(GA->getGlobal(), DL, MVT::i64,
                                          GA->getOffset());
    else if (auto *ES = dyn_cast<ExternalSymbolSDNode>(Op))
      Target = DAG.getTargetExternalSymbol(ES->getSymbol(), MVT::i64);
    else if (auto *BA = dyn_cast<BlockAddressSDNode>(Op))
      Target = DAG.getTargetBlockAddress(BA->getBlockAddress(), MVT::i64,
                                         BA->getOffset());
    else if (auto *CP = dyn_cast<ConstantPoolSDNode>(Op)) {
      if (CP->isMachineConstantPoolEntry())
        Target = DAG.getTargetConstantPool(CP->getMachineCPVal(), MVT::i64,
                                           CP->getAlign(), CP->getOffset());
      else
        Target = DAG.getTargetConstantPool(CP->getConstVal(), MVT::i64,
                                           CP->getAlign(), CP->getOffset());
    } else {
      auto *JT = cast<JumpTableSDNode>(Op);
      Target = DAG.getTargetJumpTable(JT->getIndex(), MVT::i64);
    }
    return DAG.getNode(MMIXISD::LOAD_ADDR, DL, MVT::i64, Target);
  }

  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDVTList PairVTs = DAG.getVTList(MVT::i64, MVT::i64);

  switch (Op.getOpcode()) {
  case ISD::UMUL_LOHI:
    return DAG.getNode(MMIXISD::UMUL_LOHI, DL, PairVTs, LHS, RHS);
  case ISD::SMUL_LOHI: {
    SDValue Product = DAG.getNode(MMIXISD::UMUL_LOHI, DL, PairVTs, LHS, RHS);
    SDValue Shift = DAG.getConstant(63, DL, MVT::i64);
    SDValue LHSMask = DAG.getNode(ISD::SRA, DL, MVT::i64, LHS, Shift);
    SDValue RHSMask = DAG.getNode(ISD::SRA, DL, MVT::i64, RHS, Shift);
    SDValue LHSCorrection = DAG.getNode(ISD::AND, DL, MVT::i64, LHSMask, RHS);
    SDValue RHSCorrection = DAG.getNode(ISD::AND, DL, MVT::i64, RHSMask, LHS);
    SDValue High =
        DAG.getNode(ISD::SUB, DL, MVT::i64, Product.getValue(1), LHSCorrection);
    High = DAG.getNode(ISD::SUB, DL, MVT::i64, High, RHSCorrection);
    return DAG.getMergeValues({Product, High}, DL);
  }
  case ISD::UDIVREM:
    return DAG.getNode(MMIXISD::UDIVREM, DL, PairVTs, LHS, RHS);
  case ISD::SDIVREM: {
    SDValue Floor = DAG.getNode(MMIXISD::SDIVREM, DL, PairVTs, LHS, RHS);
    SDValue Zero = DAG.getConstant(0, DL, MVT::i64);
    SDValue Shift = DAG.getConstant(63, DL, MVT::i64);
    SDValue SignDifference = DAG.getNode(ISD::XOR, DL, MVT::i64, LHS, RHS);
    SignDifference = DAG.getNode(ISD::SRA, DL, MVT::i64, SignDifference, Shift);
    SDValue NegativeRemainder =
        DAG.getNode(ISD::SUB, DL, MVT::i64, Zero, Floor.getValue(1));
    SDValue NonZeroRemainder = DAG.getNode(
        ISD::OR, DL, MVT::i64, Floor.getValue(1), NegativeRemainder);
    NonZeroRemainder =
        DAG.getNode(ISD::SRA, DL, MVT::i64, NonZeroRemainder, Shift);
    SDValue Correction =
        DAG.getNode(ISD::AND, DL, MVT::i64, SignDifference, NonZeroRemainder);
    SDValue Quotient = DAG.getNode(ISD::SUB, DL, MVT::i64, Floor, Correction);
    SDValue RemainderCorrection =
        DAG.getNode(ISD::AND, DL, MVT::i64, RHS, Correction);
    SDValue Remainder = DAG.getNode(ISD::SUB, DL, MVT::i64, Floor.getValue(1),
                                    RemainderCorrection);
    return DAG.getMergeValues({Quotient, Remainder}, DL);
  }
  default:
    llvm_unreachable("unexpected custom MMIX operation");
  }
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
