//===- MMIXAddressEmissionTest.cpp - MMIX address emission tests --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXAddressEmission.h"
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
#include "llvm/Support/Casting.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>

using namespace llvm;

namespace {

class MMIXAddressEmissionTest : public testing::Test {
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

  static int64_t absoluteValue(const MCExpr *Expr) {
    int64_t Value = 0;
    EXPECT_TRUE(Expr->evaluateAsAbsolute(Value));
    return Value;
  }
};

TEST_F(MMIXAddressEmissionTest, MMIXALUsesSplitAddressSequence) {
  static constexpr std::array<unsigned, 4> Opcodes = {MMIX::SETH, MMIX::INCMH,
                                                      MMIX::INCML, MMIX::INCL};
  static constexpr std::array<unsigned, 4> Shifts = {48, 32, 16, 0};
  const MCExpr *Address = symbol("target");

  SmallVector<MCInst, 4> Sequence = createMMIXStaticAddressSequence(
      MMIXEmissionMode::MMIXALAssembly, MMIX::R7, Address, Ctx);
  ASSERT_EQ(Sequence.size(), Opcodes.size());
  for (unsigned I = 0; I != Sequence.size(); ++I) {
    const MCInst &Inst = Sequence[I];
    EXPECT_EQ(Inst.getOpcode(), Opcodes[I]);
    ASSERT_EQ(Inst.getNumOperands(), 2u);
    EXPECT_EQ(Inst.getOperand(0).getReg(), MMIX::R7);

    const auto *Mask = dyn_cast<MCBinaryExpr>(Inst.getOperand(1).getExpr());
    ASSERT_NE(Mask, nullptr);
    EXPECT_EQ(Mask->getOpcode(), MCBinaryExpr::And);
    EXPECT_EQ(absoluteValue(Mask->getRHS()), 0xffff);
    if (!Shifts[I]) {
      EXPECT_EQ(Mask->getLHS(), Address);
      continue;
    }

    const auto *Shift = dyn_cast<MCBinaryExpr>(Mask->getLHS());
    ASSERT_NE(Shift, nullptr);
    EXPECT_EQ(Shift->getOpcode(), MCBinaryExpr::LShr);
    EXPECT_EQ(Shift->getLHS(), Address);
    EXPECT_EQ(absoluteValue(Shift->getRHS()), Shifts[I]);
  }
}

TEST_F(MMIXAddressEmissionTest, CanonicalELFUsesGETAReservationContract) {
  const MCExpr *Address = MCBinaryExpr::createAdd(
      symbol("target"), MCConstantExpr::create(-16, Ctx), Ctx);
  for (MMIXEmissionMode Mode : {MMIXEmissionMode::CanonicalAssembly,
                                MMIXEmissionMode::ELFObject}) {
    SmallVector<MCInst, 4> Sequence =
        createMMIXStaticAddressSequence(Mode, MMIX::R9, Address, Ctx);

    ASSERT_EQ(Sequence.size(), 1u);
    const MCInst &Inst = Sequence.front();
    EXPECT_EQ(Inst.getOpcode(), MMIX::GETA);
    ASSERT_EQ(Inst.getNumOperands(), 3u);
    EXPECT_EQ(Inst.getOperand(0).getReg(), MMIX::R9);
    const auto *Specifier =
        dyn_cast<MCSpecifierExpr>(Inst.getOperand(1).getExpr());
    ASSERT_NE(Specifier, nullptr);
    EXPECT_EQ(Specifier->getSpecifier(), MMIXII::S_GETA);
    EXPECT_EQ(Specifier->getSubExpr(), Address);
    EXPECT_EQ(Inst.getOperand(2).getImm(),
              MMIXII::GETARelocationReservedSlots);
    EXPECT_EQ((1 + MMIXII::GETARelocationReservedSlots) * 4, 16u);

    std::unique_ptr<MCCodeEmitter> Emitter{createMMIXMCCodeEmitter(*MII, Ctx)};
    SmallVector<char, 16> Bytes;
    SmallVector<MCFixup, 1> Fixups;
    Emitter->encodeInstruction(Inst, Bytes, Fixups, *STI);

    static constexpr std::array<unsigned char, 16> ExpectedBytes = {
        0xf4, 0x09, 0x00, 0x00, 0xfd, 0x00, 0x00, 0x00,
        0xfd, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x00, 0x00};
    ASSERT_EQ(Bytes.size(), ExpectedBytes.size());
    for (unsigned I = 0; I != Bytes.size(); ++I)
      EXPECT_EQ(static_cast<unsigned char>(Bytes[I]), ExpectedBytes[I]);

    ASSERT_EQ(Fixups.size(), 1u);
    EXPECT_EQ(Fixups.front().getKind(), MMIX::fixup_mmix_geta);
    EXPECT_TRUE(Fixups.front().isPCRel());
    EXPECT_TRUE(Fixups.front().isLinkerRelaxable());
    MCValue Value;
    ASSERT_TRUE(
        Fixups.front().getValue()->evaluateAsRelocatable(Value, nullptr));
    ASSERT_NE(Value.getAddSym(), nullptr);
    EXPECT_EQ(Value.getAddSym()->getName(), "target");
    EXPECT_EQ(Value.getSubSym(), nullptr);
    EXPECT_EQ(Value.getConstant(), -16);
  }
}

} // namespace
