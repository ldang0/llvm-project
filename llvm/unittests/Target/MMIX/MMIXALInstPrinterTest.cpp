//===- MMIXALInstPrinterTest.cpp - MMIXAL printer unit tests -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALInstPrinter.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCAsmInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>

using namespace llvm;

extern "C" void LLVMInitializeMMIXTargetInfo();
extern "C" void LLVMInitializeMMIXTargetMC();

class MMIXALInstPrinterTest : public testing::Test {
protected:
  Triple TT{"mmix-unknown-elf"};
  MCTargetOptions Options;
  MMIXMCAsmInfo MAI{TT, Options};
  std::unique_ptr<MCInstrInfo> MII{createMMIXMCInstrInfo()};
  std::unique_ptr<MCRegisterInfo> MRI{createMMIXMCRegisterInfo(TT)};
  std::unique_ptr<MCSubtargetInfo> STI{
      createMMIXMCSubtargetInfo(TT, "generic", "")};
  MMIXALInstPrinter Printer{MAI, *MII, *MRI};

  std::string print(unsigned Opcode,
                    std::initializer_list<MCOperand> Operands) {
    MCInst Inst;
    Inst.setOpcode(Opcode);
    for (const MCOperand &Operand : Operands)
      Inst.addOperand(Operand);

    std::string Output;
    raw_string_ostream OS(Output);
    Printer.printInst(&Inst, 0, "", *STI, OS);
    OS.flush();
    return Output;
  }
};

TEST_F(MMIXALInstPrinterTest, PrintsGeneralAndFloatingRegisterBoundaries) {
  EXPECT_EQ(print(MMIX::ADD, {MCOperand::createReg(MMIX::R0),
                              MCOperand::createReg(MMIX::R255),
                              MCOperand::createReg(MMIX::R1)}),
            "\tADD $0, $255, $1");
  EXPECT_EQ(print(MMIX::FADD, {MCOperand::createReg(MMIX::R255),
                               MCOperand::createReg(MMIX::R0),
                               MCOperand::createReg(MMIX::R255)}),
            "\tFADD $255, $0, $255");
}

TEST_F(MMIXALInstPrinterTest, PrintsEverySpecialRegisterName) {
  static constexpr MCRegister Registers[] = {
      MMIX::RB,  MMIX::RD,  MMIX::RE,  MMIX::RH, MMIX::RJ, MMIX::RM, MMIX::RR,
      MMIX::RBB, MMIX::RC,  MMIX::RN,  MMIX::RO, MMIX::RS, MMIX::RI, MMIX::RT,
      MMIX::RTT, MMIX::RK,  MMIX::RQ,  MMIX::RU, MMIX::RV, MMIX::RG, MMIX::RL,
      MMIX::RA,  MMIX::RF,  MMIX::RP,  MMIX::RW, MMIX::RX, MMIX::RY, MMIX::RZ,
      MMIX::RWW, MMIX::RXX, MMIX::RYY, MMIX::RZZ};
  static constexpr const char *Names[] = {
      "rB", "rD", "rE", "rH",  "rJ", "rM", "rR",  "rBB", "rC",  "rN", "rO",
      "rS", "rI", "rT", "rTT", "rK", "rQ", "rU",  "rV",  "rG",  "rL", "rA",
      "rF", "rP", "rW", "rX",  "rY", "rZ", "rWW", "rXX", "rYY", "rZZ"};
  static_assert(std::size(Registers) == std::size(Names));

  for (size_t I = 0; I != std::size(Registers); ++I)
    EXPECT_EQ(print(MMIX::GET, {MCOperand::createReg(MMIX::R0),
                                MCOperand::createReg(Registers[I])}),
              std::string("\tGET $0, ") + Names[I]);
}

TEST_F(MMIXALInstPrinterTest, PrintsUnsignedScalarBoundaries) {
  EXPECT_EQ(print(MMIX::ADDI,
                  {MCOperand::createReg(MMIX::R0),
                   MCOperand::createReg(MMIX::R255), MCOperand::createImm(0)}),
            "\tADD $0, $255, 0");
  EXPECT_EQ(print(MMIX::ADDI,
                  {MCOperand::createReg(MMIX::R255),
                   MCOperand::createReg(MMIX::R0), MCOperand::createImm(255)}),
            "\tADD $255, $0, 255");
  EXPECT_EQ(print(MMIX::SETH,
                  {MCOperand::createReg(MMIX::R0), MCOperand::createImm(0)}),
            "\tSETH $0, 0");
  EXPECT_EQ(print(MMIX::SETH, {MCOperand::createReg(MMIX::R255),
                               MCOperand::createImm(65535)}),
            "\tSETH $255, 65535");
  EXPECT_EQ(print(MMIX::POP,
                  {MCOperand::createImm(255), MCOperand::createImm(65535)}),
            "\tPOP 255, 65535");
}

