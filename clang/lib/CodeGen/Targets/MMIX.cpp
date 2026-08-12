//===- MMIX.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {

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
};

} // namespace

ABIArgInfo MMIXABIInfo::classifyReturnType(QualType Ty) const {
  if (Ty->isVoidType())
    return ABIArgInfo::getIgnore();
  if (isAggregateTypeForABI(Ty))
    return DefaultABIInfo::classifyReturnType(Ty);
  return ABIArgInfo::getDirect();
}

ABIArgInfo MMIXABIInfo::classifyArgumentType(QualType Ty) const {
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

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createMMIXTargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<MMIXTargetCodeGenInfo>(CGM.getTypes());
}
