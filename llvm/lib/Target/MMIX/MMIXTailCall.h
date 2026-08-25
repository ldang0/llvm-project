//===-- MMIXTailCall.h - MMIX tail-call eligibility -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXTAILCALL_H
#define LLVM_LIB_TARGET_MMIX_MMIXTAILCALL_H

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/CallingConv.h"
#include <cstdint>

namespace llvm {

enum class MMIXAggregateABIKind;

enum class MMIXTailCallRequestKind {
  Ordinary,
  Required,
};

enum class MMIXTailCallCalleeKind {
  Direct,
  Indirect,
  Unsupported,
};

enum class MMIXTailCallIndirectResultKind {
  None,
  Forwarded,
  Incompatible,
};

enum class MMIXTailCallResultShape {
  NoResult,
  OneRegister,
  TwoRegisters,
  Indirect,
  Unsupported,
};

enum class MMIXTailCallEligibilityReason {
  Eligible,
  UnsupportedCallerCallingConvention,
  UnsupportedCalleeCallingConvention,
  MismatchedCallingConvention,
  VariadicCaller,
  VariadicCallee,
  IncompatibleResults,
  IncompatibleArguments,
  IncompatibleIndirectResult,
  EphemeralCallerCopy,
  UnsupportedCallee,
  DynamicStack,
  StackRealignment,
  UnrestorableFrame,
  MisalignedOutgoingStack,
  InsufficientReusableStack,
};

enum class MMIXTailCallDisposition {
  TailTransfer,
  NormalCall,
  Diagnose,
};

struct MMIXTailCallFrameState {
  bool HasDynamicStack = false;
  bool RequiresStackRealignment = false;
  bool CanRestoreFrame = false;

  bool isEligible() const;
  MMIXTailCallEligibilityReason getReason() const;
};

struct MMIXTailCallEligibilityInput {
  CallingConv::ID CallerCC = CallingConv::C;
  CallingConv::ID CalleeCC = CallingConv::C;
  bool CallerIsVarArg = false;
  bool CalleeIsVarArg = false;
  bool ResultsAreCompatible = false;
  bool ArgumentsAreCompatible = false;
  MMIXTailCallIndirectResultKind IndirectResult =
      MMIXTailCallIndirectResultKind::None;
  bool HasCallerCopy = false;
  bool CallerCopySurvivesTransfer = false;
  MMIXTailCallCalleeKind Callee = MMIXTailCallCalleeKind::Unsupported;
  bool HasDynamicStack = false;
  bool RequiresStackRealignment = false;
  bool CanRestoreFrame = false;
  uint64_t OutgoingStackBytes = 0;
  uint64_t ReusableIncomingStackBytes = 0;
};

struct MMIXTailCallABIInput {
  MMIXTailCallResultShape CallerResult =
      MMIXTailCallResultShape::Unsupported;
  MMIXTailCallResultShape CalleeResult =
      MMIXTailCallResultShape::Unsupported;
  bool ArgumentsAreCompatible = false;
  bool ForwardsIndirectResult = false;
  bool HasCallerCopy = false;
  bool CallerCopySurvivesTransfer = false;
};

class MMIXTailCallEligibility {
  MMIXTailCallEligibilityReason Reason;

public:
  explicit MMIXTailCallEligibility(MMIXTailCallEligibilityReason Reason)
      : Reason(Reason) {}

  bool isEligible() const {
    return Reason == MMIXTailCallEligibilityReason::Eligible;
  }
  MMIXTailCallEligibilityReason getReason() const { return Reason; }
  StringRef getReasonText() const;
  MMIXTailCallDisposition getDisposition(MMIXTailCallRequestKind Request) const;
};

MMIXTailCallEligibility
classifyMMIXTailCall(const MMIXTailCallEligibilityInput &Input);

void applyMMIXTailCallABI(MMIXTailCallEligibilityInput &Eligibility,
                          const MMIXTailCallABIInput &ABI);

MMIXTailCallResultShape
classifyMMIXTailCallResultShape(MMIXAggregateABIKind Kind,
                                unsigned NumResultRegisters);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXTAILCALL_H
