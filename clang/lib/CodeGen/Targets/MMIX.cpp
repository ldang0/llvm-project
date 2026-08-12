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

private:
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

ABIArgInfo MMIXABIInfo::classifyArgumentType(QualType Ty) const {
  if (isUnsupportedMMIXScalarType(getContext(), Ty, /*AllowVoid=*/false))
    return ABIArgInfo::getDirect();
  if (isAggregateTypeForABI(Ty))
    return DefaultABIInfo::classifyArgumentType(Ty);

  if (Ty->isIntegralOrEnumerationType() && getContext().getTypeSize(Ty) < 64)
    return ABIArgInfo::getExtend(Ty, CGT.ConvertType(Ty));
  return ABIArgInfo::getDirect();
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
