//===- MMIX.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "CGCall.h"
#include "CodeGenModule.h"
#include "TargetInfo.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Diagnostic.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/MathExtras.h"

#include <algorithm>

using namespace clang;
using namespace clang::CodeGen;

namespace {

static bool isDeferredMMIXBoundaryType(QualType Ty) {
  return Ty->isRecordType() || Ty->isArrayType() || Ty->isAtomicType();
}

static bool isSupportedMMIXScalarType(const ASTContext &Context, QualType Ty,
                                      bool AllowVoid) {
  if (Ty->isVoidType())
    return AllowVoid;

  if (Ty->isIntegralOrEnumerationType()) {
    if (Ty->isBitIntType())
      return false;
    return Context.getTypeSize(Ty) <= 64;
  }

  if (const auto *PT = Ty->getAs<PointerType>())
    return Context.getTargetAddressSpace(
               PT->getPointeeType().getAddressSpace()) == 0;

  return Ty->isSpecificBuiltinType(BuiltinType::Float) ||
         Ty->isSpecificBuiltinType(BuiltinType::Double) ||
         Ty->isSpecificBuiltinType(BuiltinType::LongDouble);
}

static bool isUnsupportedMMIXScalarType(const ASTContext &Context,
                                        QualType Ty, bool AllowVoid) {
  if (isDeferredMMIXBoundaryType(Ty))
    return false;
  return !isSupportedMMIXScalarType(Context, Ty, AllowVoid);
}

enum class MMIXGCCModeKind { Scalar, Block, AlignmentOnlyBlock };

struct MMIXGCCMode {
  MMIXGCCModeKind Kind;
  uint64_t SizeInBits;
  uint64_t AlignInBits;
  bool IsInteger;
};

static MMIXGCCMode getMMIXGCCIntegerMode(uint64_t SizeInBits) {
  if (SizeInBits == 8 || SizeInBits == 16 || SizeInBits == 32 ||
      SizeInBits == 64 || SizeInBits == 128)
    return {MMIXGCCModeKind::Scalar, SizeInBits,
            std::min(SizeInBits, uint64_t(64)), true};
  return {MMIXGCCModeKind::Block, SizeInBits, 0, false};
}

// Mirror the part of GCC compute_record_mode that distinguishes scalar record
// modes from BLKmode for the frozen MMIX C result boundary. AlignmentOnlyBlock
// represents GCC's TYPE_NO_FORCE_BLK case, which does not force an enclosing
// record to remain BLKmode.
static MMIXGCCMode getMMIXGCCTypeMode(const ASTContext &Context, QualType Ty) {
  if (Ty->isIncompleteType())
    return {MMIXGCCModeKind::Block, 0, 0, false};

  uint64_t Size = Context.getTypeSize(Ty);
  if (const auto *RT = Ty->getAsCanonical<RecordType>()) {
    const RecordDecl *RD = RT->getDecl()->getDefinitionOrSelf();
    MMIXGCCMode WholeField = {MMIXGCCModeKind::Block, 0, 0, false};

    for (const FieldDecl *Field : RD->fields()) {
      QualType FieldTy = Field->getType();
      uint64_t FieldSize;
      if (Field->isBitField())
        FieldSize = Field->getBitWidthValue();
      else if (FieldTy->isIncompleteArrayType())
        FieldSize = 0;
      else if (FieldTy->isIncompleteType())
        return {MMIXGCCModeKind::Block, Size, 0, false};
      else
        FieldSize = Context.getTypeSize(FieldTy);

      if (FieldSize == 0)
        continue;

      MMIXGCCMode FieldMode = getMMIXGCCTypeMode(Context, FieldTy);
      if (FieldMode.Kind == MMIXGCCModeKind::Block)
        return {MMIXGCCModeKind::Block, Size, 0, false};

      if (FieldSize == Size && FieldMode.Kind == MMIXGCCModeKind::Scalar &&
          (!RD->isUnion() || FieldMode.IsInteger) &&
          FieldMode.SizeInBits > WholeField.SizeInBits)
        WholeField = FieldMode;
    }

    MMIXGCCMode Mode = WholeField.Kind == MMIXGCCModeKind::Scalar
                           ? WholeField
                           : getMMIXGCCIntegerMode(Size);
    if (Mode.Kind == MMIXGCCModeKind::Block)
      return Mode;
    if (Context.getTypeAlign(Ty) < Mode.AlignInBits)
      return {MMIXGCCModeKind::AlignmentOnlyBlock, Size, 0, false};
    return Mode;
  }

  if (const auto *AT = Context.getAsConstantArrayType(Ty)) {
    if (AT->getSize().isOne()) {
      MMIXGCCMode ElementMode =
          getMMIXGCCTypeMode(Context, AT->getElementType());
      if (ElementMode.Kind != MMIXGCCModeKind::Scalar)
        return {MMIXGCCModeKind::Block, Size, 0, false};
      return ElementMode;
    }
    return getMMIXGCCIntegerMode(Size);
  }

  bool IsInteger = Ty->isIntegralOrEnumerationType() || Ty->isPointerType();
  return {MMIXGCCModeKind::Scalar, Size, std::min(Size, uint64_t(64)),
          IsInteger};
}

static void diagnoseUnsupportedMMIXScalar(CodeGenModule &CGM,
                                          SourceLocation Loc,
                                          StringRef ValueKind, QualType Ty) {
  unsigned DiagID = CGM.getDiags().getCustomDiagID(
      DiagnosticsEngine::Error,
      "MMIX GNU ABI does not support %0 type %1");
  CGM.getDiags().Report(Loc, DiagID) << ValueKind << Ty;
}

static bool diagnoseUnsupportedMMIXAggregateArgument(CodeGenModule &CGM,
                                                     SourceLocation Loc,
                                                     QualType Ty) {
  ASTContext &Context = CGM.getContext();
  if (!Ty->isRecordType())
    return false;

  StringRef Reason;
  if (Ty->isIncompleteType())
    Reason = "incomplete";
  else if (Ty->isVariablyModifiedType())
    Reason = "variable-size";
  else if (Context.getTypeAlign(Ty) > 64)
    Reason = "over-aligned";
  else
    return false;

  unsigned DiagID = CGM.getDiags().getCustomDiagID(
      DiagnosticsEngine::Error,
      "MMIX GNU ABI does not support %0 aggregate argument type %1");
  CGM.getDiags().Report(Loc, DiagID) << Reason << Ty;
  return true;
}

static bool diagnoseUnsupportedMMIXAggregateResult(CodeGenModule &CGM,
                                                   SourceLocation Loc,
                                                   QualType Ty) {
  ASTContext &Context = CGM.getContext();
  if (!Ty->isRecordType())
    return false;

  StringRef Reason;
  if (Ty->isIncompleteType())
    Reason = "incomplete";
  else if (Ty->isVariablyModifiedType())
    Reason = "variable-size";
  else if (Context.getTypeAlign(Ty) > 64)
    Reason = "over-aligned";
  else {
    MMIXGCCMode Mode = getMMIXGCCTypeMode(Context, Ty);
    if (Mode.Kind == MMIXGCCModeKind::Scalar && Mode.SizeInBits > 64)
      Reason = "wide scalar-mode";
    else
      return false;
  }

  unsigned DiagID = CGM.getDiags().getCustomDiagID(
      DiagnosticsEngine::Error,
      "MMIX GNU ABI does not support %0 aggregate return type %1");
  CGM.getDiags().Report(Loc, DiagID) << Reason << Ty;
  return true;
}

static bool diagnoseUnsupportedMMIXVariadicSignature(CodeGenModule &CGM,
                                                      SourceLocation Loc,
                                                      const FunctionDecl *FD) {
  if (!FD || !FD->isVariadic() || FD->getNumParams() == 0)
    return false;

  QualType LastNamedType = FD->getParamDecl(FD->getNumParams() - 1)->getType();
  if (!isEmptyRecord(CGM.getContext(), LastNamedType, /*AllowArrays=*/true))
    return false;

  unsigned DiagID = CGM.getDiags().getCustomDiagID(
      DiagnosticsEngine::Error,
      "MMIX GNU ABI does not support an empty final named parameter in a "
      "variadic function");
  CGM.getDiags().Report(Loc, DiagID);
  return true;
}

class MMIXABIInfo : public DefaultABIInfo {
public:
  explicit MMIXABIInfo(CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}

