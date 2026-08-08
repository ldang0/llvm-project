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
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <array>
#include <initializer_list>
#include <iterator>
#include <limits>
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
  MCContext Ctx{TT, MAI, *MRI, *STI};
  MMIXALInstPrinter Printer{MAI, *MII, *MRI};

  std::string printAt(uint64_t Address, unsigned Opcode,
                      std::initializer_list<MCOperand> Operands) {
    MCInst Inst;
    Inst.setOpcode(Opcode);
    for (const MCOperand &Operand : Operands)
      Inst.addOperand(Operand);

    std::string Output;
    raw_string_ostream OS(Output);
    Printer.printInst(&Inst, Address, "", *STI, OS);
    OS.flush();
    return Output;
  }

  std::string print(unsigned Opcode,
                    std::initializer_list<MCOperand> Operands) {
    return printAt(0, Opcode, Operands);
  }

  MCOperand constantTarget(uint64_t Target) {
    return MCOperand::createExpr(
        MCConstantExpr::create(static_cast<int64_t>(Target), Ctx));
  }

  MCOperand symbolicTarget(StringRef Name) {
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(Ctx.getOrCreateSymbol(Name), Ctx));
  }

  const MCExpr *constant(int64_t Value) {
    return MCConstantExpr::create(Value, Ctx);
  }

  const MCExpr *definedSymbol(StringRef Name, int64_t Value) {
    MCSymbol *Symbol = Ctx.getOrCreateSymbol(Name);
    Symbol->setVariableValue(constant(Value));
    return MCSymbolRefExpr::create(Symbol, Ctx);
  }

  static MCOperand expression(const MCExpr *Expr) {
    return MCOperand::createExpr(Expr);
  }

  void expectOpcodePair(unsigned RegisterOpcode, unsigned ImmediateOpcode,
                        std::initializer_list<MCOperand> LeadingOperands,
                        StringRef ExpectedPrefix) {
    auto Check = [&](unsigned Opcode, MCOperand SelectableOperand,
                     StringRef ExpectedOperand) {
      MCInst Inst;
      Inst.setOpcode(Opcode);
      for (const MCOperand &Operand : LeadingOperands)
        Inst.addOperand(Operand);
      Inst.addOperand(SelectableOperand);

      std::string Output;
      raw_string_ostream OS(Output);
      Printer.printInst(&Inst, 0, "", *STI, OS);
      OS.flush();

      EXPECT_EQ(Output, (ExpectedPrefix + ExpectedOperand).str());
      EXPECT_EQ(Inst.getOpcode(), Opcode);
    };

    Check(RegisterOpcode, MCOperand::createReg(MMIX::R7), "$7");
    Check(ImmediateOpcode, MCOperand::createImm(7), "7");
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

TEST_F(MMIXALInstPrinterTest, PrintsSupportedExpressionsWithMMIXALPrecedence) {
  const MCExpr *A = definedSymbol("expr_a", 2);
  const MCExpr *B = definedSymbol("expr_b", 3);
  const MCExpr *C = definedSymbol("expr_c", 4);

  EXPECT_EQ(print(MMIX::SETH,
                  {MCOperand::createReg(MMIX::R1), expression(constant(7))}),
            "\tSETH $1, 7");
  EXPECT_EQ(print(MMIX::SETH, {MCOperand::createReg(MMIX::R1), expression(A)}),
            "\tSETH $1, expr_a");
  EXPECT_EQ(
      print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                         expression(MCBinaryExpr::createAdd(
                             A, MCBinaryExpr::createMul(B, C, Ctx), Ctx))}),
      "\tSETH $1, expr_a+expr_b*expr_c");
  EXPECT_EQ(
      print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                         expression(MCBinaryExpr::createMul(
                             MCBinaryExpr::createAdd(A, B, Ctx), C, Ctx))}),
      "\tSETH $1, (expr_a+expr_b)*expr_c");
  EXPECT_EQ(
      print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                         expression(MCBinaryExpr::createSub(
                             A, MCBinaryExpr::createSub(B, C, Ctx), Ctx))}),
      "\tSETH $1, expr_a-(expr_b-expr_c)");

  EXPECT_EQ(print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                               expression(MCUnaryExpr::createPlus(A, Ctx))}),
            "\tSETH $1, +expr_a");
  EXPECT_EQ(print(MMIX::SETH,
                  {MCOperand::createReg(MMIX::R1),
                   expression(MCBinaryExpr::createAnd(
                       MCUnaryExpr::createMinus(A, Ctx), constant(255), Ctx))}),
            "\tSETH $1, -expr_a&255");
  EXPECT_EQ(print(MMIX::SETH,
                  {MCOperand::createReg(MMIX::R1),
                   expression(MCBinaryExpr::createAnd(
                       MCUnaryExpr::createNot(A, Ctx), constant(255), Ctx))}),
            "\tSETH $1, ~expr_a&255");

  struct BinaryCase {
    MCBinaryExpr::Opcode Opcode;
    int64_t LHS;
    int64_t RHS;
    const char *Expected;
  };
  static constexpr BinaryCase Cases[] = {
      {MCBinaryExpr::Add, 8, 2, "8+2"},  {MCBinaryExpr::Sub, 8, 2, "8-2"},
      {MCBinaryExpr::Mul, 8, 2, "8*2"},  {MCBinaryExpr::And, 8, 2, "8&2"},
      {MCBinaryExpr::Or, 8, 2, "8|2"},   {MCBinaryExpr::Xor, 8, 2, "8^2"},
      {MCBinaryExpr::Shl, 8, 2, "8<<2"}, {MCBinaryExpr::LShr, 8, 2, "8>>2"},
  };
  for (const BinaryCase &Case : Cases) {
    const MCExpr *Expr = MCBinaryExpr::create(Case.Opcode, constant(Case.LHS),
                                              constant(Case.RHS), Ctx);
    EXPECT_EQ(
        print(MMIX::SETH, {MCOperand::createReg(MMIX::R1), expression(Expr)}),
        std::string("\tSETH $1, ") + Case.Expected);
  }
}

