//===- MMIXCallEmissionTest.cpp - MMIX call emission tests ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXCallEmission.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXFixupKinds.h"
#include "MCTargetDesc/MMIXMCAsmInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCValue.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>

using namespace llvm;

namespace {

class MMIXCallEmissionTest : public testing::Test {
protected:
  Triple TT{"mmix-unknown-elf"};
  MCTargetOptions Options;
  MMIXMCAsmInfo MAI{TT, Options};
  std::unique_ptr<MCInstrInfo> MII{createMMIXMCInstrInfo()};
  std::unique_ptr<MCRegisterInfo> MRI{createMMIXMCRegisterInfo(TT)};
  std::unique_ptr<MCSubtargetInfo> STI{
      createMMIXMCSubtargetInfo(TT, "generic", "")};
  MCContext Ctx{TT, MAI, *MRI, *STI};

  const MCExpr *symbol(StringRef Name) {
    return MCSymbolRefExpr::create(Ctx.getOrCreateSymbol(Name), Ctx);
  }
};

TEST_F(MMIXCallEmissionTest, TextOutputsKeepConservativeCallStrategy) {
  static constexpr std::array Modes = {MMIXEmissionMode::CanonicalAssembly,
                                       MMIXEmissionMode::MMIXALAssembly};
  for (MMIXEmissionMode Mode : Modes)
    EXPECT_FALSE(
        createMMIXUnresolvedDirectCall(Mode, MMIX::R31, symbol("callee")));
}

TEST_F(MMIXCallEmissionTest, ELFObjectUsesStubbableDirectCall) {
  const MCExpr *Callee = MCBinaryExpr::createAdd(
      symbol("callee"), MCConstantExpr::create(-12, Ctx), Ctx);
  std::optional<MCInst> Call = createMMIXUnresolvedDirectCall(
      MMIXEmissionMode::ELFObject, MMIX::R31, Callee);

  ASSERT_TRUE(Call);
  EXPECT_EQ(Call->getOpcode(), MMIX::PUSHJ);
  EXPECT_EQ(Call->getFlags(), MMIXII::DirectionNeutralCall);
  ASSERT_EQ(Call->getNumOperands(), 2u);
  EXPECT_EQ(Call->getOperand(0).getReg(), MMIX::R31);
  EXPECT_EQ(Call->getOperand(1).getExpr(), Callee);

  std::unique_ptr<MCCodeEmitter> Emitter{createMMIXMCCodeEmitter(*MII, Ctx)};
  SmallVector<char, 4> Bytes;
  SmallVector<MCFixup, 1> Fixups;
  Emitter->encodeInstruction(*Call, Bytes, Fixups, *STI);

  static constexpr std::array<unsigned char, 4> ExpectedBytes = {0xf2, 0x1f,
                                                                 0x00, 0x00};
  ASSERT_EQ(Bytes.size(), ExpectedBytes.size());
  for (unsigned I = 0; I != Bytes.size(); ++I)
    EXPECT_EQ(static_cast<unsigned char>(Bytes[I]), ExpectedBytes[I]);

  ASSERT_EQ(Fixups.size(), 1u);
  EXPECT_EQ(Fixups.front().getKind(), MMIX::fixup_mmix_direction_neutral_call);
  EXPECT_TRUE(Fixups.front().isPCRel());
  MCValue Value;
  ASSERT_TRUE(Fixups.front().getValue()->evaluateAsRelocatable(Value, nullptr));
  ASSERT_NE(Value.getAddSym(), nullptr);
  EXPECT_EQ(Value.getAddSym()->getName(), "callee");
  EXPECT_EQ(Value.getSubSym(), nullptr);
  EXPECT_EQ(Value.getConstant(), -12);
}

} // namespace
