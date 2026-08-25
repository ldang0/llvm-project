//===-- MMIXTailCall.cpp - MMIX tail-call eligibility -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXTailCall.h"
#include "MMIXCallingConv.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

bool MMIXTailCallFrameState::isEligible() const {
  return getReason() == MMIXTailCallEligibilityReason::Eligible;
}

MMIXTailCallEligibilityReason MMIXTailCallFrameState::getReason() const {
  if (HasDynamicStack)
    return MMIXTailCallEligibilityReason::DynamicStack;
  if (RequiresStackRealignment)
    return MMIXTailCallEligibilityReason::StackRealignment;
  return CanRestoreFrame ? MMIXTailCallEligibilityReason::Eligible
                         : MMIXTailCallEligibilityReason::UnrestorableFrame;
}

MMIXTailCallEligibility
llvm::classifyMMIXTailCall(const MMIXTailCallEligibilityInput &Input) {
  using Reason = MMIXTailCallEligibilityReason;

  if (!isSupportedMMIXCallingConv(Input.CallerCC))
    return MMIXTailCallEligibility(Reason::UnsupportedCallerCallingConvention);
  if (!isSupportedMMIXCallingConv(Input.CalleeCC))
    return MMIXTailCallEligibility(Reason::UnsupportedCalleeCallingConvention);
  if (Input.CallerCC != Input.CalleeCC)
    return MMIXTailCallEligibility(Reason::MismatchedCallingConvention);
  if (Input.CallerIsVarArg)
    return MMIXTailCallEligibility(Reason::VariadicCaller);
  if (Input.CalleeIsVarArg)
    return MMIXTailCallEligibility(Reason::VariadicCallee);
  if (!Input.ResultsAreCompatible)
    return MMIXTailCallEligibility(Reason::IncompatibleResults);
  if (!Input.ArgumentsAreCompatible)
    return MMIXTailCallEligibility(Reason::IncompatibleArguments);
  if (Input.IndirectResult == MMIXTailCallIndirectResultKind::Incompatible)
    return MMIXTailCallEligibility(Reason::IncompatibleIndirectResult);
  if (Input.HasCallerCopy && !Input.CallerCopySurvivesTransfer)
    return MMIXTailCallEligibility(Reason::EphemeralCallerCopy);
  if (Input.Callee == MMIXTailCallCalleeKind::Unsupported)
    return MMIXTailCallEligibility(Reason::UnsupportedCallee);
  if (Input.HasDynamicStack)
    return MMIXTailCallEligibility(Reason::DynamicStack);
  if (Input.RequiresStackRealignment)
    return MMIXTailCallEligibility(Reason::StackRealignment);
  if (!Input.CanRestoreFrame)
    return MMIXTailCallEligibility(Reason::UnrestorableFrame);
  if (Input.OutgoingStackBytes % 8 != 0)
    return MMIXTailCallEligibility(Reason::MisalignedOutgoingStack);
  if (Input.OutgoingStackBytes > Input.ReusableIncomingStackBytes)
    return MMIXTailCallEligibility(Reason::InsufficientReusableStack);
  return MMIXTailCallEligibility(Reason::Eligible);
}

StringRef MMIXTailCallEligibility::getReasonText() const {
  switch (Reason) {
  case MMIXTailCallEligibilityReason::Eligible:
    return {};
  case MMIXTailCallEligibilityReason::UnsupportedCallerCallingConvention:
    return "caller calling convention is unsupported";
  case MMIXTailCallEligibilityReason::UnsupportedCalleeCallingConvention:
    return "callee calling convention is unsupported";
  case MMIXTailCallEligibilityReason::MismatchedCallingConvention:
    return "caller and callee calling conventions do not match";
  case MMIXTailCallEligibilityReason::VariadicCaller:
    return "variadic caller is unsupported";
  case MMIXTailCallEligibilityReason::VariadicCallee:
    return "variadic callee is unsupported";
  case MMIXTailCallEligibilityReason::IncompatibleResults:
    return "caller and callee result locations are incompatible";
  case MMIXTailCallEligibilityReason::IncompatibleArguments:
    return "callee argument locations are incompatible";
  case MMIXTailCallEligibilityReason::IncompatibleIndirectResult:
    return "indirect result is not forwarded compatibly";
  case MMIXTailCallEligibilityReason::EphemeralCallerCopy:
    return "caller-copy storage does not survive the transfer";
  case MMIXTailCallEligibilityReason::UnsupportedCallee:
    return "callee form is unsupported";
  case MMIXTailCallEligibilityReason::DynamicStack:
    return "dynamic stack allocation prevents frame reuse";
  case MMIXTailCallEligibilityReason::StackRealignment:
    return "stack realignment prevents frame reuse";
  case MMIXTailCallEligibilityReason::UnrestorableFrame:
    return "software frame cannot be restored before transfer";
  case MMIXTailCallEligibilityReason::MisalignedOutgoingStack:
    return "outgoing stack argument area is misaligned";
  case MMIXTailCallEligibilityReason::InsufficientReusableStack:
    return "outgoing stack arguments exceed the reusable incoming area";
  }
  llvm_unreachable("unhandled MMIX tail-call eligibility reason");
}

MMIXTailCallDisposition
MMIXTailCallEligibility::getDisposition(MMIXTailCallRequestKind Request) const {
  if (isEligible())
    return MMIXTailCallDisposition::TailTransfer;
  return Request == MMIXTailCallRequestKind::Ordinary
             ? MMIXTailCallDisposition::NormalCall
             : MMIXTailCallDisposition::Diagnose;
}