TEST_F(MMIXALInstPrinterTest, EnforcesMMIXALFutureExpressionRules) {
  const MCExpr *Future =
      MCSymbolRefExpr::create(Ctx.getOrCreateSymbol("future_target"), Ctx);
  EXPECT_EQ(
      printAt(0, MMIX::JMP, {expression(MCUnaryExpr::createPlus(Future, Ctx))}),
      "\tJMP +future_target");
  EXPECT_DEATH(
      printAt(0, MMIX::JMP,
              {expression(MCBinaryExpr::createAdd(Future, constant(4), Ctx))}),
      "unsupported MMIXAL future instruction expression");
  EXPECT_DEATH(
      print(MMIX::SETH, {MCOperand::createReg(MMIX::R1), expression(Future)}),
      "unsupported MMIXAL future instruction expression");
}

TEST_F(MMIXALInstPrinterTest, RejectsUnsupportedMMIXALExpressions) {
  const MCExpr *Defined = definedSymbol("defined_symbol", 8);
  MCSymbol *VariantSymbol = Ctx.getOrCreateSymbol("variant_symbol");
  VariantSymbol->setVariableValue(constant(8));

  EXPECT_DEATH(
      print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                         expression(MCUnaryExpr::createLNot(Defined, Ctx))}),
      "unsupported MMIXAL unary expression operator");
  EXPECT_DEATH(
      print(MMIX::SETH,
            {MCOperand::createReg(MMIX::R1),
             expression(MCBinaryExpr::createDiv(Defined, constant(2), Ctx))}),
      "unsupported MMIXAL binary expression operator");
  EXPECT_DEATH(
      print(MMIX::SETH,
            {MCOperand::createReg(MMIX::R1),
             expression(MCSymbolRefExpr::create(
                 VariantSymbol, MCSymbolRefExpr::FirstTargetSpecifier, Ctx))}),
      "unsupported MMIXAL symbol reference variant");
  EXPECT_DEATH(
      print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                         expression(MCSpecifierExpr::create(Defined, 1, Ctx))}),
      "unsupported MMIXAL relocation specifier");
  EXPECT_DEATH(print(MMIX::SETH, {MCOperand::createReg(MMIX::R1),
                                  expression(constant(65536))}),
               "MMIXAL expression is not representable as wyde operand");
  EXPECT_DEATH(print(MMIX::ADDI,
                     {MCOperand::createReg(MMIX::R1),
                      MCOperand::createReg(MMIX::R2), expression(constant(7))}),
               "MMIXAL immediate-form instruction requires a byte operand");
}

