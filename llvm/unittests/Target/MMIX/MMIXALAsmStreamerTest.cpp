//===- MMIXALAsmStreamerTest.cpp - MMIXAL streamer unit tests ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALAsmStreamer.h"
#include "MCTargetDesc/MMIXALInstPrinter.h"
#include "MCTargetDesc/MMIXALStartup.h"
#include "MCTargetDesc/MMIXMCAsmInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <functional>
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

  static MCSectionELF *getSection(MCContext &Context, StringRef Name,
                                  unsigned Type, unsigned Flags,
                                  unsigned EntrySize = 0) {
    return Context.getELFSection(Name, Type, Flags, EntrySize);
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

TEST_F(MMIXALAsmStreamerTest, LateLinkageEventFailsAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));

  MCSymbol *Label = Context.getOrCreateSymbol("canonical_label");
  expectSuccess(Streamer->registerUserSymbol(*Label, "label"));
  Streamer->emitLabel(Label);
  MCInst Add;
  Add.setOpcode(MMIX::ADD);
  Add.addOperand(MCOperand::createReg(MMIX::R1));
  Add.addOperand(MCOperand::createReg(MMIX::R2));
  Add.addOperand(MCOperand::createReg(MMIX::R3));
  Streamer->emitInstruction(Add, *STI);
  EXPECT_TRUE(Streamer->emitSymbolAttribute(Label, MCSA_Global));

  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL does not support symbol linkage or visibility events");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, ResetDiscardsBufferedEvents) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));

  Streamer->emitBytes("discarded");
  ASSERT_EQ(Streamer->getNumBufferedEvents(), 2u);
  Streamer->reset();
  ASSERT_EQ(Streamer->getNumBufferedEvents(), 0u);

  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, EmitsPreludeBeforeAllocatedItems) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->addPreludeGlobalRegister("__LLVM_G_SP", 254,
                                     UINT64_C(0x2000000004000000));
  Streamer->addPreludeGlobalRegister("__LLVM_G_FP", 253, 0);
  Streamer->finalizePrelude();
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitIntValue(1, 1);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Streamer->getNumPreludeGlobalRegisters(), 2u);
  EXPECT_EQ(Output, "__LLVM_G_SP\tGREG #2000000004000000\n"
                    "__LLVM_G_FP\tGREG 0\n"
                    "\tLOC #2000000000000000\n"
                    "\tBYTE #01\n");
}

TEST_F(MMIXALAsmStreamerTest, ResetDiscardsPrelude) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->addPreludeGlobalRegister("__LLVM_G_SP", 254,
                                     UINT64_C(0x2000000004000000));
  Streamer->finalizePrelude();
  ASSERT_EQ(Streamer->getNumPreludeGlobalRegisters(), 1u);
  Streamer->reset();
  ASSERT_EQ(Streamer->getNumPreludeGlobalRegisters(), 0u);

  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsInvalidPreludeDeclarationsAtomically) {
  struct PreludeCase {
    const char *Diagnostic;
    std::function<void(MMIXALAsmStreamer &)> AddInvalidDeclaration;
  };
  const PreludeCase Cases[] = {
      {"duplicate target-owned MMIXAL prelude name: __LLVM_G_SP",
       [](MMIXALAsmStreamer &Streamer) {
         Streamer.addPreludeGlobalRegister("__LLVM_G_SP", 254, 0);
         Streamer.addPreludeGlobalRegister("__LLVM_G_SP", 253, 0);
       }},
      {"MMIXAL GREG declaration for $252 is out of allocation order; expected "
       "$253",
       [](MMIXALAsmStreamer &Streamer) {
         Streamer.addPreludeGlobalRegister("__LLVM_G_SP", 254, 0);
         Streamer.addPreludeGlobalRegister("__LLVM_G_FP", 252, 0);
       }},
      {"cannot add an MMIXAL prelude declaration after finalization",
       [](MMIXALAsmStreamer &Streamer) {
         Streamer.finalizePrelude();
         Streamer.addPreludeGlobalRegister("__LLVM_G_SP", 254, 0);
       }},
  };

  for (const PreludeCase &Case : Cases) {
    SCOPED_TRACE(Case.Diagnostic);
    MCContext Context(TT, MAI, *MRI, *STI);
    std::string Diagnostic;
    captureDiagnostic(Context, Diagnostic);
    std::string Output;
    raw_string_ostream OutputOS(Output);
    auto Streamer = createStreamer(Context, OutputOS);

    Case.AddInvalidDeclaration(*Streamer);
    Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                       ELF::SHF_ALLOC | ELF::SHF_WRITE));
    Streamer->emitIntValue(1, 1);
    Streamer->finish();

    EXPECT_TRUE(Context.hadError());
    EXPECT_EQ(Diagnostic, Case.Diagnostic);
    EXPECT_TRUE(Output.empty());
  }
}

