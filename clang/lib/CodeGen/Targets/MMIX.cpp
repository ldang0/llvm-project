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
#include "llvm/Support/MathExtras.h"

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

static void diagnoseUnsupportedMMIXScalar(CodeGenModule &CGM,
                                          SourceLocation Loc,
                                          StringRef ValueKind, QualType Ty) {
  unsigned DiagID = CGM.getDiags().getCustomDiagID(
      DiagnosticsEngine::Error,
      "MMIX GNU ABI does not support %0 type %1");
  CGM.getDiags().Report(Loc, DiagID) << ValueKind << Ty;
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
  ABIArgInfo classifyReturnType(QualType Ty) const;
  ABIArgInfo classifyArgumentType(QualType Ty) const;
  void computeInfo(CGFunctionInfo &FI) const override;
};

class MMIXTargetCodeGenInfo : public TargetCodeGenInfo {
public:
  explicit MMIXTargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<MMIXABIInfo>(CGT)) {}

  void checkFunctionABI(CodeGenModule &CGM,
                        const FunctionDecl *FD) const override;
  void checkFunctionCallABI(CodeGenModule &CGM, SourceLocation CallLoc,
                            const FunctionDecl *, const FunctionDecl *,
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
    return DefaultABIInfo::classifyReturnType(Ty);
  return ABIArgInfo::getDirect();
}

ABIArgInfo MMIXABIInfo::classifyAggregateArgument(QualType Ty) const {
  if (isEmptyRecord(getContext(), Ty, /*AllowArrays=*/true))
    return ABIArgInfo::getIgnore();

  uint64_t Size = getContext().getTypeSize(Ty);
  if (Size > 64)
    return DefaultABIInfo::classifyArgumentType(Ty);

  llvm::IntegerType *CoerceTy = llvm::IntegerType::get(getVMContext(), Size);
  if (llvm::isPowerOf2_64(Size))
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

void MMIXTargetCodeGenInfo::checkFunctionABI(CodeGenModule &CGM,
                                             const FunctionDecl *FD) const {
  ASTContext &Context = CGM.getContext();
  QualType ReturnType = FD->getReturnType();
  if (isUnsupportedMMIXScalarType(Context, ReturnType, /*AllowVoid=*/true))
    diagnoseUnsupportedMMIXScalar(CGM, FD->getLocation(), "return",
                                  ReturnType);

  for (const ParmVarDecl *Param : FD->parameters()) {
    QualType Ty = Param->getType();
    if (isUnsupportedMMIXScalarType(Context, Ty, /*AllowVoid=*/false))
      diagnoseUnsupportedMMIXScalar(CGM, Param->getLocation(), "argument", Ty);
  }
}

void MMIXTargetCodeGenInfo::checkFunctionCallABI(
    CodeGenModule &CGM, SourceLocation CallLoc, const FunctionDecl *,
    const FunctionDecl *, const CallArgList &Args, QualType ReturnType) const {
  ASTContext &Context = CGM.getContext();
  if (isUnsupportedMMIXScalarType(Context, ReturnType, /*AllowVoid=*/true))
    diagnoseUnsupportedMMIXScalar(CGM, CallLoc, "return", ReturnType);

  for (const CallArg &Arg : Args) {
    QualType Ty = Arg.getType();
    if (isUnsupportedMMIXScalarType(Context, Ty, /*AllowVoid=*/false))
      diagnoseUnsupportedMMIXScalar(CGM, CallLoc, "argument", Ty);
  }
}

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createMMIXTargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<MMIXTargetCodeGenInfo>(CGM.getTypes());
}