TEST_F(MMIXALInstPrinterTest, PrintsRegisterImmediateOpcodePairs) {
  expectOpcodePair(
      MMIX::ADD, MMIX::ADDI,
      {MCOperand::createReg(MMIX::R1), MCOperand::createReg(MMIX::R2)},
      "\tADD $1, $2, ");
  expectOpcodePair(MMIX::CSN, MMIX::CSNI,
                   {MCOperand::createReg(MMIX::R1),
                    MCOperand::createReg(MMIX::R1),
                    MCOperand::createReg(MMIX::R2)},
                   "\tCSN $1, $2, ");
  expectOpcodePair(
      MMIX::LDB, MMIX::LDBI,
      {MCOperand::createReg(MMIX::R1), MCOperand::createReg(MMIX::R2)},
      "\tLDB $1, $2, ");
  expectOpcodePair(
      MMIX::STB, MMIX::STBI,
      {MCOperand::createReg(MMIX::R1), MCOperand::createReg(MMIX::R2)},
      "\tSTB $1, $2, ");
  expectOpcodePair(MMIX::PRELD, MMIX::PRELDI,
                   {MCOperand::createImm(1), MCOperand::createReg(MMIX::R2)},
                   "\tPRELD 1, $2, ");
  expectOpcodePair(MMIX::CSWAP, MMIX::CSWAPI,
                   {MCOperand::createReg(MMIX::R1),
                    MCOperand::createReg(MMIX::R1),
                    MCOperand::createReg(MMIX::R2)},
                   "\tCSWAP $1, $2, ");
  expectOpcodePair(MMIX::FLOT, MMIX::FLOTI,
                   {MCOperand::createReg(MMIX::R1), MCOperand::createImm(4)},
                   "\tFLOT $1, ROUND_NEAR, ");
  expectOpcodePair(
      MMIX::GO, MMIX::GOI,
      {MCOperand::createReg(MMIX::R1), MCOperand::createReg(MMIX::R2)},
      "\tGO $1, $2, ");
  expectOpcodePair(
      MMIX::PUSHGO, MMIX::PUSHGOI,
      {MCOperand::createReg(MMIX::R1), MCOperand::createReg(MMIX::R2)},
      "\tPUSHGO $1, $2, ");
  expectOpcodePair(MMIX::PUT, MMIX::PUTI, {MCOperand::createReg(MMIX::RB)},
                   "\tPUT rB, ");
}

TEST_F(MMIXALInstPrinterTest, RejectsOpcodePairOperandKindMismatch) {
  EXPECT_DEATH(print(MMIX::ADDI, {MCOperand::createReg(MMIX::R0),
                                  MCOperand::createReg(MMIX::R1)}),
               "missing MMIXAL selectable operand");
  EXPECT_DEATH(print(MMIX::ADD,
                     {MCOperand::createReg(MMIX::R0),
                      MCOperand::createReg(MMIX::R1), MCOperand::createImm(2)}),
               "register-form instruction requires a register operand");
  EXPECT_DEATH(print(MMIX::ADDI, {MCOperand::createReg(MMIX::R0),
                                  MCOperand::createReg(MMIX::R1),
                                  MCOperand::createReg(MMIX::R2)}),
               "immediate-form instruction requires a byte operand");
}