TEST_F(MMIXALAsmStreamerTest, LateModuleFailureDiscardsPrelude) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->addPreludeGlobalRegister("__LLVM_G_SP", 254,
                                     UINT64_C(0x2000000004000000));
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitValue(MCConstantExpr::create(1, Context), 3);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "MMIXAL data value width must be 1, 2, 4, or 8 bytes");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, EmitsFixedBareMetalGlobalRegisterPrelude) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *ReservedName = Context.getOrCreateSymbol("canonical_reserved");
  expectSuccess(Streamer->registerUserSymbol(*ReservedName, "__LLVM_G_SP"));
  addMMIXALBareMetalGlobalRegisterPrelude(*Streamer);
  EXPECT_EQ(Streamer->getNumPreludeGlobalRegisters(), 24u);

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(ReservedName);
  MCInst Add;
  Add.setOpcode(MMIX::ADD);
  Add.addOperand(MCOperand::createReg(MMIX::R231));
  Add.addOperand(MCOperand::createReg(MMIX::R254));
  Add.addOperand(MCOperand::createReg(MMIX::R255));
  Streamer->emitInstruction(Add, *STI);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(lookup(*Streamer, *ReservedName),
            "__LLVM_U_11_5F5F4C4C564D5F475F5350");
  EXPECT_EQ(Output, "__LLVM_G_SP\tGREG #2000000004000000\n"
                    "__LLVM_G_FP\tGREG 0\n"
                    "__LLVM_G_R252\tGREG 0\n"
                    "__LLVM_G_R251\tGREG 0\n"
                    "__LLVM_G_R250\tGREG 0\n"
                    "__LLVM_G_R249\tGREG 0\n"
                    "__LLVM_G_R248\tGREG 0\n"
                    "__LLVM_G_R247\tGREG 0\n"
                    "__LLVM_G_R246\tGREG 0\n"
                    "__LLVM_G_R245\tGREG 0\n"
                    "__LLVM_G_R244\tGREG 0\n"
                    "__LLVM_G_R243\tGREG 0\n"
                    "__LLVM_G_R242\tGREG 0\n"
                    "__LLVM_G_R241\tGREG 0\n"
                    "__LLVM_G_R240\tGREG 0\n"
                    "__LLVM_G_R239\tGREG 0\n"
                    "__LLVM_G_R238\tGREG 0\n"
                    "__LLVM_G_R237\tGREG 0\n"
                    "__LLVM_G_R236\tGREG 0\n"
                    "__LLVM_G_R235\tGREG 0\n"
                    "__LLVM_G_R234\tGREG 0\n"
                    "__LLVM_G_R233\tGREG 0\n"
                    "__LLVM_G_R232\tGREG 0\n"
                    "__LLVM_G_R231\tGREG 0\n"
                    "\tLOC #0000000000000100\n"
                    "__LLVM_U_11_5F5F4C4C564D5F475F5350\tIS @\n"
                    "\tADD $231, $254, $255\n");
}

TEST_F(MMIXALAsmStreamerTest, RejectsFixedPreludeCollisionAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->addPreludeGlobalRegister("__LLVM_G_SP", 254, 0);
  addMMIXALBareMetalGlobalRegisterPrelude(*Streamer);
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitIntValue(1, 1);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "duplicate target-owned MMIXAL prelude name: __LLVM_G_SP");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, BuildsAndPlacesRawEntryPrefix) {
  const std::array<MCInst, 2> Prefix = createMMIXALRawEntryPrefix();
  ASSERT_EQ(Prefix[0].getOpcode(), MMIX::PUTI);
  ASSERT_EQ(Prefix[0].getNumOperands(), 2u);
  EXPECT_TRUE(Prefix[0].getOperand(0).isReg());
  EXPECT_EQ(Prefix[0].getOperand(0).getReg(), MMIX::RA);
  EXPECT_TRUE(Prefix[0].getOperand(1).isImm());
  EXPECT_EQ(Prefix[0].getOperand(1).getImm(), 0);
  ASSERT_EQ(Prefix[1].getOpcode(), MMIX::PUTI);
  ASSERT_EQ(Prefix[1].getNumOperands(), 2u);
  EXPECT_TRUE(Prefix[1].getOperand(0).isReg());
  EXPECT_EQ(Prefix[1].getOperand(0).getReg(), MMIX::RL);
  EXPECT_TRUE(Prefix[1].getOperand(1).isImm());
  EXPECT_EQ(Prefix[1].getOperand(1).getImm(), 0);

  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);
  MCSymbol *Entry = Context.getOrCreateSymbol("canonical_main");
  MCSymbol *Body = Context.getOrCreateSymbol("canonical_entry_body");
  expectSuccess(Streamer->registerEntrySymbol(*Entry, "Main"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Body, "Main",
                                                        Kind::BasicBlock, 0));

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Entry);
  for (const MCInst &Inst : Prefix)
    Streamer->emitInstruction(Inst, *STI);
  Streamer->emitLabel(Body);
  MCInst Jump;
  Jump.setOpcode(MMIX::JMPB);
  Jump.addOperand(
      MCOperand::createExpr(MCSymbolRefExpr::create(Entry, Context)));
  Streamer->emitInstruction(Jump, *STI);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #0000000000000100\n"
                    "Main\tIS @\n"
                    "\tPUT rA, 0\n"
                    "\tPUT rL, 0\n"
                    "\tLOC #0000000000000108\n"
                    "__LLVM_L_F_4D61696E_BB_0\tIS @\n"
                    "\tJMP Main\n");
}

TEST_F(MMIXALAsmStreamerTest,
       SuppressesAddressNeutralMetadataAndEmptyNoteSections) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->emitFileDirective("input.s");
  Streamer->emitFileDirective("input.s", "compiler", "timestamp",
                              "description");
  Expected<unsigned> File = Streamer->tryEmitDwarfFileDirective(
      1, "source", "input.s", std::nullopt, std::nullopt, 0);
  ASSERT_TRUE(static_cast<bool>(File));
  EXPECT_EQ(*File, 1u);
  Streamer->emitDwarfFile0Directive("source", "input.s", std::nullopt,
                                    std::nullopt, 0);
  Streamer->emitDwarfLocDirective(1, 7, 3, 0, 0, 0, "input.s");
  Streamer->emitIdent("compiler identification");

  Streamer->switchSection(getSection(Context, ".note.test", ELF::SHT_NOTE, 0));
  Streamer->switchSection(
      getSection(Context, ".note.GNU-stack", ELF::SHT_PROGBITS, 0));
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  MCSymbol *Data = Context.getOrCreateSymbol("canonical_data");
  expectSuccess(Streamer->registerUserSymbol(*Data, "data"));
  EXPECT_TRUE(Streamer->emitSymbolAttribute(Data, MCSA_ELF_TypeObject));
  Streamer->emitLabel(Data);
  Streamer->emitValue(MCConstantExpr::create(42, Context), 8);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "data\tIS @\n"
                    "\tOCTA #000000000000002A\n");
}