  llvm::Value *createCoercedLoad(Address Src, const ABIArgInfo &AI,
                                 CodeGenFunction &CGF) const override;
  void createCoercedStore(llvm::Value *Val, Address Dst,
                          const ABIArgInfo &AI, bool DestIsVolatile,
                          CodeGenFunction &CGF) const override;

private:
  ABIArgInfo classifyAggregateArgument(QualType Ty) const;
  ABIArgInfo classifyAggregateReturn(QualType Ty) const;
  ABIArgInfo classifyReturnType(QualType Ty) const;
  ABIArgInfo classifyArgumentType(QualType Ty) const;
  void computeInfo(CGFunctionInfo &FI) const override;
  RValue EmitVAArg(CodeGenFunction &CGF, Address VAListAddr, QualType Ty,
                   AggValueSlot Slot) const override;
};

class MMIXTargetCodeGenInfo : public TargetCodeGenInfo {
public:
  explicit MMIXTargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<MMIXABIInfo>(CGT)) {}

  void checkFunctionABI(CodeGenModule &CGM,
                        const FunctionDecl *FD) const override;
  void checkFunctionCallABI(CodeGenModule &CGM, SourceLocation CallLoc,
                            const FunctionDecl *, const FunctionDecl *Callee,
                            const CallArgList &Args,
                            QualType ReturnType) const override;
};

} // namespace