TEST_F(MMIXALInstPrinterTest, PrintsEveryRelativeInstructionFamily) {
  EXPECT_EQ(printAt(0x1000, MMIX::BN,
                    {MCOperand::createReg(MMIX::R1),
                     symbolicTarget("branch_target")}),
            "\tBN $1, branch_target");
  EXPECT_EQ(printAt(0x1000, MMIX::BNB,
                    {MCOperand::createReg(MMIX::R1),
                     symbolicTarget("branch_target")}),
            "\tBN $1, branch_target");
  EXPECT_EQ(printAt(0x1000, MMIX::PBN,
                    {MCOperand::createReg(MMIX::R2),
                     symbolicTarget("probable_target")}),
            "\tPBN $2, probable_target");
  EXPECT_EQ(printAt(0x1000, MMIX::PBNB,
                    {MCOperand::createReg(MMIX::R2),
                     symbolicTarget("probable_target")}),
            "\tPBN $2, probable_target");
  EXPECT_EQ(printAt(0x1000, MMIX::JMP, {symbolicTarget("jump_target")}),
            "\tJMP jump_target");
  EXPECT_EQ(printAt(0x1000, MMIX::JMPB, {symbolicTarget("jump_target")}),
            "\tJMP jump_target");
  EXPECT_EQ(printAt(0x1000, MMIX::GETA,
                    {MCOperand::createReg(MMIX::R3),
                     symbolicTarget("address_target")}),
            "\tGETA $3, address_target");
  EXPECT_EQ(printAt(0x1000, MMIX::GETAB,
                    {MCOperand::createReg(MMIX::R3),
                     symbolicTarget("address_target")}),
            "\tGETA $3, address_target");
  EXPECT_EQ(
      printAt(0x1000, MMIX::PUSHJ,
              {MCOperand::createReg(MMIX::R4), symbolicTarget("call_target")}),
      "\tPUSHJ $4, call_target");
  EXPECT_EQ(
      printAt(0x1000, MMIX::PUSHJB,
              {MCOperand::createReg(MMIX::R4), symbolicTarget("call_target")}),
      "\tPUSHJ $4, call_target");
}

TEST_F(MMIXALInstPrinterTest, PrintsRelativeTargetBoundaries) {
  constexpr uint64_t BranchAddress = 0x100000;
  constexpr uint64_t BranchForwardLimit = BranchAddress + 65535 * 4;
  constexpr uint64_t BranchBackwardLimit = BranchAddress - 65536 * 4;
  EXPECT_EQ(
      printAt(BranchAddress, MMIX::BN,
              {MCOperand::createReg(MMIX::R1), constantTarget(BranchAddress)}),
      "\tBN $1, " + std::to_string(BranchAddress));
  EXPECT_EQ(printAt(BranchAddress, MMIX::GETA,
                    {MCOperand::createReg(MMIX::R2),
                     constantTarget(BranchForwardLimit)}),
            "\tGETA $2, " + std::to_string(BranchForwardLimit));
  EXPECT_EQ(printAt(BranchAddress, MMIX::PUSHJB,
                    {MCOperand::createReg(MMIX::R3),
                     constantTarget(BranchBackwardLimit)}),
            "\tPUSHJ $3, " + std::to_string(BranchBackwardLimit));

  constexpr uint64_t JumpAddress = 0x10000000;
  constexpr uint64_t JumpForwardLimit = JumpAddress + 16777215 * 4;
  constexpr uint64_t JumpBackwardLimit = JumpAddress - 16777216 * 4;
  EXPECT_EQ(printAt(JumpAddress, MMIX::JMP, {constantTarget(JumpForwardLimit)}),
            "\tJMP " + std::to_string(JumpForwardLimit));
  EXPECT_EQ(
      printAt(JumpAddress, MMIX::JMPB, {constantTarget(JumpBackwardLimit)}),
      "\tJMP " + std::to_string(JumpBackwardLimit));
}