TEST_F(MMIXALAsmStreamerTest, RejectsContentInNonallocatingNoteSection) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->switchSection(getSection(Context, ".note.test", ELF::SHT_NOTE, 0));
  Streamer->emitBytes("note payload");
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL cannot allocate section '.note.test': section is not "
            "allocated");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsEmptyIncompatibleCustomSection) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->switchSection(
      getSection(Context, ".custom.metadata", ELF::SHT_PROGBITS, 0));
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL cannot allocate section '.custom.metadata': section is "
            "not allocated");
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
  First->switchSection(getSection(FirstContext, ".data", ELF::SHT_PROGBITS,
                                  ELF::SHF_ALLOC | ELF::SHF_WRITE));

  First->emitBytes("first-only");
  ASSERT_EQ(First->getNumBufferedEvents(), 2u);
  ASSERT_EQ(Second->getNumBufferedEvents(), 0u);

  Second->finish();
  First->finish();

  EXPECT_FALSE(SecondContext.hadError());
  EXPECT_TRUE(SecondDiagnostic.empty());
  EXPECT_TRUE(SecondOutput.empty());
  EXPECT_FALSE(FirstContext.hadError());
  EXPECT_TRUE(FirstDiagnostic.empty());
  EXPECT_EQ(FirstOutput, "\tLOC #2000000000000000\n"
                         "\tBYTE #66, #69, #72, #73, #74, #2D, #6F, #6E, "
                         "#6C, #79\n");
}

TEST_F(MMIXALAsmStreamerTest, RegistersSemanticAndPrivateSymbolIdentities) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));

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
  EXPECT_EQ(Streamer->getNumBufferedEvents(), 4u);
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

TEST_F(MMIXALAsmStreamerTest, PreservesDistinguishedEntryAndMapsItsReferences) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Entry = Context.getOrCreateSymbol("canonical_main");
  MCSymbol *Alias = Context.getOrCreateSymbol("canonical_entry_alias");
  expectSuccess(Streamer->registerEntrySymbol(*Entry, "Main"));
  expectSuccess(Streamer->registerUserSymbol(*Alias, "entry_alias"));
  expectSuccess(Streamer->registerSourceBlock(*Entry, "Main", "Main"));

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Entry);
  Streamer->emitAssignment(Alias, MCSymbolRefExpr::create(Entry, Context));
  MCInst Jump;
  Jump.setOpcode(MMIX::JMP);
  Jump.addOperand(
      MCOperand::createExpr(MCSymbolRefExpr::create(Entry, Context)));
  Streamer->emitInstruction(Jump, *STI);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(lookup(*Streamer, *Entry), "Main");
  EXPECT_EQ(lookup(*Streamer, *Alias), "entry_alias");
  EXPECT_EQ(Output, "\tLOC #0000000000000100\n"
                    "Main\tIS @\n"
                    "__LLVM_B_F_4D61696E_B_4D61696E\tIS @\n"
                    "entry_alias\tIS @\n"
                    "\tJMP Main\n");
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

TEST_F(MMIXALAsmStreamerTest, ClassifiesAllocatedItemsIntoLogicalGroups) {
  using Group = MMIXALAsmStreamer::LogicalGroup;
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSectionELF *Text = getSection(Context, ".custom.text", ELF::SHT_PROGBITS,
                                  ELF::SHF_ALLOC | ELF::SHF_EXECINSTR);
  MCSectionELF *ReadOnly =
      getSection(Context, ".custom.rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  MCSectionELF *Writable =
      getSection(Context, ".custom.data", ELF::SHT_PROGBITS,
                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  MCSectionELF *Zero = getSection(Context, ".custom.bss", ELF::SHT_NOBITS,
                                  ELF::SHF_ALLOC | ELF::SHF_WRITE);

  MCSymbol *TextOwner = Context.getOrCreateSymbol("canonical_text");
  MCSymbol *ReadOnlyOwner = Context.getOrCreateSymbol("canonical_read_only");
  MCSymbol *ConstantPool = Context.getOrCreateSymbol("canonical_cpi");
  MCSymbol *JumpTable = Context.getOrCreateSymbol("canonical_jti");
  MCSymbol *BlockAddressTable =
      Context.getOrCreateSymbol("canonical_block_address_table");
  MCSymbol *WritableOwner = Context.getOrCreateSymbol("canonical_data");
  MCSymbol *ZeroOwner = Context.getOrCreateSymbol("canonical_bss");
  MCSymbol *SecondReadOnlyOwner =
      Context.getOrCreateSymbol("canonical_second_read_only");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(
      *ConstantPool, "function", Kind::ConstantPool, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*JumpTable, "function",
                                                        Kind::JumpTable, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(
      *BlockAddressTable, "function", Kind::BlockAddress, 0));

  Streamer->switchSection(Text);
  Streamer->emitCodeAlignment(Align(16), *STI, 12);
  Streamer->emitLabel(TextOwner);
  MCInst Inst;
  Inst.setOpcode(MMIX::ADD);
  Streamer->emitInstruction(Inst, *STI);

  Streamer->switchSection(ReadOnly);
  Streamer->emitLabel(ReadOnlyOwner);
  Streamer->emitValue(MCConstantExpr::create(1, Context), 8);
  Streamer->emitBytes("ro");
  Streamer->switchSection(ReadOnly);
  Streamer->emitBytes("x");

  Streamer->emitValueToAlignment(Align(8), 0, 1, 7);
  Streamer->emitLabel(ConstantPool);
  Streamer->emitValue(MCConstantExpr::create(2, Context), 8);

  Streamer->emitLabel(JumpTable);
  Streamer->emitValue(MCSymbolRefExpr::create(TextOwner, Context), 8);

  Streamer->emitLabel(BlockAddressTable);
  Streamer->emitValue(MCSymbolRefExpr::create(TextOwner, Context), 8);

  Streamer->switchSection(Writable);
  Streamer->emitLabel(WritableOwner);
  Streamer->emitFill(*MCConstantExpr::create(6, Context), 0);

  Streamer->switchSection(Zero);
  Streamer->emitLabel(ZeroOwner);
  Streamer->emitFill(*MCConstantExpr::create(16, Context), 0);

  Streamer->switchSection(ReadOnly);
  Streamer->emitLabel(SecondReadOnlyOwner);
  Streamer->emitBytes("z");

  ASSERT_EQ(Streamer->getNumBufferedItems(), 8u);
  ASSERT_EQ(Streamer->getBufferedItems(Group::Text).size(), 1u);
  ASSERT_EQ(Streamer->getBufferedItems(Group::ReadOnly).size(), 2u);
  ASSERT_EQ(Streamer->getBufferedItems(Group::ConstantPool).size(), 1u);
  ASSERT_EQ(Streamer->getBufferedItems(Group::JumpTable).size(), 2u);
  ASSERT_EQ(Streamer->getBufferedItems(Group::WritableData).size(), 1u);
  ASSERT_EQ(Streamer->getBufferedItems(Group::ZeroStorage).size(), 1u);

  const auto &TextItem = Streamer->getBufferedItems(Group::Text).front();
  EXPECT_EQ(TextItem.SourceOrder, 0u);
  EXPECT_EQ(TextItem.OwningSymbols, ArrayRef<const MCSymbol *>({TextOwner}));
  ASSERT_EQ(TextItem.Alignments.size(), 1u);
  EXPECT_EQ(TextItem.Alignments.front().Alignment, 16u);
  EXPECT_EQ(TextItem.Alignments.front().MaxBytesToEmit, 12u);
  EXPECT_TRUE(TextItem.Alignments.front().IsCodeAlignment);
  EXPECT_EQ(TextItem.KnownSize, 4u);
  EXPECT_TRUE(TextItem.SizeIsKnown);

  const auto &FirstReadOnly =
      Streamer->getBufferedItems(Group::ReadOnly).front();
  EXPECT_EQ(FirstReadOnly.SourceOrder, 1u);
  EXPECT_EQ(FirstReadOnly.KnownSize, 11u);
  EXPECT_EQ(FirstReadOnly.OwningSymbols,
            ArrayRef<const MCSymbol *>({ReadOnlyOwner}));

  const auto &ConstantPoolItem =
      Streamer->getBufferedItems(Group::ConstantPool).front();
  EXPECT_EQ(ConstantPoolItem.SourceOrder, 2u);
  EXPECT_EQ(ConstantPoolItem.KnownSize, 8u);
  ASSERT_EQ(ConstantPoolItem.Alignments.size(), 1u);
  EXPECT_EQ(ConstantPoolItem.Alignments.front().Alignment, 8u);
  EXPECT_EQ(ConstantPoolItem.Alignments.front().MaxBytesToEmit, 7u);

  const auto &JumpTableItem =
      Streamer->getBufferedItems(Group::JumpTable).front();
  EXPECT_EQ(JumpTableItem.SourceOrder, 3u);
  EXPECT_EQ(JumpTableItem.Dependencies,
            ArrayRef<const MCSymbol *>({TextOwner}));
  EXPECT_EQ(JumpTableItem.KnownSize, 8u);

  const auto &BlockAddressItem =
      Streamer->getBufferedItems(Group::JumpTable).back();
  EXPECT_EQ(BlockAddressItem.SourceOrder, 4u);
  EXPECT_EQ(BlockAddressItem.Dependencies,
            ArrayRef<const MCSymbol *>({TextOwner}));

  EXPECT_EQ(Streamer->getBufferedItems(Group::WritableData).front().SourceOrder,
            5u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::WritableData).front().KnownSize,
            6u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::ZeroStorage).front().SourceOrder,
            6u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::ZeroStorage).front().KnownSize,
            16u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::ReadOnly).back().SourceOrder, 7u);
  EXPECT_FALSE(Streamer->hasClassificationError());
  EXPECT_TRUE(Output.empty());

  Streamer->reset();
  Streamer->finish();
}

