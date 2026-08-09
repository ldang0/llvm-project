//===-- MMIXELFObjectWriterTest.cpp - MMIX ELF writer tests --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/Support/Casting.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

TEST(MMIXELFObjectWriterTest, UsesContractedELFIdentity) {
  std::unique_ptr<MCObjectTargetWriter> Writer = createMMIXELFObjectWriter();
  auto *ELFWriter = dyn_cast<MCELFObjectTargetWriter>(Writer.get());

  ASSERT_NE(ELFWriter, nullptr);
  EXPECT_TRUE(ELFWriter->is64Bit());
  EXPECT_EQ(ELFWriter->getOSABI(), ELF::ELFOSABI_NONE);
  EXPECT_EQ(ELFWriter->getABIVersion(), 0);
  EXPECT_EQ(ELFWriter->getEMachine(), ELF::EM_MMIX);
  EXPECT_TRUE(ELFWriter->hasRelocationAddend());
}

} // namespace
