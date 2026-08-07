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
#include "llvm/IR/Constants.h"
#include "llvm/IR/IntrinsicsMMIX.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"
#include <limits>
#include <string>

using namespace llvm;

#define GET_CALLING_CONV_IMPL
#include "MMIXGenCallingConv.inc"

#define GET_REGISTER_MATCHER
#include "MMIXGenAsmMatcher.inc"

static bool isMMIXIntegerInlineAsmConstraint(char Constraint) {
  switch (Constraint) {
  case 'I':
  case 'J':
  case 'K':
  case 'M':
  case 'O':
    return true;
  default:
    return false;
  }
}

static bool isMMIXInlineAsmImmediate(char Constraint, int64_t Value) {
  switch (Constraint) {
  case 'I':
    return Value >= 0 && isUInt<8>(uint64_t(Value));
  case 'J':
    return Value >= 0 && isUInt<16>(uint64_t(Value));
  case 'K':
    return Value >= -255 && Value <= 0;
  case 'M':
    return Value == 0;
  case 'O':
    return Value == 3 || Value == 5 || Value == 9 || Value == 17;
  default:
    return false;
  }
}

static bool isUnsafeInlineAsmRegister(MCRegister Reg) {
  switch (Reg.id()) {
  case MMIX::R30:  // Holds the incoming rJ in non-leaf functions.
  case MMIX::R253: // Frame pointer.
  case MMIX::R254: // Stack pointer.
  case MMIX::RA:   // Floating environment.
  case MMIX::RG:   // Global-register threshold.
  case MMIX::RJ:   // Return address.
  case MMIX::RL:   // Local-register count.
  case MMIX::RN:   // Register-stack serial number.
  case MMIX::RO:   // Register-stack offset.
  case MMIX::RS:   // Register-stack pointer.
    return true;
  default:
    return false;
  }
}

static MCRegister getMMIXSpecialRegister(unsigned Selector,
                                         const TargetRegisterInfo &TRI) {
  for (MCPhysReg Reg : MMIX::SPR64RegClass)
    if (TRI.getEncodingValue(Reg) == Selector)
      return Reg;
  llvm_unreachable("missing MMIX special-register definition");
}

static bool isMMIXSystemSpecialRegister(MCRegister Reg) {
  switch (Reg.id()) {
  case MMIX::RC:
  case MMIX::RI:
  case MMIX::RT:
  case MMIX::RTT:
  case MMIX::RK:
  case MMIX::RQ:
  case MMIX::RU:
    return true;
  default:
    return false;
  }
}

static SDValue emitMMIXIntrinsicError(SDValue Op, StringRef IntrinsicName,
                                      const Twine &Message, SelectionDAG &DAG) {
  DAG.getContext()->emitError(Twine(IntrinsicName) + ": " + Message);
  if (Op.getOpcode() == ISD::INTRINSIC_W_CHAIN)
    return DAG.getMergeValues(
        {DAG.getUNDEF(Op.getValueType()), Op.getOperand(0)}, SDLoc(Op));
  return Op.getOperand(0);
}

static SDValue lowerMMIXSpecialRegisterIntrinsic(SDValue Op,
                                                 SelectionDAG &DAG) {
  bool IsGet = Op.getOpcode() == ISD::INTRINSIC_W_CHAIN;
  StringRef IntrinsicName = IsGet ? "llvm.mmix.get" : "llvm.mmix.put";
  auto *SelectorNode = dyn_cast<ConstantSDNode>(Op.getOperand(2));
  if (!SelectorNode)
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  "selector must be an immediate", DAG);

  uint64_t Selector = SelectorNode->getZExtValue();
  if (!isUInt<5>(Selector))
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  "selector must be in the range [0, 31]", DAG);

  const MMIXSubtarget &STI =
      DAG.getMachineFunction().getSubtarget<MMIXSubtarget>();
  const TargetRegisterInfo &TRI = *STI.getRegisterInfo();
  MCRegister Reg = getMMIXSpecialRegister(Selector, TRI);
  SDLoc DL(Op);
  SDValue SpecialReg = DAG.getRegister(Reg, MVT::i64);
  if (IsGet)
    return DAG.getNode(MMIXISD::GET_SPECIAL_REGISTER, DL,
                       {MVT::i64, MVT::Other}, {Op.getOperand(0), SpecialReg});

  std::string RegName = TRI.getName(Reg);
  RegName.front() = 'r';
  switch (Reg.id()) {
  case MMIX::RN:
  case MMIX::RO:
  case MMIX::RS:
    return emitMMIXIntrinsicError(
        Op, IntrinsicName,
        Twine("register '") + RegName + "' is architecturally read-only", DAG);
  case MMIX::RJ:
  case MMIX::RG:
  case MMIX::RL:
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  Twine("register '") + RegName +
                                      "' is reserved by the provisional ABI",
                                  DAG);
  case MMIX::RA:
    return emitMMIXIntrinsicError(
        Op, IntrinsicName,
        "register 'rA' requires explicit floating-environment modeling", DAG);
  default:
    break;
  }

  if (isMMIXSystemSpecialRegister(Reg) && !STI.hasMMIXSystem())
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  Twine("register '") + RegName +
                                      "' requires the system target feature",
                                  DAG);
  if (Reg == MMIX::RV && !STI.hasMMIXVirtualMemory())
    return emitMMIXIntrinsicError(
        Op, IntrinsicName,
        "register 'rV' requires the virtual-memory target feature", DAG);

  return DAG.getNode(MMIXISD::PUT_SPECIAL_REGISTER, DL, MVT::Other,
                     {Op.getOperand(0), SpecialReg, Op.getOperand(3),
                      DAG.getConstant(Selector, DL, MVT::i64)});
}