TEST_F(MMIXALAsmStreamerTest, KeepsBlockAddressCodeLabelsInText) {
  using Group = MMIXALAsmStreamer::LogicalGroup;
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *BlockAddress = Context.getOrCreateSymbol("canonical_block_address");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(
      *BlockAddress, "function", Kind::BlockAddress, 0));

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(BlockAddress);
  MCInst Inst;
  Inst.setOpcode(MMIX::ADD);
  Streamer->emitInstruction(Inst, *STI);

  ASSERT_EQ(Streamer->getBufferedItems(Group::Text).size(), 1u);
  EXPECT_TRUE(Streamer->getBufferedItems(Group::JumpTable).empty());
  EXPECT_EQ(Streamer->getBufferedItems(Group::Text).front().OwningSymbols,
            ArrayRef<const MCSymbol *>({BlockAddress}));
  EXPECT_FALSE(Streamer->hasClassificationError());
  EXPECT_TRUE(Output.empty());

  Streamer->reset();
  Streamer->finish();
}

TEST_F(MMIXALAsmStreamerTest, RejectsUnsupportedAllocatedSections) {
  struct SectionCase {
    const char *Name;
    unsigned Type;
    unsigned Flags;
    unsigned EntrySize;
    uint32_t Subsection;
    const char *Diagnostic;
  };
  const SectionCase Cases[] = {
      {".merge", ELF::SHT_PROGBITS, ELF::SHF_ALLOC | ELF::SHF_MERGE, 1, 0,
       "merge, TLS, group, ordering, or target-specific ELF flags"},
      {".tls", ELF::SHT_PROGBITS,
       ELF::SHF_ALLOC | ELF::SHF_WRITE | ELF::SHF_TLS, 0, 0,
       "merge, TLS, group, ordering, or target-specific ELF flags"},
      {".eh_frame", ELF::SHT_PROGBITS, ELF::SHF_ALLOC, 0, 0,
       "special-purpose section semantics"},
      {".init_array", ELF::SHT_INIT_ARRAY, ELF::SHF_ALLOC | ELF::SHF_WRITE, 0,
       0, "special-purpose section semantics"},
      {".debug_info", ELF::SHT_PROGBITS, 0, 0, 0, "section is not allocated"},
      {".ordered", ELF::SHT_PROGBITS, ELF::SHF_ALLOC, 0, 1,
       "nonzero ELF subsections"},
  };

  for (const SectionCase &Case : Cases) {
    SCOPED_TRACE(Case.Name);
    MCContext Context(TT, MAI, *MRI, *STI);
    std::string Diagnostic;
    captureDiagnostic(Context, Diagnostic);
    std::string Output;
    raw_string_ostream OutputOS(Output);
    auto Streamer = createStreamer(Context, OutputOS);
    Streamer->switchSection(
        getSection(Context, Case.Name, Case.Type, Case.Flags, Case.EntrySize),
        Case.Subsection);
    Streamer->emitBytes("allocated");

    Streamer->finish();

    EXPECT_TRUE(Context.hadError());
    EXPECT_NE(Diagnostic.find(Case.Diagnostic), std::string::npos)
        << Diagnostic;
    EXPECT_TRUE(Output.empty());
  }
}