ABIArgInfo MMIXABIInfo::classifyReturnType(QualType Ty) const {
  if (Ty->isVoidType())
    return ABIArgInfo::getIgnore();
  if (isUnsupportedMMIXScalarType(getContext(), Ty, /*AllowVoid=*/true))
    return ABIArgInfo::getDirect();
  if (isAggregateTypeForABI(Ty))
    return classifyAggregateReturn(Ty);
  return ABIArgInfo::getDirect();
}

ABIArgInfo MMIXABIInfo::classifyAggregateReturn(QualType Ty) const {
  if (isEmptyRecord(getContext(), Ty, /*AllowArrays=*/true))
    return ABIArgInfo::getIgnore();

  MMIXGCCMode Mode = getMMIXGCCTypeMode(getContext(), Ty);
  if (Mode.Kind != MMIXGCCModeKind::Scalar)
    return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(),
                                   /*ByVal=*/false);

  llvm::IntegerType *CoerceTy =
      llvm::IntegerType::get(getVMContext(), Mode.SizeInBits);
  if (llvm::isPowerOf2_64(Mode.SizeInBits))
    return ABIArgInfo::getDirect(CoerceTy);

  return ABIArgInfo::getTargetSpecific(llvm::Type::getInt64Ty(getVMContext()),
                                       /*Offset=*/0, /*Padding=*/nullptr,
                                       /*CanBeFlattened=*/false);
}

ABIArgInfo MMIXABIInfo::classifyAggregateArgument(QualType Ty) const {
  if (isEmptyRecord(getContext(), Ty, /*AllowArrays=*/true))
    return ABIArgInfo::getIgnore();

  if (const auto *RT = Ty->getAsCanonical<RecordType>()) {
    const RecordDecl *RD = RT->getDecl()->getDefinitionOrSelf();
    if (!isa<CXXRecordDecl>(RD) && !RD->canPassInRegisters())
      return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace());
  }

  uint64_t Size = getContext().getTypeSize(Ty);
  if (Size > 64)
    return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace());

  llvm::IntegerType *CoerceTy = llvm::IntegerType::get(getVMContext(), Size);
  if (llvm::isPowerOf2_64(Size) && Size < 64)
    return ABIArgInfo::getNoExtend(CoerceTy);
  if (Size == 64)
    return ABIArgInfo::getDirect(CoerceTy);

  return ABIArgInfo::getTargetSpecific(
      llvm::Type::getInt64Ty(getVMContext()), /*Offset=*/0, /*Padding=*/nullptr,
      /*CanBeFlattened=*/false);
}

