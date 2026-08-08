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
#include "llvm/MC/MCExpr.h"
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

  static void expectSuccess(Error Result) {
    if (Result)
      ADD_FAILURE() << toString(std::move(Result));
  }

  static std::string lookup(const MMIXALAsmStreamer &Streamer,
                            const MCSymbol &Symbol) {
    Expected<StringRef> Result = Streamer.getMappedSymbol(Symbol);
    if (!Result) {
      ADD_FAILURE() << toString(Result.takeError());
      return {};
    }
    return Result->str();
  }

  static std::string lookupSourceAlias(const MMIXALAsmStreamer &Streamer,
                                       const MCSymbol &Symbol) {
    Expected<StringRef> Result = Streamer.getSourceBlockAlias(Symbol);
    if (!Result) {
      ADD_FAILURE() << toString(Result.takeError());
      return {};
    }
    return Result->str();
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

TEST_F(MMIXALAsmStreamerTest, RegistersSemanticAndPrivateSymbolIdentities) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Function = Context.getOrCreateSymbol("canonical_function");
  MCSymbol *Block = Context.getOrCreateSymbol(".Lcanonical_block");
  MCSymbol *ConstantPool = Context.getOrCreateSymbol(".Lcanonical_constant");
  MCSymbol *JumpTable = Context.getOrCreateSymbol(".Lcanonical_jump_table");
  MCSymbol *BlockAddress = Context.getOrCreateSymbol(".Lcanonical_address");
  MCSymbol *FunctionEnd = Context.getOrCreateSymbol(".Lcanonical_end");
  MCSymbol *ReferencedTemporary =
      Context.getOrCreateSymbol(".Lcanonical_reference");
  MCSymbol *Temporary = Context.getOrCreateSymbol(".Lcanonical_temporary");
  MCSymbol *ObservedModuleTemporary =
      Context.getOrCreateSymbol(".Lobserved_module_temporary");
  MCSymbol *ModuleTemporary =
      Context.getOrCreateSymbol(".Lcanonical_module_temporary");

  expectSuccess(Streamer->registerUserSymbol(*Function, "Main"));
  expectSuccess(Streamer->beginFunctionSymbols("Main"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Block, "Main",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerSourceBlock(*Block, "Main", "success"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*ConstantPool, "Main",
                                                        Kind::ConstantPool, 2));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*JumpTable, "Main",
                                                        Kind::JumpTable, 1));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*BlockAddress, "Main",
                                                        Kind::BlockAddress, 3));
  MCInst ReferencingInst;
  ReferencingInst.setOpcode(MMIX::JMP);
  ReferencingInst.addOperand(MCOperand::createExpr(
      MCSymbolRefExpr::create(ReferencedTemporary, Context)));
  Streamer->emitInstruction(ReferencingInst, *STI);
  Streamer->emitLabel(Temporary);
  expectSuccess(Streamer->endFunctionSymbols(FunctionEnd));
  Streamer->emitLabel(ObservedModuleTemporary);
  expectSuccess(Streamer->registerModulePrivateSymbol(*ModuleTemporary,
                                                      Kind::Temporary, 4));

  EXPECT_EQ(Streamer->getNumRegisteredSymbols(), 9u);
  EXPECT_EQ(Streamer->getNumSourceBlockAliases(), 1u);
  expectSuccess(Streamer->finalizeSymbolMappings());

  EXPECT_EQ(lookup(*Streamer, *Function), "Main");
  EXPECT_EQ(lookup(*Streamer, *Block), "__LLVM_L_F_4D61696E_BB_0");
  EXPECT_EQ(lookupSourceAlias(*Streamer, *Block), "success");
  EXPECT_EQ(lookup(*Streamer, *ConstantPool), "__LLVM_L_F_4D61696E_CP_2");
  EXPECT_EQ(lookup(*Streamer, *JumpTable), "__LLVM_L_F_4D61696E_JT_1");
  EXPECT_EQ(lookup(*Streamer, *BlockAddress), "__LLVM_L_F_4D61696E_BA_3");
  EXPECT_EQ(lookup(*Streamer, *FunctionEnd), "__LLVM_L_F_4D61696E_END_0");
  EXPECT_EQ(lookup(*Streamer, *ReferencedTemporary),
            "__LLVM_L_F_4D61696E_TMP_0");
  EXPECT_EQ(lookup(*Streamer, *Temporary), "__LLVM_L_F_4D61696E_TMP_1");
  EXPECT_EQ(lookup(*Streamer, *ObservedModuleTemporary), "__LLVM_M_TMP_0");
  EXPECT_EQ(lookup(*Streamer, *ModuleTemporary), "__LLVM_M_TMP_4");
  EXPECT_EQ(Streamer->getNumRegisteredSymbols(), 10u);
  EXPECT_EQ(Streamer->getNumBufferedEvents(), 3u);
  EXPECT_TRUE(Output.empty());

  Streamer->reset();
  Streamer->finish();
}