TEST_F(MMIXALInstPrinterTest, UsesScopedMappedSymbolNames) {
  MCSymbol *Target = Ctx.getOrCreateSymbol(".Lcanonical_target");
  MCInst Inst;
  Inst.setOpcode(MMIX::JMP);
  Inst.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(Target, Ctx)));

  std::string Output;
  raw_string_ostream OS(Output);
  Printer.printInstWithSymbolNames(
      &Inst, 0x100, *STI,
      [](const MCSymbol &) -> MMIXALSymbolPrintInfo {
        return {"mapped_target", false};
      },
      OS);
  OS.flush();

  EXPECT_EQ(Output, "\tJMP mapped_target");
  EXPECT_EQ(
      printAt(0x100, MMIX::JMP,
              {MCOperand::createExpr(MCSymbolRefExpr::create(Target, Ctx))}),
      "\tJMP .Lcanonical_target");
}

TEST_F(MMIXALInstPrinterTest, RejectsInvalidRelativeTargets) {
  constexpr uint64_t Address = 0x100000;
  EXPECT_DEATH(
      printAt(Address, MMIX::BNB,
              {MCOperand::createReg(MMIX::R1), constantTarget(Address)}),
      "relative target direction does not match instruction");
  EXPECT_DEATH(
      printAt(Address, MMIX::BN,
              {MCOperand::createReg(MMIX::R1), constantTarget(Address - 4)}),
      "relative target direction does not match instruction");
  EXPECT_DEATH(
      printAt(Address, MMIX::BN,
              {MCOperand::createReg(MMIX::R1), constantTarget(Address + 2)}),
      "relative target is not four-byte aligned");
  EXPECT_DEATH(printAt(Address, MMIX::BN,
                       {MCOperand::createReg(MMIX::R1),
                        constantTarget(Address + 65536 * 4)}),
               "relative target is out of range");
  EXPECT_DEATH(printAt(Address, MMIX::BNB,
                       {MCOperand::createReg(MMIX::R1),
                        constantTarget(Address - 65537 * 4)}),
               "relative target is out of range");
  EXPECT_DEATH(
      printAt(Address, MMIX::JMP, {constantTarget(Address + 16777216 * 4)}),
      "relative target is out of range");
  EXPECT_EQ(printAt(Address, MMIX::JMP, {MCOperand::createImm(1)}), "\tJMP 1");
  EXPECT_DEATH(printAt(Address, MMIX::JMP, {MCOperand::createImm(-1)}),
               "decoded relative target does not match instruction");
  EXPECT_DEATH(
      printAt(Address, MMIX::BNB,
              {MCOperand::createReg(MMIX::R1), MCOperand::createImm(0)}),
      "decoded relative target does not match instruction");
  EXPECT_DEATH(
      printAt(Address, MMIX::BN,
              {MCOperand::createReg(MMIX::R1), MCOperand::createImm(65536)}),
      "decoded relative target does not match instruction");
}

TEST_F(MMIXALInstPrinterTest, PreservesMixedRegisterAndImmediateKinds) {
  EXPECT_EQ(print(MMIX::SWYM,
                  {MCOperand::createReg(MMIX::R7), MCOperand::createImm(7),
                   MCOperand::createReg(MMIX::R255)}),
            "\tSWYM $7, 7, $255");
  EXPECT_EQ(print(MMIX::TRAP,
                  {MCOperand::createReg(MMIX::R0), MCOperand::createImm(1),
                   MCOperand::createReg(MMIX::R255)}),
            "\tTRAP $0, 1, $255");
  EXPECT_EQ(print(MMIX::TRIP,
                  {MCOperand::createImm(255), MCOperand::createReg(MMIX::R1),
                   MCOperand::createImm(0)}),
            "\tTRIP 255, $1, 0");
}

