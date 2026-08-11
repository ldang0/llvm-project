//===-- MMIXAggregateABI.h - MMIX aggregate ABI classification -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXAGGREGATEABI_H
#define LLVM_LIB_TARGET_MMIX_MMIXAGGREGATEABI_H

#include "llvm/Support/Alignment.h"
#include <cstdint>
#include <optional>

namespace llvm {

enum class MMIXAggregateABIRole {
  Argument,
  Result,
};

enum class MMIXAggregateABIKind {
  Scalar,
  Empty,
  DirectArgument,
  CallerCopyArgument,
  DirectResult,
  IndirectResult,
};

enum class MMIXAggregateABIError {
  None,
  VariableSize,
  OverAligned,
  NonZeroAddressSpace,
  Split,
  MultiRegister,
  UnsupportedFlags,
};

struct MMIXAggregateABIValue {
  MMIXAggregateABIRole Role = MMIXAggregateABIRole::Argument;
  bool IsAggregate = false;
  bool IsByVal = false;
  bool IsSRet = false;
  bool IsSplit = false;
  bool IsInConsecutiveRegs = false;
  bool HasUnsupportedFlags = false;
  std::optional<uint64_t> Size;
  Align Alignment = Align(1);
  unsigned AddressSpace = 0;
  unsigned NumParts = 1;
};

struct MMIXAggregateABIClassification {
  MMIXAggregateABIKind Kind = MMIXAggregateABIKind::Scalar;
  MMIXAggregateABIError Error = MMIXAggregateABIError::None;

  bool isValid() const { return Error == MMIXAggregateABIError::None; }
  bool isAggregate() const { return Kind != MMIXAggregateABIKind::Scalar; }
};

MMIXAggregateABIClassification
classifyMMIXAggregateABI(const MMIXAggregateABIValue &Value);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXAGGREGATEABI_H