TEST_F(MMIXALInstPrinterTest, PreservesMixedRegisterAndImmediateKinds) {
  EXPECT_EQ(print(MMIX::SWYM,
                  {MCOperand::createReg(MMIX::R7), MCOperand::createImm(7),
                   MCOperand::createReg(MMIX::R255)}),
            "\tSWYM $7, 7, $255");
}

TEST_F(MMIXALInstPrinterTest, PrintsEveryRoundingMode) {
  static constexpr const char *Names[] = {
      "ROUND_CURRENT", "ROUND_OFF", "ROUND_UP", "ROUND_DOWN", "ROUND_NEAR"};

  for (int64_t Mode = 0; Mode != static_cast<int64_t>(std::size(Names)); ++Mode)
    EXPECT_EQ(print(MMIX::FSQRT,
                    {MCOperand::createReg(MMIX::R0), MCOperand::createImm(Mode),
                     MCOperand::createReg(MMIX::R255)}),
              std::string("\tFSQRT $0, ") + Names[Mode] + ", $255");
}

TEST_F(MMIXALInstPrinterTest, PrintsSynchronizationAndResumeModes) {
  for (int64_t Mode = 0; Mode != 8; ++Mode)
    EXPECT_EQ(print(MMIX::SYNC, {MCOperand::createImm(Mode)}),
              "\tSYNC " + std::to_string(Mode));

  EXPECT_EQ(print(MMIX::RESUME, {MCOperand::createImm(0)}), "\tRESUME 0");
  EXPECT_EQ(print(MMIX::RESUME, {MCOperand::createImm(1)}), "\tRESUME 1");
}

TEST_F(MMIXALInstPrinterTest, RejectsOutOfRangeScalarOperands) {
  EXPECT_DEATH(print(MMIX::ADDI, {MCOperand::createReg(MMIX::R0),
                                  MCOperand::createReg(MMIX::R1),
                                  MCOperand::createImm(-1)}),
               "invalid MMIXAL byte operand");
  EXPECT_DEATH(print(MMIX::ADDI, {MCOperand::createReg(MMIX::R0),
                                  MCOperand::createReg(MMIX::R1),
                                  MCOperand::createImm(256)}),
               "invalid MMIXAL byte operand");
  EXPECT_DEATH(print(MMIX::SETH, {MCOperand::createReg(MMIX::R0),
                                  MCOperand::createImm(65536)}),
               "invalid MMIXAL wyde operand");
  EXPECT_DEATH(print(MMIX::FSQRT,
                     {MCOperand::createReg(MMIX::R0), MCOperand::createImm(5),
                      MCOperand::createReg(MMIX::R1)}),
               "invalid MMIXAL rounding mode");
  EXPECT_DEATH(print(MMIX::SYNC, {MCOperand::createImm(8)}),
               "invalid MMIXAL synchronization mode");
  EXPECT_DEATH(print(MMIX::RESUME, {MCOperand::createImm(2)}),
               "invalid MMIXAL resume mode");
  EXPECT_DEATH(
      print(MMIX::SWYM, {MCOperand::createReg(MMIX::RB),
                         MCOperand::createImm(0), MCOperand::createImm(0)}),
      "invalid MMIXAL general register operand");
  EXPECT_DEATH(print(MMIX::GET, {MCOperand::createReg(MMIX::R0),
                                 MCOperand::createReg(MMIX::R0)}),
               "invalid MMIXAL special register operand");
}

TEST(MMIXALInstPrinterFactoryTest, PublicFactoryRejectsMMIXALVariant) {
  LLVMInitializeMMIXTargetInfo();
  LLVMInitializeMMIXTargetMC();

  const Triple TT("mmix-unknown-elf");
  const MCTargetOptions Options;
  const MMIXMCAsmInfo MAI(TT, Options);
  const std::unique_ptr<MCInstrInfo> MII(createMMIXMCInstrInfo());
  const std::unique_ptr<MCRegisterInfo> MRI(createMMIXMCRegisterInfo(TT));
  const Target &T = getTheMMIXTarget();

  std::unique_ptr<MCInstPrinter> Canonical(
      T.createMCInstPrinter(TT, MMIXII::CanonicalAsmVariant, MAI, *MII, *MRI));
  std::unique_ptr<MCInstPrinter> MMIXAL(
      T.createMCInstPrinter(TT, MMIXII::MMIXALAsmVariant, MAI, *MII, *MRI));

  EXPECT_NE(Canonical, nullptr);
  EXPECT_EQ(MMIXAL, nullptr);
}