TEST_F(MMIXALAsmStreamerTest, RejectsCommonSymbolAllocationAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Common = Context.getOrCreateSymbol("canonical_common");
  Streamer->emitCommonSymbol(Common, 8, Align(8));
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL does not support common-symbol allocation events");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsCFIUnwindEventsAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitCFIStartProc(false);
  Streamer->emitCFIDefCfa(MMIX::R254, 0);
  Streamer->emitCFIEndProc();
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "MMIXAL does not support CFI/unwind events");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsUnsupportedScalarWidthAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitValue(MCConstantExpr::create(1, Context), 3);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "MMIXAL data value width must be 1, 2, 4, or 8 bytes");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsDuplicateDefinitionAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Duplicate = Context.getOrCreateSymbol("canonical_duplicate");
  expectSuccess(Streamer->registerUserSymbol(*Duplicate, "duplicate"));
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitLabel(Duplicate);
  Streamer->emitLabel(Duplicate);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL layout item 'canonical_duplicate': symbol "
            "'canonical_duplicate' is assigned by more than one item");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsNonzeroDataAlignmentFillAtomically) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitIntValue(1, 1);
  Streamer->emitValueToAlignment(Align(8), 0xff);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL data alignment with nonzero fill is unsupported");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsInitializedContentInZeroStorage) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);
  Streamer->switchSection(getSection(Context, ".bss", ELF::SHT_NOBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitFill(*MCConstantExpr::create(4, Context), 1);

  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "MMIXAL zero-storage section contains a nonzero fill");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest,
       EmitsPlannedLocationsAliasesInstructionsAndExecutablePadding) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Function = Context.getOrCreateSymbol("canonical_function");
  MCSymbol *EntryAlias = Context.getOrCreateSymbol("canonical_entry_alias");
  MCSymbol *FunctionAlias =
      Context.getOrCreateSymbol("canonical_function_alias");
  MCSymbol *Block = Context.getOrCreateSymbol(".Lcanonical_block");
  MCSymbol *Target = Context.getOrCreateSymbol(".Lcanonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Function, "entry_function"));
  expectSuccess(Streamer->registerUserSymbol(*EntryAlias, "entry_alias"));
  expectSuccess(Streamer->registerUserSymbol(*FunctionAlias, "function_alias"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Block, "function",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerSourceBlock(*Block, "function", "success"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Target, "function",
                                                        Kind::BasicBlock, 1));

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Function);
  Streamer->emitLabel(EntryAlias);
  MCInst Add;
  Add.setOpcode(MMIX::ADD);
  Add.addOperand(MCOperand::createReg(MMIX::R1));
  Add.addOperand(MCOperand::createReg(MMIX::R2));
  Add.addOperand(MCOperand::createReg(MMIX::R3));
  Streamer->emitInstruction(Add, *STI);
  Streamer->emitAssignment(FunctionAlias,
                           MCSymbolRefExpr::create(Function, Context));

  Streamer->emitCodeAlignment(Align(16), *STI, 12);
  Streamer->emitLabel(Block);
  MCInst Jump;
  Jump.setOpcode(MMIX::JMP);
  Jump.addOperand(
      MCOperand::createExpr(MCSymbolRefExpr::create(Target, Context)));
  Streamer->emitInstruction(Jump, *STI);

  Streamer->emitLabel(Target);
  Streamer->emitInstruction(Add, *STI);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_TRUE(Diagnostic.empty());
  EXPECT_EQ(Output, "\tLOC #0000000000000100\n"
                    "entry_function\tIS @\n"
                    "entry_alias\tIS @\n"
                    "\tADD $1, $2, $3\n"
                    "function_alias\tIS entry_function\n"
                    "\tLOC #0000000000000104\n"
                    "\tSWYM 0, 0, 0\n"
                    "\tSWYM 0, 0, 0\n"
                    "\tSWYM 0, 0, 0\n"
                    "__LLVM_L_F_66756E6374696F6E_BB_0\tIS @\n"
                    "success\tIS @\n"
                    "\tJMP __LLVM_L_F_66756E6374696F6E_BB_1\n"
                    "\tLOC #0000000000000114\n"
                    "__LLVM_L_F_66756E6374696F6E_BB_1\tIS @\n"
                    "\tADD $1, $2, $3\n");
}

TEST_F(MMIXALAsmStreamerTest, EmitsInitializedScalarsAndByteArrays) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Data = Context.getOrCreateSymbol("canonical_data");
  expectSuccess(Streamer->registerUserSymbol(*Data, "data"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(Data);
  Streamer->emitValue(MCConstantExpr::create(0x1122334455667788, Context), 8);
  Streamer->emitValue(MCConstantExpr::create(0x99aabbcc, Context), 4);
  Streamer->emitValue(MCConstantExpr::create(0xddee, Context), 2);
  Streamer->emitValue(MCConstantExpr::create(0xff, Context), 1);
  const char Bytes[] = {0, 1, 0x7f, static_cast<char>(0x80),
                        static_cast<char>(0xff)};
  Streamer->emitBytes(StringRef(Bytes, sizeof(Bytes)));
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "data\tIS @\n"
                    "\tOCTA #1122334455667788\n"
                    "\tTETRA #99AABBCC\n"
                    "\tWYDE #DDEE\n"
                    "\tBYTE #FF\n"
                    "\tBYTE 0, #01, #7F, #80, #FF\n");
}

TEST_F(MMIXALAsmStreamerTest,
       PreservesUnalignedAndRepeatedValuesAsBigEndianBytes) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Patterns = Context.getOrCreateSymbol("canonical_patterns");
  expectSuccess(Streamer->registerUserSymbol(*Patterns, "patterns"));
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitLabel(Patterns);
  Streamer->emitValue(MCConstantExpr::create(0xa1, Context), 1);
  Streamer->emitValue(MCConstantExpr::create(0xb2c3, Context), 2);
  Streamer->emitFill(*MCConstantExpr::create(3, Context), 0x1ab);
  Streamer->emitFill(*MCConstantExpr::create(2, Context), 3, 0xd4e5f6);
  Streamer->emitValue(MCConstantExpr::create(-1, Context), 2);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "patterns\tIS @\n"
                    "\tBYTE #A1\n"
                    "\tBYTE #B2, #C3\n"
                    "\tBYTE #AB, #AB, #AB\n"
                    "\tBYTE #D4, #E5, #F6, #D4, #E5, #F6\n"
                    "\tWYDE #FFFF\n");
}