static SDValue lowerMMIXCacheIntrinsic(SDValue Op, unsigned IntrinsicID,
                                       SelectionDAG &DAG) {
  const MMIXSubtarget &STI =
      DAG.getMachineFunction().getSubtarget<MMIXSubtarget>();
  StringRef IntrinsicName;
  unsigned Operation;
  switch (IntrinsicID) {
  case Intrinsic::mmix_preld:
    IntrinsicName = "llvm.mmix.preld";
    Operation = MMIXISD::CachePreload;
    break;
  case Intrinsic::mmix_prego:
    IntrinsicName = "llvm.mmix.prego";
    Operation = MMIXISD::CachePrefetchForExecution;
    break;
  case Intrinsic::mmix_prest:
    IntrinsicName = "llvm.mmix.prest";
    Operation = MMIXISD::CachePrestore;
    break;
  case Intrinsic::mmix_syncd:
    IntrinsicName = "llvm.mmix.syncd";
    Operation = MMIXISD::CacheSyncData;
    break;
  case Intrinsic::mmix_syncid:
    IntrinsicName = "llvm.mmix.syncid";
    Operation = MMIXISD::CacheSyncInstructionAndData;
    break;
  default:
    llvm_unreachable("not an MMIX cache intrinsic");
  }

  if (!STI.hasMMIXCache())
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  "requires the cache target feature", DAG);

  auto *SpanNode = dyn_cast<ConstantSDNode>(Op.getOperand(3));
  if (!SpanNode)
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  "span must be an immediate", DAG);
  uint64_t Span = SpanNode->getZExtValue();
  if (!isUInt<8>(Span))
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  "span must be in the range [0, 255]", DAG);

  SDLoc DL(Op);
  return DAG.getNode(MMIXISD::CACHE_OPERATION, DL, MVT::Other,
                     {Op.getOperand(0), Op.getOperand(2),
                      DAG.getConstant(Span, DL, MVT::i64),
                      DAG.getConstant(Operation, DL, MVT::i64)});
}

static SDValue lowerMMIXSyncIntrinsic(SDValue Op, SelectionDAG &DAG) {
  const MMIXSubtarget &STI =
      DAG.getMachineFunction().getSubtarget<MMIXSubtarget>();
  if (!STI.hasMMIXSystem())
    return emitMMIXIntrinsicError(Op, "llvm.mmix.sync",
                                  "requires the system target feature", DAG);

  auto *ModeNode = dyn_cast<ConstantSDNode>(Op.getOperand(2));
  if (!ModeNode)
    return emitMMIXIntrinsicError(Op, "llvm.mmix.sync",
                                  "mode must be an immediate", DAG);
  uint64_t Mode = ModeNode->getZExtValue();
  if (!isUInt<3>(Mode))
    return emitMMIXIntrinsicError(Op, "llvm.mmix.sync",
                                  "mode must be in the range [0, 7]", DAG);

  SDLoc DL(Op);
  return DAG.getNode(MMIXISD::SYNC, DL, MVT::Other,
                     {Op.getOperand(0), DAG.getConstant(Mode, DL, MVT::i64)});
}

static SDValue lowerMMIXUncachedMemoryIntrinsic(SDValue Op,
                                                unsigned IntrinsicID,
                                                SelectionDAG &DAG) {
  const MMIXSubtarget &STI =
      DAG.getMachineFunction().getSubtarget<MMIXSubtarget>();
  bool IsLoad = IntrinsicID == Intrinsic::mmix_ldunc;
  StringRef IntrinsicName = IsLoad ? "llvm.mmix.ldunc" : "llvm.mmix.stunc";
  if (!STI.hasMMIXCache())
    return emitMMIXIntrinsicError(Op, IntrinsicName,
                                  "requires the cache target feature", DAG);

  auto *Mem = cast<MemIntrinsicSDNode>(Op);
  SDLoc DL(Op);
  if (IsLoad)
    return DAG.getMemIntrinsicNode(MMIXISD::UNCACHED_LOAD, DL, Op->getVTList(),
                                   {Op.getOperand(0), Op.getOperand(2)},
                                   Mem->getMemoryVT(), Mem->getMemOperand());

  return DAG.getMemIntrinsicNode(
      MMIXISD::UNCACHED_STORE, DL, Op->getVTList(),
      {Op.getOperand(0), Op.getOperand(2), Op.getOperand(3)},
      Mem->getMemoryVT(), Mem->getMemOperand());
}