TEST_F(MMIXALInstPrinterTest, PrintsIdiosyncraticIntegerOperandForms) {
  EXPECT_EQ(
      print(MMIX::NEG, {MCOperand::createReg(MMIX::R1), MCOperand::createImm(2),
                        MCOperand::createReg(MMIX::R3)}),
      "\tNEG $1, 2, $3");
  EXPECT_EQ(
      print(MMIX::NEGI, {MCOperand::createReg(MMIX::R1),
                         MCOperand::createImm(2), MCOperand::createImm(3)}),
      "\tNEG $1, 2, 3");
}

TEST_F(MMIXALInstPrinterTest, PrintsContextStateFixedFields) {
  EXPECT_EQ(print(MMIX::SAVE, {MCOperand::createReg(MMIX::R255)}),
            "\tSAVE $255, 0");
  EXPECT_EQ(print(MMIX::UNSAVE, {MCOperand::createReg(MMIX::R0)}),
            "\tUNSAVE $0");
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
      print(MMIX::SWYM, {MCOperand::createImm(256), MCOperand::createImm(0),
                         MCOperand::createImm(0)}),
      "invalid MMIXAL register-or-byte operand");
  EXPECT_DEATH(
      print(MMIX::SWYM, {MCOperand::createReg(MMIX::RB),
                         MCOperand::createImm(0), MCOperand::createImm(0)}),
      "invalid MMIXAL general register operand");
  EXPECT_DEATH(print(MMIX::GET, {MCOperand::createReg(MMIX::R0),
                                 MCOperand::createReg(MMIX::R0)}),
               "invalid MMIXAL special register operand");
}

TEST_F(MMIXALInstPrinterTest, PrintsEveryArchitecturalOpcodeRecord) {
  constexpr unsigned MissingOpcode = std::numeric_limits<unsigned>::max();
  std::array<unsigned, 256> RecordsByArchitecturalOpcode;
  RecordsByArchitecturalOpcode.fill(MissingOpcode);

  unsigned ArchitecturalRecordCount = 0;
  for (unsigned Opcode = 0; Opcode != MII->getNumOpcodes(); ++Opcode) {
    const MCInstrDesc &Desc = MII->get(Opcode);
    const MMIXII::MMIXALSelectionKind Selection =
        MMIXII::getMMIXALSelection(Desc.TSFlags);
    if (Selection == MMIXII::MMIXALSelectionUnclassified)
      continue;

    EXPECT_FALSE(Desc.isPseudo()) << MII->getName(Opcode).str();
    const unsigned ArchitecturalOpcode = Desc.TSFlags & MMIXII::OpcodeMask;
    ASSERT_EQ(RecordsByArchitecturalOpcode[ArchitecturalOpcode], MissingOpcode)
        << "duplicate architectural opcode " << ArchitecturalOpcode;
    RecordsByArchitecturalOpcode[ArchitecturalOpcode] = Opcode;
    ++ArchitecturalRecordCount;
  }
  ASSERT_EQ(ArchitecturalRecordCount, 256u);

  constexpr uint64_t Address = 0x1000;
  for (unsigned ArchitecturalOpcode = 0; ArchitecturalOpcode != 256;
       ++ArchitecturalOpcode) {
    const unsigned Opcode = RecordsByArchitecturalOpcode[ArchitecturalOpcode];
    ASSERT_NE(Opcode, MissingOpcode)
        << "missing architectural opcode " << ArchitecturalOpcode;

    const MCInstrDesc &Desc = MII->get(Opcode);
    const MMIXII::MMIXALSelectionKind Selection =
        MMIXII::getMMIXALSelection(Desc.TSFlags);
    ASSERT_TRUE(Selection == MMIXII::MMIXALSelectionExact ||
                Selection == MMIXII::MMIXALSelectionRegister ||
                Selection == MMIXII::MMIXALSelectionImmediate ||
                Selection == MMIXII::MMIXALSelectionForward ||
                Selection == MMIXII::MMIXALSelectionBackward)
        << MII->getName(Opcode).str();

    MCInst Inst;
    Inst.setOpcode(Opcode);
    for (const MCOperandInfo &Operand : Desc.operands()) {
      if (Operand.RegClass == MMIX::GPR64RegClassID ||
          Operand.RegClass == MMIX::FPR64RegClassID) {
        Inst.addOperand(MCOperand::createReg(MMIX::R1));
        continue;
      }
      if (Operand.RegClass == MMIX::SPR64RegClassID) {
        Inst.addOperand(MCOperand::createReg(MMIX::RB));
        continue;
      }

      switch (Operand.OperandType) {
      case MCOI::OPERAND_PCREL: {
        const uint64_t Target = Selection == MMIXII::MMIXALSelectionBackward
                                    ? Address - 4
                                    : Address + 4;
        Inst.addOperand(constantTarget(Target));
        break;
      }
      case MMIXII::OPERAND_UIMM8:
      case MMIXII::OPERAND_UIMM16:
      case MMIXII::OPERAND_REG_OR_IMM8:
        Inst.addOperand(MCOperand::createImm(1));
        break;
      case MMIXII::OPERAND_ROUNDING_MODE:
      case MMIXII::OPERAND_RESUME_MODE:
      case MMIXII::OPERAND_SYNC_MODE:
        Inst.addOperand(MCOperand::createImm(0));
        break;
      default:
        FAIL() << "unhandled operand type for " << MII->getName(Opcode).str();
      }
    }

    std::string Output;
    raw_string_ostream OS(Output);
    Printer.printInst(&Inst, Address, "", *STI, OS);
    OS.flush();
    EXPECT_FALSE(Output.empty()) << MII->getName(Opcode).str();
  }
}