ABIArgInfo MMIXABIInfo::classifyArgumentType(QualType Ty) const {
  Ty = useFirstFieldIfTransparentUnion(Ty);
  if (isUnsupportedMMIXScalarType(getContext(), Ty, /*AllowVoid=*/false))
    return ABIArgInfo::getDirect();
  if (isAggregateTypeForABI(Ty))
    return classifyAggregateArgument(Ty);

  if (Ty->isIntegralOrEnumerationType() && getContext().getTypeSize(Ty) < 64)
    return ABIArgInfo::getExtend(Ty, CGT.ConvertType(Ty));
  return ABIArgInfo::getDirect();
}

llvm::Value *MMIXABIInfo::createCoercedLoad(Address Src, const ABIArgInfo &AI,
                                            CodeGenFunction &CGF) const {
  assert(AI.isTargetSpecific() && AI.getCoerceToType()->isIntegerTy(64));
  uint64_t Size = getDataLayout().getTypeAllocSize(Src.getElementType());
  assert(Size > 0 && Size < 8 && !llvm::isPowerOf2_64(Size));

  llvm::Value *Result = llvm::ConstantInt::get(CGF.Int64Ty, 0);
  Address ByteSrc = Src.withElementType(CGF.Int8Ty);
  for (uint64_t I = 0; I != Size; ++I) {
    Address ByteAddr = CGF.Builder.CreateConstInBoundsByteGEP(
        ByteSrc, CharUnits::fromQuantity(I));
    llvm::Value *Byte = CGF.Builder.CreateLoad(ByteAddr);
    Byte = CGF.Builder.CreateZExt(Byte, CGF.Int64Ty);
    unsigned Shift = 8 * (Size - I - 1);
    if (Shift)
      Byte = CGF.Builder.CreateShl(Byte, Shift);
    Result = CGF.Builder.CreateOr(Result, Byte);
  }
  return Result;
}

void MMIXABIInfo::createCoercedStore(llvm::Value *Val, Address Dst,
                                     const ABIArgInfo &AI,
                                     bool DestIsVolatile,
                                     CodeGenFunction &CGF) const {
  assert(AI.isTargetSpecific() && Val->getType()->isIntegerTy(64));
  uint64_t Size = getDataLayout().getTypeAllocSize(Dst.getElementType());
  assert(Size > 0 && Size < 8 && !llvm::isPowerOf2_64(Size));

  Address ByteDst = Dst.withElementType(CGF.Int8Ty);
  for (uint64_t I = 0; I != Size; ++I) {
    unsigned Shift = 8 * (Size - I - 1);
    llvm::Value *Byte = Val;
    if (Shift)
      Byte = CGF.Builder.CreateLShr(Byte, Shift);
    Byte = CGF.Builder.CreateTrunc(Byte, CGF.Int8Ty);
    Address ByteAddr = CGF.Builder.CreateConstInBoundsByteGEP(
        ByteDst, CharUnits::fromQuantity(I));
    CGF.Builder.CreateStore(Byte, ByteAddr, DestIsVolatile);
  }
}

void MMIXABIInfo::computeInfo(CGFunctionInfo &FI) const {
  if (!getCXXABI().classifyReturnType(FI))
    FI.getReturnInfo() = classifyReturnType(FI.getReturnType());
  for (auto &Arg : FI.arguments())
    Arg.info = classifyArgumentType(Arg.type);
}