static SDValue lowerMMIXVirtualTranslationIntrinsic(SDValue Op,
                                                    SelectionDAG &DAG) {
  const MMIXSubtarget &STI =
      DAG.getMachineFunction().getSubtarget<MMIXSubtarget>();
  if (!STI.hasMMIXVirtualMemory())
    return emitMMIXIntrinsicError(Op, "llvm.mmix.ldvts",
                                  "requires the virtual-memory target feature",
                                  DAG);

  return DAG.getNode(MMIXISD::VIRTUAL_TRANSLATION_SEARCH, SDLoc(Op),
                     {MVT::i64, MVT::Other},
                     {Op.getOperand(0), Op.getOperand(2)});
}

MMIXTargetLowering::AsmOperandInfoVector
MMIXTargetLowering::ParseConstraints(const DataLayout &DL,
                                     const TargetRegisterInfo *TRI,
                                     const CallBase &Call) const {
  AsmOperandInfoVector Operands =
      TargetLowering::ParseConstraints(DL, TRI, Call);
  for (const AsmOperandInfo &Operand : Operands) {
    if (Operand.Type != InlineAsm::isClobber)
      continue;
    for (StringRef Code : Operand.Codes) {
      if (!Code.starts_with('{') || !Code.ends_with('}'))
        continue;
      StringRef Name = Code.drop_front().drop_back();
      MCRegister Reg = MatchRegisterName(Name);
      if (!Reg || !isUnsafeInlineAsmRegister(Reg))
        continue;
      Call.getContext().emitError(
          &Call, Twine("MMIX inline assembly may not clobber register '") +
                     Name + "' in an ordinary function");
      return {};
    }
  }
  return Operands;
}

MMIXTargetLowering::ConstraintType
MMIXTargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint.size() == 1) {
    if (isMMIXIntegerInlineAsmConstraint(Constraint[0]))
      return C_Immediate;
    switch (Constraint[0]) {
    case 'G':
      return C_Other;
    case 'm':
    case 'o':
    case 'V':
    case 'p':
      return C_Unknown;
    default:
      break;
    }
  }
  return TargetLowering::getConstraintType(Constraint);
}

MMIXTargetLowering::ConstraintWeight
MMIXTargetLowering::getSingleConstraintMatchWeight(
    AsmOperandInfo &Info, const char *Constraint) const {
  if (!Info.CallOperandVal)
    return CW_Default;

  if (Constraint[1] != '\0')
    return TargetLowering::getSingleConstraintMatchWeight(Info, Constraint);

  if (Constraint[0] == 'r') {
    Type *Ty = Info.CallOperandVal->getType();
    return Ty->isIntegerTy() || Ty->isPointerTy() || Ty->isDoubleTy()
               ? CW_Register
               : CW_Invalid;
  }

  if (Constraint[0] == 'G') {
    const auto *C = dyn_cast<ConstantFP>(Info.CallOperandVal);
    return C && C->isZero() ? CW_Constant : CW_Invalid;
  }

  const auto *C = dyn_cast<ConstantInt>(Info.CallOperandVal);
  if (!C)
    return TargetLowering::getSingleConstraintMatchWeight(Info, Constraint);

  if (!C->getValue().isSignedIntN(64))
    return CW_Invalid;
  if (!isMMIXIntegerInlineAsmConstraint(Constraint[0]))
    return TargetLowering::getSingleConstraintMatchWeight(Info, Constraint);
  return isMMIXInlineAsmImmediate(Constraint[0], C->getSExtValue())
             ? CW_Constant
             : CW_Invalid;
}

std::pair<unsigned, const TargetRegisterClass *>
MMIXTargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                                 StringRef Constraint,
                                                 MVT VT) const {
  if (Constraint == "r") {
    if (VT == MVT::f64)
      return {0, &MMIX::FPR64CodeGenRegClass};
    if (!VT.isVector())
      return {0, &MMIX::GPR64CodeGenRegClass};
    return {0, nullptr};
  }

  if (!Constraint.starts_with('{') || !Constraint.ends_with('}'))
    return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);

  MCRegister Reg = MatchRegisterName(Constraint.drop_front().drop_back());
  if (!Reg || isUnsafeInlineAsmRegister(Reg))
    return {0, nullptr};

  if (MMIX::GPR64CodeGenRegClass.contains(Reg)) {
    const TargetRegisterClass *RC = VT == MVT::f64
                                        ? &MMIX::FPR64CodeGenRegClass
                                        : &MMIX::GPR64CodeGenRegClass;
    return {Reg.id(), RC};
  }

  // Reserved architectural and special registers are valid clobber names, but
  // cannot carry values without changing the provisional register-allocation
  // or special-register state contract.
  if (VT != MVT::Other)
    return {0, nullptr};
  if (MMIX::GPR64RegClass.contains(Reg))
    return {Reg.id(), &MMIX::GPR64RegClass};
  if (MMIX::SPR64RegClass.contains(Reg))
    return {Reg.id(), &MMIX::SPR64RegClass};
  return {0, nullptr};
}

