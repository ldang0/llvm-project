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
#include <memory>
#include <string>

using namespace llvm;

extern "C" void LLVMInitializeMMIXTargetInfo();
extern "C" void LLVMInitializeMMIXTargetMC();

TEST(MMIXALInstPrinterTest, PrinterIsDirectlyCallable) {
  const Triple TT("mmix-unknown-elf");
  const MCTargetOptions Options;
  const MMIXMCAsmInfo MAI(TT, Options);
  const std::unique_ptr<MCInstrInfo> MII(createMMIXMCInstrInfo());
  const std::unique_ptr<MCRegisterInfo> MRI(createMMIXMCRegisterInfo(TT));
  const std::unique_ptr<MCSubtargetInfo> STI(
      createMMIXMCSubtargetInfo(TT, "generic", ""));
  MMIXALInstPrinter Printer(MAI, *MII, *MRI);

  MCInst Inst;
  Inst.setOpcode(MMIX::ADD);
  Inst.addOperand(MCOperand::createReg(MMIX::R1));
  Inst.addOperand(MCOperand::createReg(MMIX::R2));
  Inst.addOperand(MCOperand::createReg(MMIX::R3));

  std::string Output;
  raw_string_ostream OS(Output);
  Printer.printInst(&Inst, 0, "", *STI, OS);
  EXPECT_EQ(Output, "\tADD r1, r2, r3");
}

TEST(MMIXALInstPrinterTest, PublicFactoryRejectsMMIXALVariant) {
  LLVMInitializeMMIXTargetInfo();
  LLVMInitializeMMIXTargetMC();

  const Triple TT("mmix-unknown-elf");
  const MCTargetOptions Options;
  const MMIXMCAsmInfo MAI(TT, Options);
  const std::unique_ptr<MCInstrInfo> MII(createMMIXMCInstrInfo());
  const std::unique_ptr<MCRegisterInfo> MRI(createMMIXMCRegisterInfo(TT));
  const Target &T = getTheMMIXTarget();

  std::unique_ptr<MCInstPrinter> Canonical(T.createMCInstPrinter(
      TT, MMIXII::CanonicalAsmVariant, MAI, *MII, *MRI));
  std::unique_ptr<MCInstPrinter> MMIXAL(T.createMCInstPrinter(
      TT, MMIXII::MMIXALAsmVariant, MAI, *MII, *MRI));

  EXPECT_NE(Canonical, nullptr);
  EXPECT_EQ(MMIXAL, nullptr);
}
