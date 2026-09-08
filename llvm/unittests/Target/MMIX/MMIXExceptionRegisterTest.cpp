//===- MMIXExceptionRegisterTest.cpp - MMIX landing-pad registers ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXISelLowering.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXSubtarget.h"
#include "MMIXTargetMachine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "gtest/gtest.h"

using namespace llvm;

TEST(MMIXExceptionRegisters, DwarfLandingPadContract) {
  LLVMInitializeMMIXTargetInfo();
  LLVMInitializeMMIXTarget();
  LLVMInitializeMMIXTargetMC();
  Triple TT("mmix-unknown-unknown");
  std::string Error;
  const Target *T = TargetRegistry::lookupTarget("", TT, Error);
  ASSERT_NE(T, nullptr) << Error;
  std::unique_ptr<TargetMachine> TM(T->createTargetMachine(
      TT, "generic", "", TargetOptions(), Reloc::Static));
  ASSERT_NE(TM, nullptr);
  LLVMContext Ctx;
  Module M("landing-pad", Ctx);
  Function *F = Function::Create(FunctionType::get(Type::getVoidTy(Ctx), false),
                                 GlobalValue::ExternalLinkage, "f", M);
  const auto *ST = TM->getSubtargetImpl(*F);
  const auto *TLI = ST->getTargetLowering();
  const auto *TRI = ST->getRegisterInfo();
  const Register Pointer =
      TLI->getExceptionPointerRegister(ExceptionHandling::DwarfCFI, nullptr);
  const Register Selector =
      TLI->getExceptionSelectorRegister(ExceptionHandling::DwarfCFI, nullptr);
  EXPECT_EQ(Pointer, Register(MMIX::R231));
  EXPECT_EQ(Selector, Register(MMIX::R232));
  EXPECT_EQ(TRI->getDwarfRegNum(Pointer, true), 7);
  EXPECT_EQ(TRI->getDwarfRegNum(Selector, true), 8);
  EXPECT_TRUE(MMIX::GPR64CodeGenRegClass.contains(Pointer));
  EXPECT_TRUE(MMIX::GPR64CodeGenRegClass.contains(Selector));
  for (auto EH : {ExceptionHandling::None, ExceptionHandling::SjLj,
                  ExceptionHandling::WinEH, ExceptionHandling::ARM}) {
    EXPECT_FALSE(TLI->getExceptionPointerRegister(EH, nullptr));
    EXPECT_FALSE(TLI->getExceptionSelectorRegister(EH, nullptr));
  }
}
