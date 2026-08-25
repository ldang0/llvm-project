//===- MMIXTailCallTest.cpp - MMIX tail-call eligibility tests -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXTailCall.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInstrInfo.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>

using namespace llvm;

namespace {

static MMIXTailCallEligibilityInput eligibleInput() {
  MMIXTailCallEligibilityInput Input;
  Input.ResultsAreCompatible = true;
  Input.ArgumentsAreCompatible = true;
  Input.Callee = MMIXTailCallCalleeKind::Direct;
  Input.CanRestoreFrame = true;
  return Input;
}

static void expectReason(const MMIXTailCallEligibilityInput &Input,
                         MMIXTailCallEligibilityReason Reason, StringRef Text) {
  MMIXTailCallEligibility Result = classifyMMIXTailCall(Input);
  EXPECT_FALSE(Result.isEligible());
  EXPECT_EQ(Result.getReason(), Reason);
  EXPECT_EQ(Result.getReasonText(), Text);
  EXPECT_EQ(Result.getDisposition(MMIXTailCallRequestKind::Ordinary),
            MMIXTailCallDisposition::NormalCall);
  EXPECT_EQ(Result.getDisposition(MMIXTailCallRequestKind::Required),
            MMIXTailCallDisposition::Diagnose);
}

TEST(MMIXTailCallTest, AcceptsReviewedCAndFastForms) {
  MMIXTailCallEligibilityInput Input = eligibleInput();
  MMIXTailCallEligibility Result = classifyMMIXTailCall(Input);
  EXPECT_TRUE(Result.isEligible());
  EXPECT_EQ(Result.getReason(), MMIXTailCallEligibilityReason::Eligible);
  EXPECT_TRUE(Result.getReasonText().empty());
  EXPECT_EQ(Result.getDisposition(MMIXTailCallRequestKind::Ordinary),
            MMIXTailCallDisposition::TailTransfer);
  EXPECT_EQ(Result.getDisposition(MMIXTailCallRequestKind::Required),
            MMIXTailCallDisposition::TailTransfer);

  Input.CallerCC = CallingConv::Fast;
  Input.CalleeCC = CallingConv::Fast;
  Input.Callee = MMIXTailCallCalleeKind::Indirect;
  Input.IndirectResult = MMIXTailCallIndirectResultKind::Forwarded;
  Input.HasCallerCopy = true;
  Input.CallerCopySurvivesTransfer = true;
  Input.OutgoingStackBytes = 24;
  Input.ReusableIncomingStackBytes = 24;
  EXPECT_TRUE(classifyMMIXTailCall(Input).isEligible());
}

TEST(MMIXTailCallTest, ClassifiesCallingConventionAndVariadicBoundaries) {
  MMIXTailCallEligibilityInput Input = eligibleInput();
  Input.CallerCC = CallingConv::PreserveMost;
  expectReason(
      Input, MMIXTailCallEligibilityReason::UnsupportedCallerCallingConvention,
      "caller calling convention is unsupported");

  Input = eligibleInput();
  Input.CalleeCC = CallingConv::PreserveMost;
  expectReason(
      Input, MMIXTailCallEligibilityReason::UnsupportedCalleeCallingConvention,
      "callee calling convention is unsupported");

  Input = eligibleInput();
  Input.CalleeCC = CallingConv::Fast;
  expectReason(Input,
               MMIXTailCallEligibilityReason::MismatchedCallingConvention,
               "caller and callee calling conventions do not match");

  Input = eligibleInput();
  Input.CallerIsVarArg = true;
  expectReason(Input, MMIXTailCallEligibilityReason::VariadicCaller,
               "variadic caller is unsupported");

  Input = eligibleInput();
  Input.CalleeIsVarArg = true;
  expectReason(Input, MMIXTailCallEligibilityReason::VariadicCallee,
               "variadic callee is unsupported");
}

TEST(MMIXTailCallTest, ClassifiesABIAndStorageBoundaries) {
  MMIXTailCallEligibilityInput Input = eligibleInput();
  Input.ResultsAreCompatible = false;
  expectReason(Input, MMIXTailCallEligibilityReason::IncompatibleResults,
               "caller and callee result locations are incompatible");

  Input = eligibleInput();
  Input.ArgumentsAreCompatible = false;
  expectReason(Input, MMIXTailCallEligibilityReason::IncompatibleArguments,
               "callee argument locations are incompatible");

  Input = eligibleInput();
  Input.IndirectResult = MMIXTailCallIndirectResultKind::Incompatible;
  expectReason(Input, MMIXTailCallEligibilityReason::IncompatibleIndirectResult,
               "indirect result is not forwarded compatibly");

  Input = eligibleInput();
  Input.HasCallerCopy = true;
  expectReason(Input, MMIXTailCallEligibilityReason::EphemeralCallerCopy,
               "caller-copy storage does not survive the transfer");
}

TEST(MMIXTailCallTest, ClassifiesCalleeAndFrameBoundaries) {
  MMIXTailCallEligibilityInput Input = eligibleInput();
  Input.Callee = MMIXTailCallCalleeKind::Unsupported;
  expectReason(Input, MMIXTailCallEligibilityReason::UnsupportedCallee,
               "callee form is unsupported");

  Input = eligibleInput();
  Input.HasDynamicStack = true;
  expectReason(Input, MMIXTailCallEligibilityReason::DynamicStack,
               "dynamic stack allocation prevents frame reuse");

  Input = eligibleInput();
  Input.RequiresStackRealignment = true;
  expectReason(Input, MMIXTailCallEligibilityReason::StackRealignment,
               "stack realignment prevents frame reuse");

  Input = eligibleInput();
  Input.CanRestoreFrame = false;
  expectReason(Input, MMIXTailCallEligibilityReason::UnrestorableFrame,
               "software frame cannot be restored before transfer");
}

TEST(MMIXTailCallTest, ClassifiesReusableStackBoundaries) {
  MMIXTailCallEligibilityInput Input = eligibleInput();
  Input.OutgoingStackBytes = 9;
  Input.ReusableIncomingStackBytes = 16;
  expectReason(Input, MMIXTailCallEligibilityReason::MisalignedOutgoingStack,
               "outgoing stack argument area is misaligned");

  Input = eligibleInput();
  Input.OutgoingStackBytes = 24;
  Input.ReusableIncomingStackBytes = 16;
  expectReason(Input, MMIXTailCallEligibilityReason::InsufficientReusableStack,
               "outgoing stack arguments exceed the reusable incoming area");
}

TEST(MMIXTailCallTest, ReportsTheFirstSemanticFailure) {
  MMIXTailCallEligibilityInput Input = eligibleInput();
  Input.CallerCC = CallingConv::PreserveMost;
  Input.CalleeCC = CallingConv::Fast;
  Input.CallerIsVarArg = true;
  Input.ResultsAreCompatible = false;
  Input.Callee = MMIXTailCallCalleeKind::Unsupported;
  Input.CanRestoreFrame = false;
  Input.OutgoingStackBytes = 9;
  expectReason(
      Input, MMIXTailCallEligibilityReason::UnsupportedCallerCallingConvention,
      "caller calling convention is unsupported");
}

TEST(MMIXTailCallTest, ClassifiesMachineFrameState) {
  MMIXTailCallFrameState State;
  State.CanRestoreFrame = true;
  EXPECT_TRUE(State.isEligible());
  EXPECT_EQ(State.getReason(), MMIXTailCallEligibilityReason::Eligible);

  State.HasDynamicStack = true;
  EXPECT_EQ(State.getReason(), MMIXTailCallEligibilityReason::DynamicStack);

  State.RequiresStackRealignment = true;
  EXPECT_EQ(State.getReason(), MMIXTailCallEligibilityReason::DynamicStack);

  State.HasDynamicStack = false;
  EXPECT_EQ(State.getReason(), MMIXTailCallEligibilityReason::StackRealignment);

  State.RequiresStackRealignment = false;
  State.CanRestoreFrame = false;
  EXPECT_EQ(State.getReason(),
            MMIXTailCallEligibilityReason::UnrestorableFrame);
}

TEST(MMIXTailCallTest, MachinePseudosModelTerminalCallState) {
  std::unique_ptr<MCInstrInfo> MII{createMMIXMCInstrInfo()};
  static constexpr std::array Opcodes = {MMIX::INDIRECT_TAIL_STATE,
                                         MMIX::DIRECT_TAIL_STATE,
                                         MMIX::MATERIALIZED_DIRECT_TAIL_STATE};
  static constexpr std::array<MCPhysReg, 5> ExpectedUses = {
      MMIX::R254, MMIX::RJ, MMIX::RG, MMIX::RL, MMIX::RO};

  for (unsigned Opcode : Opcodes) {
    const MCInstrDesc &Desc = MII->get(Opcode);
    EXPECT_TRUE(Desc.isCall());
    EXPECT_TRUE(Desc.isReturn());
    EXPECT_TRUE(Desc.isTerminator());
    EXPECT_TRUE(Desc.isBarrier());
    EXPECT_TRUE(Desc.hasUnmodeledSideEffects());
    EXPECT_TRUE(Desc.isVariadic());
    EXPECT_FALSE(Desc.isBranch());
    EXPECT_TRUE(Desc.implicit_defs().empty());
    EXPECT_TRUE(llvm::equal(Desc.implicit_uses(), ExpectedUses));
  }
}

} // namespace