void MMIXTargetLowering::LowerAsmOperandForConstraint(SDValue Op,
                                                      StringRef Constraint,
                                                      std::vector<SDValue> &Ops,
                                                      SelectionDAG &DAG) const {
  if (Constraint.size() != 1)
    return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops,
                                                        DAG);

  if (Constraint[0] == 'G') {
    if (const auto *C = dyn_cast<ConstantFPSDNode>(Op); C && C->isZero())
      Ops.push_back(DAG.getTargetConstant(0, SDLoc(Op), MVT::i64));
    return;
  }

  const auto *C = dyn_cast<ConstantSDNode>(Op);
  if (!C)
    return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops,
                                                        DAG);

  if (!isMMIXIntegerInlineAsmConstraint(Constraint[0]))
    return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops,
                                                        DAG);

  int64_t Value = C->getSExtValue();
  if (isMMIXInlineAsmImmediate(Constraint[0], Value))
    Ops.push_back(DAG.getSignedTargetConstant(Value, SDLoc(Op), MVT::i64));
}

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
  setMaxAtomicSizeInBitsSupported(64);
  setMinCmpXchgSizeInBits(64);
  setMinimumJumpTableEntries(std::numeric_limits<unsigned>::max());
  setTargetDAGCombine(ISD::STORE);

  auto RejectOperation = [this](unsigned Opcode, MVT VT) {
    setOperationAction(Opcode, VT, Custom);
  };

  // Only i64 and f64 have ABI register classes. Type legalization promotes
  // narrower scalar values and splits wider scalar and vector values. Keep
  // legal-width operations unavailable until their dedicated lowering task
  // defines the exact MMIX semantics.
  static constexpr unsigned IntegerOperations[] = {
      ISD::ROTL, ISD::ROTR, ISD::BSWAP, ISD::CTLZ, ISD::CTTZ};
  for (unsigned Opcode : IntegerOperations)
    RejectOperation(Opcode, MVT::i64);

  setOperationAction(ISD::CTPOP, MVT::i64, Legal);
  setOperationAction(ISD::USUBSAT, MVT::i64, Legal);

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

  for (unsigned Opcode :
       {ISD::FADD, ISD::FSUB, ISD::FMUL, ISD::FDIV, ISD::FSQRT})
    setOperationAction(Opcode, MVT::f64, Legal);
  setOperationAction(ISD::ConstantFP, MVT::f64, Legal);
  setOperationAction(ISD::SETCC, MVT::f64, Custom);

  for (unsigned Opcode : {ISD::SINT_TO_FP, ISD::UINT_TO_FP})
    setOperationAction(Opcode, MVT::i64, Legal);
  for (unsigned Opcode : {ISD::FP_TO_SINT, ISD::FP_TO_UINT})
    setOperationAction(Opcode, MVT::i64, Legal);
  for (unsigned Opcode :
       {ISD::FTRUNC, ISD::FCEIL, ISD::FFLOOR, ISD::FROUNDEVEN, ISD::FRINT})
    setOperationAction(Opcode, MVT::f64, Legal);

  static constexpr unsigned UnsupportedFloatingOperations[] = {
      ISD::FREM,   ISD::FNEG,      ISD::FABS,   ISD::FCOPYSIGN, ISD::FMA,
      ISD::SELECT, ISD::SELECT_CC, ISD::FROUND, ISD::FNEARBYINT};
  for (unsigned Opcode : UnsupportedFloatingOperations)
    RejectOperation(Opcode, MVT::f64);

  static constexpr unsigned StrictFloatingOperations[] = {
      ISD::STRICT_FADD,       ISD::STRICT_FSUB,       ISD::STRICT_FMUL,
      ISD::STRICT_FDIV,       ISD::STRICT_FREM,       ISD::STRICT_FSQRT,
      ISD::STRICT_FMA,        ISD::STRICT_FSETCC,     ISD::STRICT_FSETCCS,
      ISD::STRICT_FP_ROUND,   ISD::STRICT_FP_EXTEND,  ISD::STRICT_FTRUNC,
      ISD::STRICT_FCEIL,      ISD::STRICT_FFLOOR,     ISD::STRICT_FROUND,
      ISD::STRICT_FROUNDEVEN, ISD::STRICT_FNEARBYINT, ISD::STRICT_FRINT};
  for (unsigned Opcode : StrictFloatingOperations)
    RejectOperation(Opcode, MVT::f64);
  for (unsigned Opcode : {ISD::STRICT_SINT_TO_FP, ISD::STRICT_UINT_TO_FP,
                          ISD::STRICT_FP_TO_SINT, ISD::STRICT_FP_TO_UINT})
    RejectOperation(Opcode, MVT::i64);

  static constexpr unsigned SymbolicAddressOperations[] = {
      ISD::GlobalAddress, ISD::ExternalSymbol, ISD::BlockAddress,
      ISD::ConstantPool, ISD::JumpTable, ISD::GlobalTLSAddress};
  for (unsigned Opcode : SymbolicAddressOperations)
    setOperationAction(Opcode, MVT::i64, Custom);

  static constexpr unsigned AddressOperations[] = {
      ISD::FRAMEADDR, ISD::RETURNADDR, ISD::DYNAMIC_STACKALLOC,
      ISD::ADDRSPACECAST};
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
  setOperationAction(ISD::LOAD, MVT::f64, Legal);
  setOperationAction(ISD::STORE, MVT::f64, Legal);
  setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, Legal);
  setTruncStoreAction(MVT::f64, MVT::f32, Legal);
  setOperationAction(ISD::BR_CC, MVT::i64, Expand);
  setOperationAction(ISD::BR_CC, MVT::f64, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Legal);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  RejectOperation(ISD::BRIND, MVT::Other);
  RejectOperation(ISD::TRAP, MVT::Other);
  setOperationAction(ISD::ATOMIC_CMP_SWAP, MVT::i64, Legal);
  setOperationAction(ISD::ATOMIC_CMP_SWAP_WITH_SUCCESS, MVT::i64, Expand);
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Legal);
  setOperationAction(ISD::INTRINSIC_W_CHAIN, MVT::i64, Custom);
  setOperationAction(ISD::INTRINSIC_W_CHAIN, MVT::Other, Custom);
  setOperationAction(ISD::INTRINSIC_VOID, MVT::Other, Custom);
  RejectOperation(ISD::STACKSAVE, MVT::Other);
  RejectOperation(ISD::STACKRESTORE, MVT::Other);
}

