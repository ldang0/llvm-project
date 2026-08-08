//===- MMIXALAsmStreamerTest.cpp - MMIXAL streamer unit tests ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALAsmStreamer.h"
#include "MCTargetDesc/MMIXALInstPrinter.h"
#include "MCTargetDesc/MMIXMCAsmInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <memory>
#include <string>

using namespace llvm;

namespace {

class MMIXALAsmStreamerTest : public testing::Test {
protected:
  Triple TT{"mmix-unknown-elf"};
  MCTargetOptions Options;
  MMIXMCAsmInfo MAI{TT, Options};
  std::unique_ptr<MCInstrInfo> MII{createMMIXMCInstrInfo()};
  std::unique_ptr<MCRegisterInfo> MRI{createMMIXMCRegisterInfo(TT)};
  std::unique_ptr<MCSubtargetInfo> STI{
      createMMIXMCSubtargetInfo(TT, "generic", "")};

  std::unique_ptr<MMIXALAsmStreamer> createStreamer(MCContext &Context,
                                                    raw_ostream &Output) {
    return std::make_unique<MMIXALAsmStreamer>(
        Context, std::make_unique<formatted_raw_ostream>(Output),
        std::make_unique<MMIXALInstPrinter>(MAI, *MII, *MRI));
  }

  static void captureDiagnostic(MCContext &Context, std::string &Message) {
    Context.setDiagnosticHandler([&Message](const SMDiagnostic &Diagnostic,
                                            bool, const SourceMgr &,
                                            std::vector<const MDNode *> &) {
      Message = Diagnostic.getMessage().str();
    });
  }
};

TEST_F(MMIXALAsmStreamerTest, EmptyStreamFinishesWithoutOutput) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Streamer->getNumBufferedEvents(), 0u);
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, NonEmptyStreamFailsAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCInst Inst;
  Inst.setOpcode(MMIX::ADD);
  Streamer->emitBytes("data");
  Streamer->emitInstruction(Inst, *STI);
  MCSymbol *Label = Context.getOrCreateSymbol("label");
  Streamer->emitLabel(Label);
  EXPECT_TRUE(Streamer->emitSymbolAttribute(Label, MCSA_Global));
  Streamer->emitCommonSymbol(Context.getOrCreateSymbol("common"), 8, Align(8));
  ASSERT_EQ(Streamer->getNumBufferedEvents(), 5u);

  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "MMIXAL buffered module emission is not implemented");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, ResetDiscardsBufferedEvents) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->emitBytes("discarded");
  ASSERT_EQ(Streamer->getNumBufferedEvents(), 1u);
  Streamer->reset();
  ASSERT_EQ(Streamer->getNumBufferedEvents(), 0u);

  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, InstancesKeepIndependentBuffers) {
  MCContext FirstContext(TT, MAI, *MRI, *STI);
  MCContext SecondContext(TT, MAI, *MRI, *STI);
  std::string FirstDiagnostic;
  std::string SecondDiagnostic;
  captureDiagnostic(FirstContext, FirstDiagnostic);
  captureDiagnostic(SecondContext, SecondDiagnostic);
  std::string FirstOutput;
  std::string SecondOutput;
  raw_string_ostream FirstOutputOS(FirstOutput);
  raw_string_ostream SecondOutputOS(SecondOutput);
  auto First = createStreamer(FirstContext, FirstOutputOS);
  auto Second = createStreamer(SecondContext, SecondOutputOS);

  First->emitBytes("first-only");
  ASSERT_EQ(First->getNumBufferedEvents(), 1u);
  ASSERT_EQ(Second->getNumBufferedEvents(), 0u);

  Second->finish();
  First->finish();

  EXPECT_FALSE(SecondContext.hadError());
  EXPECT_TRUE(SecondDiagnostic.empty());
  EXPECT_TRUE(SecondOutput.empty());
  EXPECT_TRUE(FirstContext.hadError());
  EXPECT_EQ(FirstDiagnostic,
            "MMIXAL buffered module emission is not implemented");
  EXPECT_TRUE(FirstOutput.empty());
}

} // namespace