TEST_F(MMIXALAsmStreamerTest, ResolvesUserAndSourceBlockNameClaimsTogether) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Global = Context.getOrCreateSymbol("canonical_global");
  MCSymbol *FirstBlock = Context.getOrCreateSymbol(".Lfirst_block");
  MCSymbol *SecondBlock = Context.getOrCreateSymbol(".Lsecond_block");
  expectSuccess(Streamer->registerUserSymbol(*Global, "shared"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*FirstBlock, "first",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerSourceBlock(*FirstBlock, "first", "shared"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*SecondBlock, "second",
                                                        Kind::BasicBlock, 0));
  expectSuccess(
      Streamer->registerSourceBlock(*SecondBlock, "second", "shared"));
  expectSuccess(Streamer->finalizeSymbolMappings());

  EXPECT_EQ(lookup(*Streamer, *Global), "__LLVM_U_6_736861726564");
  EXPECT_EQ(lookupSourceAlias(*Streamer, *FirstBlock),
            "__LLVM_B_F_6669727374_B_736861726564");
  EXPECT_EQ(lookupSourceAlias(*Streamer, *SecondBlock),
            "__LLVM_B_F_7365636F6E64_B_736861726564");

  Streamer->reset();
  Streamer->finish();
}

TEST_F(MMIXALAsmStreamerTest, DiagnosesPrivateMappingCollision) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *First = Context.getOrCreateSymbol(".Lfirst");
  MCSymbol *Second = Context.getOrCreateSymbol(".Lsecond");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*First, "function",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Second, "function",
                                                        Kind::BasicBlock, 0));

  Error Collision = Streamer->finalizeSymbolMappings();
  ASSERT_TRUE(static_cast<bool>(Collision));
  EXPECT_EQ(toString(std::move(Collision)), "MMIXAL symbol mapping collision: "
                                            "__LLVM_L_F_66756E6374696F6E_BB_0");

  Streamer->reset();
  Streamer->finish();
}

TEST_F(MMIXALAsmStreamerTest, DiagnosesSourceBlockFallbackCollision) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *First = Context.getOrCreateSymbol(".Lfirst");
  MCSymbol *Second = Context.getOrCreateSymbol(".Lsecond");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*First, "function",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Second, "function",
                                                        Kind::BasicBlock, 1));
  expectSuccess(Streamer->registerSourceBlock(*First, "function", "block"));
  expectSuccess(Streamer->registerSourceBlock(*Second, "function", "block"));

  Error Collision = Streamer->finalizeSymbolMappings();
  ASSERT_TRUE(static_cast<bool>(Collision));
  EXPECT_EQ(toString(std::move(Collision)),
            "MMIXAL source-block mapping collision: "
            "__LLVM_B_F_66756E6374696F6E_B_626C6F636B");

  Streamer->reset();
  Streamer->finish();
}

TEST_F(MMIXALAsmStreamerTest, RegistrationMetadataDoesNotBufferOutputEvents) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *User = Context.getOrCreateSymbol("canonical_user");
  MCSymbol *Private = Context.getOrCreateSymbol(".Lprivate");
  expectSuccess(Streamer->registerUserSymbol(*User, "user"));
  expectSuccess(
      Streamer->registerModulePrivateSymbol(*Private, Kind::Temporary, 0));
  EXPECT_EQ(Streamer->getNumBufferedEvents(), 0u);

  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_TRUE(Output.empty());
}

} // namespace