bool MMIXTargetLowering::allowsMisalignedMemoryAccesses(
    EVT, unsigned, Align, MachineMemOperand::Flags, unsigned *) const {
  // MMIX rounds a misaligned multi-byte address down instead of performing
  // the byte sequence required by LLVM semantics. Let SelectionDAG expand it.
  return false;
}

void MMIXTargetLowering::getTgtMemIntrinsic(
    SmallVectorImpl<IntrinsicInfo> &Infos, const CallBase &I, MachineFunction &,
    unsigned IntrinsicID) const {
  IntrinsicInfo Info;
  switch (IntrinsicID) {
  case Intrinsic::mmix_ldunc:
    Info.opc = ISD::INTRINSIC_W_CHAIN;
    Info.flags = MachineMemOperand::MOLoad;
    break;
  case Intrinsic::mmix_stunc:
    Info.opc = ISD::INTRINSIC_VOID;
    Info.flags = MachineMemOperand::MOStore;
    break;
  default:
    return;
  }

  Info.memVT = MVT::i64;
  Info.ptrVal = I.getArgOperand(0);
  Info.size = 8;
  Info.align = Align(8);
  Infos.push_back(Info);
}

SDValue MMIXTargetLowering::PerformDAGCombine(SDNode *N,
                                              DAGCombinerInfo &DCI) const {
  auto *Store = dyn_cast<StoreSDNode>(N);
  if (!Store || Store->isTruncatingStore() || Store->isIndexed() ||
      Store->getMemoryVT() != MVT::f32)
    return SDValue();

  SDValue Value = Store->getValue();
  unsigned ConvertOpcode = Value.getOpcode();
  if (ConvertOpcode != ISD::SINT_TO_FP && ConvertOpcode != ISD::UINT_TO_FP)
    return SDValue();

  SDValue Integer = Value.getOperand(0);
  EVT IntegerVT = Integer.getValueType();
  if (!IntegerVT.isInteger() || IntegerVT.isVector() ||
      IntegerVT.getSizeInBits() > 64)
    return SDValue();

  SelectionDAG &DAG = DCI.DAG;
  if (IntegerVT != MVT::i64) {
    unsigned ExtendOpcode =
        ConvertOpcode == ISD::SINT_TO_FP ? ISD::SIGN_EXTEND : ISD::ZERO_EXTEND;
    Integer = DAG.getNode(ExtendOpcode, SDLoc(Integer), MVT::i64, Integer);
  }
  unsigned Opcode =
      ConvertOpcode == ISD::SINT_TO_FP ? MMIXISD::SFLOT : MMIXISD::SFLOTU;
  SDValue Rounded = DAG.getNode(Opcode, SDLoc(Value), MVT::f64, Integer);
  return DAG.getTruncStore(Store->getChain(), SDLoc(Store), Rounded,
                           Store->getBasePtr(), MVT::f32,
                           Store->getMemOperand());
}