TEST_F(MMIXALAsmStreamerTest, BoundsNumericByteArraySourceLines) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Bytes = Context.getOrCreateSymbol("canonical_bytes");
  expectSuccess(Streamer->registerUserSymbol(*Bytes, "bytes"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(Bytes);
  Streamer->emitBytes(std::string(20, static_cast<char>(0xff)));
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "bytes\tIS @\n"
                    "\tBYTE #FF, #FF, #FF, #FF, #FF, #FF, #FF, #FF, #FF, "
                    "#FF, #FF, #FF, #FF\n"
                    "\tBYTE #FF, #FF, #FF, #FF, #FF, #FF, #FF\n");
  SmallVector<StringRef, 8> Lines;
  StringRef(Output).split(Lines, '\n');
  for (StringRef Line : Lines)
    EXPECT_LE(Line.size(), 72u);
  EXPECT_EQ(Output.find('"'), std::string::npos);
}

TEST_F(MMIXALAsmStreamerTest, EmitsInitializedAndZeroStorageZerosExplicitly) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Initialized = Context.getOrCreateSymbol("canonical_initialized");
  MCSymbol *ZeroStorage = Context.getOrCreateSymbol("canonical_zero_storage");
  expectSuccess(Streamer->registerUserSymbol(*Initialized, "initialized"));
  expectSuccess(Streamer->registerUserSymbol(*ZeroStorage, "zero_storage"));
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitLabel(Initialized);
  Streamer->emitFill(*MCConstantExpr::create(19, Context), 0);
  MCSectionELF *BSS = getSection(Context, ".bss", ELF::SHT_NOBITS,
                                 ELF::SHF_ALLOC | ELF::SHF_WRITE);
  Streamer->emitZerofill(BSS, ZeroStorage, 24, Align(8));
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "initialized\tIS @\n"
                    "\tOCTA 0, 0\n"
                    "\tWYDE 0\n"
                    "\tBYTE 0\n"
                    "\tLOC #2000000000000018\n"
                    "zero_storage\tIS @\n"
                    "\tOCTA 0, 0, 0\n");
}

TEST_F(MMIXALAsmStreamerTest,
       EmitsConstantPoolsJumpTablesAndBlockAddressObjects) {
  using Group = MMIXALAsmStreamer::LogicalGroup;
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *BlockAddress = Context.getOrCreateSymbol("canonical_block_address");
  MCSymbol *BasicBlock = Context.getOrCreateSymbol("canonical_basic_block");
  MCSymbol *ConstantPool = Context.getOrCreateSymbol("canonical_cpi");
  MCSymbol *ConstantAlias = Context.getOrCreateSymbol("canonical_cpi_alias");
  MCSymbol *JumpTable = Context.getOrCreateSymbol("canonical_jti");
  MCSymbol *BlockAddressObject =
      Context.getOrCreateSymbol("canonical_block_address_object");
  MCSymbol *Ordinary = Context.getOrCreateSymbol("canonical_ordinary");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*BlockAddress, "f",
                                                        Kind::BlockAddress, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*BasicBlock, "f",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*ConstantPool, "f",
                                                        Kind::ConstantPool, 0));
  expectSuccess(
      Streamer->registerUserSymbol(*ConstantAlias, "shared_constant"));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*JumpTable, "f",
                                                        Kind::JumpTable, 0));
  expectSuccess(Streamer->registerUserSymbol(*BlockAddressObject,
                                             "block_address_object"));
  expectSuccess(Streamer->registerUserSymbol(*Ordinary, "ordinary"));

  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  MCInst Add;
  Add.setOpcode(MMIX::ADD);
  Add.addOperand(MCOperand::createReg(MMIX::R1));
  Add.addOperand(MCOperand::createReg(MMIX::R2));
  Add.addOperand(MCOperand::createReg(MMIX::R3));
  Streamer->emitLabel(BlockAddress);
  Streamer->emitInstruction(Add, *STI);
  Streamer->emitLabel(BasicBlock);
  Streamer->emitInstruction(Add, *STI);

  MCSectionELF *ReadOnly =
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  Streamer->switchSection(ReadOnly);
  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Ordinary);
  Streamer->emitValue(MCConstantExpr::create(0x0102030405060708, Context), 8);

  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(ConstantAlias);
  Streamer->emitLabel(ConstantPool);
  Streamer->emitValue(MCConstantExpr::create(0x400921fb54442d18, Context), 8);

  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(JumpTable);
  Streamer->emitValue(MCSymbolRefExpr::create(BlockAddress, Context), 8);
  Streamer->emitValue(MCSymbolRefExpr::create(BasicBlock, Context), 8);

  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(BlockAddressObject);
  Streamer->emitValue(MCSymbolRefExpr::create(BlockAddress, Context), 8);

  EXPECT_EQ(Streamer->getBufferedItems(Group::Text).size(), 2u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::ReadOnly).size(), 1u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::ConstantPool).size(), 1u);
  EXPECT_EQ(Streamer->getBufferedItems(Group::JumpTable).size(), 2u);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #0000000000000100\n"
                    "__LLVM_L_F_66_BA_0\tIS @\n"
                    "\tADD $1, $2, $3\n"
                    "\tLOC #0000000000000104\n"
                    "__LLVM_L_F_66_BB_0\tIS @\n"
                    "\tADD $1, $2, $3\n"
                    "\tLOC #2000000000000000\n"
                    "ordinary\tIS @\n"
                    "\tOCTA #0102030405060708\n"
                    "\tLOC #2000000000000008\n"
                    "shared_constant\tIS @\n"
                    "__LLVM_L_F_66_CP_0\tIS @\n"
                    "\tOCTA #400921FB54442D18\n"
                    "\tLOC #2000000000000010\n"
                    "__LLVM_L_F_66_JT_0\tIS @\n"
                    "\tOCTA __LLVM_L_F_66_BA_0\n"
                    "\tOCTA __LLVM_L_F_66_BB_0\n"
                    "\tLOC #2000000000000020\n"
                    "block_address_object\tIS @\n"
                    "\tOCTA __LLVM_L_F_66_BA_0\n");
}

