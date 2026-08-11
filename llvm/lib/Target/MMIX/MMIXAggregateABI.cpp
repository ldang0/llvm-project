//===-- MMIXAggregateABI.cpp - MMIX aggregate ABI classification ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXAggregateABI.h"

using namespace llvm;

MMIXAggregateABIClassification
llvm::classifyMMIXAggregateABI(const MMIXAggregateABIValue &Value) {
  const bool IsAggregate = Value.IsAggregate || Value.IsByVal || Value.IsSRet;
  MMIXAggregateABIKind Kind = MMIXAggregateABIKind::Scalar;
  if (IsAggregate) {
    if (Value.Size && *Value.Size == 0)
      Kind = MMIXAggregateABIKind::Empty;
    else if (Value.IsByVal)
      Kind = MMIXAggregateABIKind::CallerCopyArgument;
    else if (Value.IsSRet)
      Kind = MMIXAggregateABIKind::IndirectResult;
    else if (Value.Role == MMIXAggregateABIRole::Argument)
      Kind = MMIXAggregateABIKind::DirectArgument;
    else
      Kind = MMIXAggregateABIKind::DirectResult;
  }

  if (Value.AddressSpace != 0)
    return {Kind, MMIXAggregateABIError::NonZeroAddressSpace};
  if (Value.IsSplit)
    return {Kind, MMIXAggregateABIError::Split};
  if (Value.IsInConsecutiveRegs || Value.NumParts > 1)
    return {Kind, MMIXAggregateABIError::MultiRegister};
  if (Value.HasUnsupportedFlags || (Value.IsByVal && Value.IsSRet) ||
      (Value.IsByVal && Value.Role != MMIXAggregateABIRole::Argument) ||
      (Value.IsSRet && Value.Role != MMIXAggregateABIRole::Result))
    return {Kind, MMIXAggregateABIError::UnsupportedFlags};
  if (!IsAggregate)
    return {MMIXAggregateABIKind::Scalar, MMIXAggregateABIError::None};
  if (!Value.Size)
    return {Kind, MMIXAggregateABIError::VariableSize};
  if (Value.Alignment > Align(8))
    return {Kind, MMIXAggregateABIError::OverAligned};
  if (*Value.Size == 0)
    return {MMIXAggregateABIKind::Empty, MMIXAggregateABIError::None};
  if (Value.IsByVal)
    return {MMIXAggregateABIKind::CallerCopyArgument,
            MMIXAggregateABIError::None};
  if (Value.IsSRet)
    return {MMIXAggregateABIKind::IndirectResult,
            MMIXAggregateABIError::None};
  if (*Value.Size > 8)
    return {Kind, MMIXAggregateABIError::MultiRegister};
  if (Value.Role == MMIXAggregateABIRole::Argument)
    return {MMIXAggregateABIKind::DirectArgument,
            MMIXAggregateABIError::None};
  return {MMIXAggregateABIKind::DirectResult, MMIXAggregateABIError::None};
}