SDValue MMIXTargetLowering::LowerOperation(SDValue Op,
                                           SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::DYNAMIC_STACKALLOC)
    report_fatal_error("MMIX does not support dynamic stack allocation");
  if (Op.getOpcode() == ISD::ADDRSPACECAST)
    report_fatal_error("MMIX does not support nonzero address spaces");
  if (Op.getOpcode() == ISD::GlobalTLSAddress)
    report_fatal_error("MMIX does not support thread-local storage");
  if (Op.getOpcode() == ISD::BRIND)
    report_fatal_error(
        "MMIX does not support indirect branches in the provisional ABI");
  if (Op.getOpcode() == ISD::TRAP)
    report_fatal_error(
        "MMIX does not define an LLVM trap convention for this environment");
  if (Op.getOpcode() == ISD::FREM)
    report_fatal_error(
        "MMIX cannot directly lower LLVM frem: MMIX FREM implements IEEE "
        "remainder instead of truncating-quotient fmod semantics");
  if (Op.getOpcode() == ISD::FMA)
    report_fatal_error(
        "MMIX cannot lower fused f64 multiply-add without a runtime helper");
  if (Op.getOpcode() == ISD::FROUND)
    report_fatal_error(
        "MMIX cannot lower round-to-nearest-ties-away without a runtime "
        "helper");
  if (Op.getOpcode() == ISD::FNEARBYINT)
    report_fatal_error(
        "MMIX FINT cannot lower nearbyint because it may raise inexact");
  if (Op->isStrictFPOpcode())
    report_fatal_error(
        "MMIX constrained floating-point lowering is not implemented");

  switch (Op.getOpcode()) {
  case ISD::INTRINSIC_W_CHAIN:
  case ISD::INTRINSIC_VOID:
  case ISD::GlobalAddress:
  case ISD::ExternalSymbol:
  case ISD::BlockAddress:
  case ISD::ConstantPool:
  case ISD::JumpTable:
  case ISD::UMUL_LOHI:
  case ISD::SMUL_LOHI:
  case ISD::UDIVREM:
  case ISD::SDIVREM:
  case ISD::SETCC:
    break;
  default:
    report_fatal_error(
        Twine("MMIX SelectionDAG operation is not implemented by this "
              "lowering stage: ") +
        Op->getOperationName(&DAG));
  }

  if (Op.getOpcode() == ISD::INTRINSIC_W_CHAIN ||
      Op.getOpcode() == ISD::INTRINSIC_VOID) {
    unsigned IntrinsicID = Op.getConstantOperandVal(1);
    switch (IntrinsicID) {
    case Intrinsic::mmix_get:
    case Intrinsic::mmix_put:
      return lowerMMIXSpecialRegisterIntrinsic(Op, DAG);
    case Intrinsic::mmix_preld:
    case Intrinsic::mmix_prego:
    case Intrinsic::mmix_prest:
    case Intrinsic::mmix_syncd:
    case Intrinsic::mmix_syncid:
      return lowerMMIXCacheIntrinsic(Op, IntrinsicID, DAG);
    case Intrinsic::mmix_sync:
      return lowerMMIXSyncIntrinsic(Op, DAG);
    case Intrinsic::mmix_ldunc:
    case Intrinsic::mmix_stunc:
      return lowerMMIXUncachedMemoryIntrinsic(Op, IntrinsicID, DAG);
    case Intrinsic::mmix_ldvts:
      return lowerMMIXVirtualTranslationIntrinsic(Op, DAG);
    default:
      report_fatal_error("unsupported chained MMIX intrinsic");
    }
  }

  SDLoc DL(Op);
  if (Op.getOpcode() == ISD::SETCC) {
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
    EVT ResultVT = Op.getValueType();
    SDValue Zero = DAG.getConstant(0, DL, ResultVT);
    SDValue One = DAG.getConstant(1, DL, ResultVT);
    auto FPCompare = [&](unsigned Opcode) {
      return DAG.getNode(Opcode, DL, ResultVT, LHS, RHS);
    };
    auto CompareZero = [&](SDValue Value, ISD::CondCode IntCC) {
      return DAG.getSetCC(DL, ResultVT, Value, Zero, IntCC);
    };

    // FEQL and FUN are quiet even for signaling NaNs. FCMP provides ordering,
    // but raises invalid for NaN operands; constrained comparisons therefore
    // remain unsupported instead of silently changing exception behavior.
    switch (CC) {
    case ISD::SETFALSE:
    case ISD::SETFALSE2:
      return Zero;
    case ISD::SETOEQ:
    case ISD::SETEQ:
      return FPCompare(MMIXISD::FEQL);
    case ISD::SETOGT:
    case ISD::SETGT:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETGT);
    case ISD::SETOGE: {
      SDValue Ordered = CompareZero(FPCompare(MMIXISD::FUN), ISD::SETEQ);
      SDValue GE = CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETGE);
      return DAG.getNode(ISD::AND, DL, ResultVT, Ordered, GE);
    }
    case ISD::SETGE:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETGE);
    case ISD::SETOLT:
    case ISD::SETLT:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETLT);
    case ISD::SETOLE: {
      SDValue Ordered = CompareZero(FPCompare(MMIXISD::FUN), ISD::SETEQ);
      SDValue LE = CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETLE);
      return DAG.getNode(ISD::AND, DL, ResultVT, Ordered, LE);
    }
    case ISD::SETLE:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETLE);
    case ISD::SETONE:
    case ISD::SETNE:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETNE);
    case ISD::SETO:
      return CompareZero(FPCompare(MMIXISD::FUN), ISD::SETEQ);
    case ISD::SETUO:
      return FPCompare(MMIXISD::FUN);
    case ISD::SETUEQ:
      return DAG.getNode(ISD::OR, DL, ResultVT, FPCompare(MMIXISD::FEQL),
                         FPCompare(MMIXISD::FUN));
    case ISD::SETUGT: {
      SDValue GT = CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETGT);
      return DAG.getNode(ISD::OR, DL, ResultVT, GT,
                         FPCompare(MMIXISD::FUN));
    }
    case ISD::SETUGE:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETGE);
    case ISD::SETULT: {
      SDValue LT = CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETLT);
      return DAG.getNode(ISD::OR, DL, ResultVT, LT,
                         FPCompare(MMIXISD::FUN));
    }
    case ISD::SETULE:
      return CompareZero(FPCompare(MMIXISD::FCMP), ISD::SETLE);
    case ISD::SETUNE:
      return CompareZero(FPCompare(MMIXISD::FEQL), ISD::SETEQ);
    case ISD::SETTRUE:
    case ISD::SETTRUE2:
      return One;
    default:
      llvm_unreachable("unexpected floating-point condition code");
    }
  }

  if (Op.getOpcode() == ISD::GlobalAddress ||
      Op.getOpcode() == ISD::ExternalSymbol ||
      Op.getOpcode() == ISD::BlockAddress ||
      Op.getOpcode() == ISD::ConstantPool || Op.getOpcode() == ISD::JumpTable) {
    if (getTargetMachine().getRelocationModel() != Reloc::Static)
      report_fatal_error("MMIX supports only the static relocation model");

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
         Flags.isSecArgPass() || Flags.isReturned() ||
         Flags.isInConsecutiveRegs() || Flags.isCopyElisionCandidate() ||
         Flags.isSplit();
}