TEST_F(MMIXALAsmStreamerTest, RejectsUndefinedOCTATargetWithoutPartialOutput) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Data = Context.getOrCreateSymbol("canonical_data");
  MCSymbol *Target = Context.getOrCreateSymbol("canonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Data, "data"));
  expectSuccess(Streamer->registerUserSymbol(*Target, "target"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(Data);
  Streamer->emitValue(MCSymbolRefExpr::create(Target, Context), 8);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL OCTA target 'canonical_target' has no allocated "
            "definition");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest,
       RejectsRelativeDirectionMismatchWithoutPartialOutput) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Source = Context.getOrCreateSymbol(".Lsource");
  MCSymbol *Target = Context.getOrCreateSymbol(".Ltarget");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Source, "function",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Target, "function",
                                                        Kind::BasicBlock, 1));
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Source);
  MCInst Jump;
  Jump.setOpcode(MMIX::JMPB);
  Jump.addOperand(
      MCOperand::createExpr(MCSymbolRefExpr::create(Target, Context)));
  Streamer->emitInstruction(Jump, *STI);
  Streamer->emitLabel(Target);

  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL relative target direction does not match instruction");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, RejectsMisalignedAndOutOfRangeRelativeTargets) {
  struct RelativeCase {
    unsigned Opcode;
    uint64_t Target;
    const char *Diagnostic;
  };
  const RelativeCase Cases[] = {
      {MMIX::JMP, 0x102, "MMIXAL relative target is not four-byte aligned"},
      {MMIX::BN, 0x40100, "MMIXAL relative target is out of range"},
  };

  for (const RelativeCase &Case : Cases) {
    SCOPED_TRACE(Case.Opcode);
    MCContext Context(TT, MAI, *MRI, *STI);
    std::string Diagnostic;
    captureDiagnostic(Context, Diagnostic);
    std::string Output;
    raw_string_ostream OutputOS(Output);
    auto Streamer = createStreamer(Context, OutputOS);
    Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                       ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));

    MCInst Inst;
    Inst.setOpcode(Case.Opcode);
    if (Case.Opcode == MMIX::BN)
      Inst.addOperand(MCOperand::createReg(MMIX::R1));
    Inst.addOperand(MCOperand::createExpr(
        MCConstantExpr::create(static_cast<int64_t>(Case.Target), Context)));
    Streamer->emitInstruction(Inst, *STI);
    Streamer->finish();

    EXPECT_TRUE(Context.hadError());
    EXPECT_EQ(Diagnostic, Case.Diagnostic);
    EXPECT_TRUE(Output.empty());
  }
}

TEST_F(MMIXALAsmStreamerTest, UsesLocationForNonFallthroughTextAlignment) {
  using Kind = MMIXALSymbolTable::PrivateSymbolKind;
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Source = Context.getOrCreateSymbol(".Lsource");
  MCSymbol *Target = Context.getOrCreateSymbol(".Ltarget");
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Source, "function",
                                                        Kind::BasicBlock, 0));
  expectSuccess(Streamer->registerFunctionPrivateSymbol(*Target, "function",
                                                        Kind::BasicBlock, 1));
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Source);
  MCInst Jump;
  Jump.setOpcode(MMIX::JMP);
  Jump.addOperand(
      MCOperand::createExpr(MCSymbolRefExpr::create(Target, Context)));
  Streamer->emitInstruction(Jump, *STI);

  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Target);
  MCInst Add;
  Add.setOpcode(MMIX::ADD);
  Add.addOperand(MCOperand::createReg(MMIX::R1));
  Add.addOperand(MCOperand::createReg(MMIX::R2));
  Add.addOperand(MCOperand::createReg(MMIX::R3));
  Streamer->emitInstruction(Add, *STI);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #0000000000000100\n"
                    "__LLVM_L_F_66756E6374696F6E_BB_0\tIS @\n"
                    "\tJMP __LLVM_L_F_66756E6374696F6E_BB_1\n"
                    "\tLOC #0000000000000108\n"
                    "__LLVM_L_F_66756E6374696F6E_BB_1\tIS @\n"
                    "\tADD $1, $2, $3\n");
}

TEST_F(MMIXALAsmStreamerTest, SchedulesAliasAfterLaterTargetDefinition) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Anchor = Context.getOrCreateSymbol("canonical_anchor");
  MCSymbol *Alias = Context.getOrCreateSymbol("canonical_alias");
  MCSymbol *Target = Context.getOrCreateSymbol("canonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Anchor, "anchor"));
  expectSuccess(Streamer->registerUserSymbol(*Alias, "alias"));
  expectSuccess(Streamer->registerUserSymbol(*Target, "target"));
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Anchor);
  Streamer->emitAssignment(Alias, MCSymbolRefExpr::create(Target, Context));
  MCInst Jump;
  Jump.setOpcode(MMIX::JMP);
  Jump.addOperand(
      MCOperand::createExpr(MCSymbolRefExpr::create(Alias, Context)));
  Streamer->emitInstruction(Jump, *STI);
  Streamer->emitLabel(Target);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #0000000000000104\n"
                    "target\tIS @\n"
                    "\tLOC #0000000000000100\n"
                    "anchor\tIS @\n"
                    "alias\tIS target\n"
                    "\tJMP alias\n");
}

TEST_F(MMIXALAsmStreamerTest, SchedulesCompoundDataExpressionAfterDefinition) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Use = Context.getOrCreateSymbol("canonical_use");
  MCSymbol *Target = Context.getOrCreateSymbol("canonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Use, "use"));
  expectSuccess(Streamer->registerUserSymbol(*Target, "target"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Use);
  Streamer->emitValue(
      MCBinaryExpr::createAdd(MCSymbolRefExpr::create(Target, Context),
                              MCConstantExpr::create(8, Context), Context),
      8);
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Target);
  Streamer->emitValue(MCConstantExpr::create(42, Context), 8);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000008\n"
                    "target\tIS @\n"
                    "\tOCTA #000000000000002A\n"
                    "\tLOC #2000000000000000\n"
                    "use\tIS @\n"
                    "\tOCTA target+8\n");
}