TEST(MMIXALInstPrinterFactoryTest, PublicFactorySelectsIndependentPrinters) {
  LLVMInitializeMMIXTargetInfo();
  LLVMInitializeMMIXTargetMC();

  const Triple TT("mmix-unknown-elf");
  const MCTargetOptions Options;
  const MMIXMCAsmInfo MAI(TT, Options);
  const std::unique_ptr<MCInstrInfo> MII(createMMIXMCInstrInfo());
  const std::unique_ptr<MCRegisterInfo> MRI(createMMIXMCRegisterInfo(TT));
  const std::unique_ptr<MCSubtargetInfo> STI(
      createMMIXMCSubtargetInfo(TT, "generic", ""));
  const Target &T = getTheMMIXTarget();

  std::unique_ptr<MCInstPrinter> Canonical(
      T.createMCInstPrinter(TT, MMIXII::CanonicalAsmVariant, MAI, *MII, *MRI));
  std::unique_ptr<MCInstPrinter> MMIXAL(
      T.createMCInstPrinter(TT, MMIXII::MMIXALAsmVariant, MAI, *MII, *MRI));
  std::unique_ptr<MCInstPrinter> Unknown(
      T.createMCInstPrinter(TT, 42, MAI, *MII, *MRI));

  EXPECT_NE(Canonical, nullptr);
  EXPECT_NE(MMIXAL, nullptr);
  EXPECT_EQ(Unknown, nullptr);

  MCInst Inst;
  Inst.setOpcode(MMIX::ADD);
  Inst.addOperand(MCOperand::createReg(MMIX::R1));
  Inst.addOperand(MCOperand::createReg(MMIX::R2));
  Inst.addOperand(MCOperand::createReg(MMIX::R3));
  auto Print = [&](MCInstPrinter &Printer) {
    std::string Output;
    raw_string_ostream OS(Output);
    Printer.printInst(&Inst, 0, "", *STI, OS);
    OS.flush();
    return Output;
  };

  EXPECT_EQ(Print(*Canonical), "\tADD r1, r2, r3");
  EXPECT_EQ(Print(*MMIXAL), "\tADD $1, $2, $3");
  EXPECT_EQ(Print(*Canonical), "\tADD r1, r2, r3");
}