static bool isSupportedCallValueType(EVT VT) {
  return VT == MVT::i1 || VT == MVT::i8 || VT == MVT::i16 || VT == MVT::i32 ||
         VT == MVT::i64 || VT == MVT::f32 || VT == MVT::f64;
}

static SDValue convertOutgoingValue(SDValue Value, const CCValAssign &VA,
                                    const SDLoc &DL, SelectionDAG &DAG) {
  switch (VA.getLocInfo()) {
  case CCValAssign::Full:
    return Value;
  case CCValAssign::SExt:
    return DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Value);
  case CCValAssign::ZExt:
    return DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Value);
  case CCValAssign::AExt:
    return DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Value);
  case CCValAssign::BCvt:
    return DAG.getNode(ISD::BITCAST, DL, VA.getLocVT(), Value);
  default:
    report_fatal_error("MMIX does not support this call operand conversion");
  }
}

SDValue MMIXTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                      SmallVectorImpl<SDValue> &InVals) const {
  if (CLI.CallConv != CallingConv::C)
    report_fatal_error("MMIX supports only the C calling convention");
  if (CLI.IsVarArg)
    report_fatal_error("MMIX does not support variadic calls");
  if (CLI.OrigRetTy && CLI.OrigRetTy->isAggregateType())
    report_fatal_error("MMIX does not support aggregate call results");
  if (CLI.Outs.size() != CLI.OutVals.size())
    report_fatal_error("MMIX call operand lowering received mismatched values");

  for (const ISD::OutputArg &Arg : CLI.Outs) {
    if (!isSupportedCallValueType(Arg.VT) ||
        hasUnsupportedArgumentFlags(Arg.Flags))
      report_fatal_error("MMIX does not support aggregate or special call "
                         "arguments");
    if (Arg.Flags.isPointer() && Arg.Flags.getPointerAddrSpace() != 0)
      report_fatal_error(
          "MMIX does not support nonzero-address-space call arguments");
  }
  if (CLI.Ins.size() > 1)
    report_fatal_error("MMIX supports at most one scalar call result");
  for (const ISD::InputArg &Result : CLI.Ins) {
    if (!isSupportedCallValueType(Result.VT) ||
        hasUnsupportedArgumentFlags(Result.Flags))
      report_fatal_error("MMIX does not support aggregate or special call "
                         "results");
    if (Result.Flags.isPointer() && Result.Flags.getPointerAddrSpace() != 0)
      report_fatal_error(
          "MMIX does not support nonzero-address-space call results");
  }

  CLI.IsTailCall = false;
  SelectionDAG &DAG = CLI.DAG;
  MachineFunction &MF = DAG.getMachineFunction();
  SDValue Chain = CLI.Chain;

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState ArgCCInfo(CLI.CallConv, CLI.IsVarArg, MF, ArgLocs, *DAG.getContext());
  ArgCCInfo.AnalyzeCallOperands(CLI.Outs, CC_MMIX);
  unsigned NumBytes = ArgCCInfo.getStackSize();
  Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, CLI.DL);

  SmallVector<std::pair<MCRegister, SDValue>, 16> RegsToPass;
  SmallVector<SDValue, 8> StackStores;
  SDValue StackPointer;
  for (unsigned I = 0; I != ArgLocs.size(); ++I) {
    const CCValAssign &VA = ArgLocs[I];
    SDValue Value = convertOutgoingValue(CLI.OutVals[I], VA, CLI.DL, DAG);
    if (VA.isRegLoc()) {
      RegsToPass.emplace_back(VA.getLocReg(), Value);
      continue;
    }

    if (!StackPointer)
      StackPointer = DAG.getCopyFromReg(Chain, CLI.DL, MMIX::R254, MVT::i64);
    if (Value.getValueType() == MVT::f64)
      Value = DAG.getNode(ISD::BITCAST, CLI.DL, MVT::i64, Value);
    SDValue Address = StackPointer;
    if (VA.getLocMemOffset())
      Address =
          DAG.getNode(ISD::ADD, CLI.DL, MVT::i64, StackPointer,
                      DAG.getConstant(VA.getLocMemOffset(), CLI.DL, MVT::i64));
    StackStores.push_back(DAG.getStore(
        Chain, CLI.DL, Value, Address,
        MachinePointerInfo::getStack(MF, VA.getLocMemOffset()), Align(8)));
  }
  if (!StackStores.empty())
    Chain = DAG.getNode(ISD::TokenFactor, CLI.DL, MVT::Other, StackStores);

  SDValue Glue;
  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, CLI.DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  SDValue Callee = CLI.Callee;
  if (auto *GA = dyn_cast<GlobalAddressSDNode>(Callee)) {
    SDValue Target = DAG.getTargetGlobalAddress(
        GA->getGlobal(), CLI.DL, MVT::i64, GA->getOffset());
    const auto *TargetFunction = dyn_cast<Function>(GA->getGlobal());
    const Function &SourceFunction = MF.getFunction();
    bool HasStableTextLayout =
        TargetFunction && !TargetFunction->isDeclaration() &&
        !getTargetMachine().getFunctionSections() &&
        !SourceFunction.hasSection() && !TargetFunction->hasSection() &&
        !SourceFunction.hasComdat() && !TargetFunction->hasComdat();
    bool IsLocalTarget =
        TargetFunction == &SourceFunction ||
        (TargetFunction &&
         (TargetFunction->hasLocalLinkage() || TargetFunction->isDSOLocal()));
    Callee = HasStableTextLayout && IsLocalTarget
                 ? Target
                 : DAG.getNode(MMIXISD::LOAD_ADDR, CLI.DL, MVT::i64, Target);
  } else if (auto *ES = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    SDValue Target =
        DAG.getTargetExternalSymbol(ES->getSymbol(), MVT::i64);
    Callee = DAG.getNode(MMIXISD::LOAD_ADDR, CLI.DL, MVT::i64, Target);
  }

  SmallVector<SDValue, 20> CallOps = {Chain, Callee};
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  const uint32_t *Mask = TRI->getCallPreservedMask(MF, CLI.CallConv);
  if (!Mask)
    report_fatal_error("MMIX has no call-preserved mask for this convention");
  CallOps.push_back(DAG.getRegisterMask(Mask));
  for (const auto &[Reg, Value] : RegsToPass)
    CallOps.push_back(DAG.getRegister(Reg, Value.getValueType()));
  if (Glue)
    CallOps.push_back(Glue);

  Chain = DAG.getNode(MMIXISD::CALL, CLI.DL,
                      DAG.getVTList(MVT::Other, MVT::Glue), CallOps);
  Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, CLI.DL);
  Glue = Chain.getValue(1);

  SmallVector<CCValAssign, 1> ResultLocs;
  CCState ResultCCInfo(CLI.CallConv, CLI.IsVarArg, MF, ResultLocs,
                       *DAG.getContext());
  ResultCCInfo.AnalyzeCallResult(CLI.Ins, RetCC_MMIX);
  for (const CCValAssign &VA : ResultLocs) {
    SDValue Copy =
        DAG.getCopyFromReg(Chain, CLI.DL, VA.getLocReg(), VA.getLocVT(), Glue);
    SDValue Value = Copy;
    Chain = Copy.getValue(1);
    Glue = Copy.getValue(2);
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Value = DAG.getNode(ISD::AssertSext, CLI.DL, VA.getLocVT(), Value,
                          DAG.getValueType(VA.getValVT()));
      Value = DAG.getNode(ISD::TRUNCATE, CLI.DL, VA.getValVT(), Value);
      break;
    case CCValAssign::ZExt:
      Value = DAG.getNode(ISD::AssertZext, CLI.DL, VA.getLocVT(), Value,
                          DAG.getValueType(VA.getValVT()));
      Value = DAG.getNode(ISD::TRUNCATE, CLI.DL, VA.getValVT(), Value);
      break;
    case CCValAssign::AExt:
      Value = DAG.getNode(ISD::TRUNCATE, CLI.DL, VA.getValVT(), Value);
      break;
    case CCValAssign::BCvt:
      Value = DAG.getNode(ISD::BITCAST, CLI.DL, VA.getValVT(), Value);
      break;
    default:
      report_fatal_error("MMIX does not support this call result conversion");
    }
    InVals.push_back(Value);
  }

  return Chain;
}

SDValue MMIXTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (DAG.getMachineFunction().getFunction().hasPersonalityFn())
    report_fatal_error("MMIX does not support exception handling");
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
  if (CallConv != CallingConv::C || Outs.size() > 1 || !SupportedType)
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