TEST_F(MMIXALAsmStreamerTest,
       SchedulesCompoundInstructionExpressionAfterDefinition) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Function = Context.getOrCreateSymbol("canonical_function");
  MCSymbol *Target = Context.getOrCreateSymbol("canonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Function, "function"));
  expectSuccess(Streamer->registerUserSymbol(*Target, "target"));
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));
  Streamer->emitLabel(Function);
  const MCExpr *HighWyde = MCBinaryExpr::createAnd(
      MCBinaryExpr::createLShr(MCSymbolRefExpr::create(Target, Context),
                               MCConstantExpr::create(48, Context), Context),
      MCConstantExpr::create(0xffff, Context), Context);
  MCInst SetHigh;
  SetHigh.setOpcode(MMIX::SETH);
  SetHigh.addOperand(MCOperand::createReg(MMIX::R1));
  SetHigh.addOperand(MCOperand::createExpr(HighWyde));
  Streamer->emitInstruction(SetHigh, *STI);
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Target);
  Streamer->emitValue(MCConstantExpr::create(42, Context), 8);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "target\tIS @\n"
                    "\tOCTA #000000000000002A\n"
                    "\tLOC #0000000000000100\n"
                    "function\tIS @\n"
                    "\tSETH $1, target>>48&65535\n");
}

TEST_F(MMIXALAsmStreamerTest, KeepsBareFutureOCTAInSourceOrder) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Use = Context.getOrCreateSymbol("canonical_use");
  MCSymbol *Target = Context.getOrCreateSymbol("canonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Use, "use"));
  expectSuccess(Streamer->registerUserSymbol(*Target, "target"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Use);
  Streamer->emitValue(MCSymbolRefExpr::create(Target, Context), 8);
  Streamer->switchSection(getSection(Context, ".data", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_WRITE));
  Streamer->emitValueToAlignment(Align(8));
  Streamer->emitLabel(Target);
  Streamer->emitValue(MCConstantExpr::create(42, Context), 8);
  Streamer->finish();

  EXPECT_FALSE(Context.hadError());
  EXPECT_EQ(Output, "\tLOC #2000000000000000\n"
                    "use\tIS @\n"
                    "\tOCTA target\n"
                    "\tLOC #2000000000000008\n"
                    "target\tIS @\n"
                    "\tOCTA #000000000000002A\n");
}

TEST_F(MMIXALAsmStreamerTest,
       RejectsCompoundDependencyCycleWithoutPartialOutput) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *A = Context.getOrCreateSymbol("canonical_a");
  MCSymbol *B = Context.getOrCreateSymbol("canonical_b");
  expectSuccess(Streamer->registerUserSymbol(*A, "a"));
  expectSuccess(Streamer->registerUserSymbol(*B, "b"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(A);
  Streamer->emitValue(
      MCBinaryExpr::createAdd(MCSymbolRefExpr::create(B, Context),
                              MCConstantExpr::create(0, Context), Context),
      8);
  Streamer->emitLabel(B);
  Streamer->emitValue(
      MCBinaryExpr::createAdd(MCSymbolRefExpr::create(A, Context),
                              MCConstantExpr::create(0, Context), Context),
      8);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "MMIXAL definition dependency cycle");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest,
       RejectsUndefinedCompoundExpressionWithoutPartialOutput) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Data = Context.getOrCreateSymbol("canonical_data");
  MCSymbol *Missing = Context.getOrCreateSymbol("canonical_missing");
  expectSuccess(Streamer->registerUserSymbol(*Data, "data"));
  expectSuccess(Streamer->registerUserSymbol(*Missing, "missing"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(Data);
  Streamer->emitValue(
      MCBinaryExpr::createAdd(MCSymbolRefExpr::create(Missing, Context),
                              MCConstantExpr::create(8, Context), Context),
      8);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL symbol 'canonical_missing' has no allocated definition");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest,
       RejectsSameItemFutureCompoundExpressionWithoutPartialOutput) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Data = Context.getOrCreateSymbol("canonical_data");
  MCSymbol *Alias = Context.getOrCreateSymbol("canonical_alias");
  expectSuccess(Streamer->registerUserSymbol(*Data, "data"));
  expectSuccess(Streamer->registerUserSymbol(*Alias, "alias"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(Data);
  Streamer->emitValue(
      MCBinaryExpr::createAdd(MCSymbolRefExpr::create(Alias, Context),
                              MCConstantExpr::create(8, Context), Context),
      8);
  Streamer->emitAssignment(Alias, MCSymbolRefExpr::create(Data, Context));
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL data expression references symbol 'canonical_alias' "
            "before its definition");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest,
       RejectsUnsupportedRelocationVariantWithoutPartialOutput) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);

  MCSymbol *Data = Context.getOrCreateSymbol("canonical_data");
  MCSymbol *Target = Context.getOrCreateSymbol("canonical_target");
  expectSuccess(Streamer->registerUserSymbol(*Data, "data"));
  expectSuccess(Streamer->registerUserSymbol(*Target, "target"));
  Streamer->switchSection(
      getSection(Context, ".rodata", ELF::SHT_PROGBITS, ELF::SHF_ALLOC));
  Streamer->emitLabel(Data);
  Streamer->emitValue(MCSymbolRefExpr::create(
                          Target, MCSymbolRefExpr::VK_COFF_IMGREL32, Context),
                      8);
  Streamer->emitLabel(Target);
  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic, "unsupported MMIXAL symbol reference variant");
  EXPECT_TRUE(Output.empty());
}

TEST_F(MMIXALAsmStreamerTest, ReportsLayoutFailureBeforeEmission) {
  MCContext Context(TT, MAI, *MRI, *STI);
  std::string Diagnostic;
  captureDiagnostic(Context, Diagnostic);
  std::string Output;
  raw_string_ostream OutputOS(Output);
  auto Streamer = createStreamer(Context, OutputOS);
  Streamer->switchSection(getSection(Context, ".text", ELF::SHT_PROGBITS,
                                     ELF::SHF_ALLOC | ELF::SHF_EXECINSTR));

  MCInst Inst;
  Inst.setOpcode(MMIX::ADD);
  Streamer->emitInstruction(Inst, *STI);
  Streamer->emitCodeAlignment(Align(8), *STI, 3);
  Streamer->emitInstruction(Inst, *STI);

  Streamer->finish();

  EXPECT_TRUE(Context.hadError());
  EXPECT_EQ(Diagnostic,
            "MMIXAL layout item at source order 1: alignment requires 4 "
            "bytes of padding, exceeding the maximum of 3");
  EXPECT_TRUE(Output.empty());
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