RValue MMIXABIInfo::EmitVAArg(CodeGenFunction &CGF, Address VAListAddr,
                              QualType Ty, AggValueSlot Slot) const {
  if (Ty->isAtomicType() ||
      isUnsupportedMMIXScalarType(getContext(), Ty, /*AllowVoid=*/false)) {
    unsigned DiagID = CGF.CGM.getDiags().getCustomDiagID(
        DiagnosticsEngine::Error,
        "MMIX GNU ABI does not support va_arg type %0");
    SourceLocation Loc =
        CGF.CurCodeDecl ? CGF.CurCodeDecl->getLocation() : SourceLocation();
    CGF.CGM.getDiags().Report(Loc, DiagID) << Ty;

    if (const auto *ComplexTy = Ty->getAs<ComplexType>()) {
      llvm::Type *ElementTy = CGF.ConvertType(ComplexTy->getElementType());
      llvm::Value *Poison = llvm::PoisonValue::get(ElementTy);
      return RValue::getComplex(Poison, Poison);
    }
    return RValue::get(llvm::PoisonValue::get(CGF.ConvertType(Ty)));
  }

  if (isAggregateTypeForABI(Ty))
    return DefaultABIInfo::EmitVAArg(CGF, VAListAddr, Ty, Slot);

  return emitVoidPtrVAArg(
      CGF, VAListAddr, Ty, /*IsIndirect=*/false,
      getContext().getTypeInfoInChars(Ty), CharUnits::fromQuantity(8),
      /*AllowHigherAlign=*/false, Slot, /*ForceRightAdjust=*/true);
}

void MMIXTargetCodeGenInfo::checkFunctionABI(CodeGenModule &CGM,
                                             const FunctionDecl *FD) const {
  if (FD->getNumParams() != 0)
    diagnoseUnsupportedMMIXVariadicSignature(
        CGM, FD->getParamDecl(FD->getNumParams() - 1)->getLocation(), FD);

  ASTContext &Context = CGM.getContext();
  QualType ReturnType = FD->getReturnType();
  if (!diagnoseUnsupportedMMIXAggregateResult(CGM, FD->getLocation(),
                                              ReturnType) &&
      isUnsupportedMMIXScalarType(Context, ReturnType, /*AllowVoid=*/true))
    diagnoseUnsupportedMMIXScalar(CGM, FD->getLocation(), "return", ReturnType);

  for (const ParmVarDecl *Param : FD->parameters()) {
    QualType Ty = Param->getType();
    if (diagnoseUnsupportedMMIXAggregateArgument(CGM, Param->getLocation(), Ty))
      continue;
    if (isUnsupportedMMIXScalarType(Context, Ty, /*AllowVoid=*/false))
      diagnoseUnsupportedMMIXScalar(CGM, Param->getLocation(), "argument", Ty);
  }
}

void MMIXTargetCodeGenInfo::checkFunctionCallABI(
    CodeGenModule &CGM, SourceLocation CallLoc, const FunctionDecl *,
    const FunctionDecl *Callee, const CallArgList &Args,
    QualType ReturnType) const {
  diagnoseUnsupportedMMIXVariadicSignature(CGM, CallLoc, Callee);

  ASTContext &Context = CGM.getContext();
  if (!diagnoseUnsupportedMMIXAggregateResult(CGM, CallLoc, ReturnType) &&
      isUnsupportedMMIXScalarType(Context, ReturnType, /*AllowVoid=*/true))
    diagnoseUnsupportedMMIXScalar(CGM, CallLoc, "return", ReturnType);

  for (const CallArg &Arg : Args) {
    QualType Ty = Arg.getType();
    if (diagnoseUnsupportedMMIXAggregateArgument(CGM, CallLoc, Ty))
      continue;
    if (isUnsupportedMMIXScalarType(Context, Ty, /*AllowVoid=*/false))
      diagnoseUnsupportedMMIXScalar(CGM, CallLoc, "argument", Ty);
  }
}

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createMMIXTargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<MMIXTargetCodeGenInfo>(CGM.getTypes());
}
