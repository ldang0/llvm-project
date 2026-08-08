//===-- MMIXAsmParser.cpp - Parse MMIX assembly to MCInst instructions -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/SMLoc.h"
#include <cassert>
#include <cstdint>
#include <memory>

using namespace llvm;

static MCRegister MatchRegisterName(StringRef Name);
static const char *getSubtargetFeatureName(uint64_t Val);

namespace {

class MMIXOperand : public MCParsedAsmOperand {
public:
  enum KindTy { Token, Register, Immediate };

private:
  KindTy Kind;
  SMLoc Start, End;
  StringRef TokenValue;
  MCRegister RegisterValue;
  const MCExpr *ImmediateValue = nullptr;

  MMIXOperand(KindTy Kind, SMLoc Start, SMLoc End)
      : Kind(Kind), Start(Start), End(End) {}

public:
  static std::unique_ptr<MMIXOperand> createToken(StringRef Value,
                                                  SMLoc Loc) {
    auto Operand = std::unique_ptr<MMIXOperand>(
        new MMIXOperand(Token, Loc, Loc));
    Operand->TokenValue = Value;
    return Operand;
  }

  static std::unique_ptr<MMIXOperand> createReg(MCRegister Reg, SMLoc Start,
                                                 SMLoc End) {
    auto Operand = std::unique_ptr<MMIXOperand>(
        new MMIXOperand(Register, Start, End));
    Operand->RegisterValue = Reg;
    return Operand;
  }

  static std::unique_ptr<MMIXOperand> createImm(const MCExpr *Value,
                                                SMLoc Start, SMLoc End) {
    auto Operand = std::unique_ptr<MMIXOperand>(
        new MMIXOperand(Immediate, Start, End));
    Operand->ImmediateValue = Value;
    return Operand;
  }

  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc() const override { return End; }
  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }
  MCRegister getReg() const override {
    assert(isReg() && "not a register operand");
    return RegisterValue;
  }
  const MCExpr *getImm() const {
    assert(isImm() && "not an immediate operand");
    return ImmediateValue;
  }
  StringRef getToken() const { return TokenValue; }

  bool isAbsoluteInRange(int64_t Min, int64_t Max) const {
    int64_t Value;
    return isImm() && getImm()->evaluateAsAbsolute(Value) && Value >= Min &&
           Value <= Max;
  }

  bool isAbsoluteInRangeOrSymbolic(int64_t Min, int64_t Max) const {
    if (!isImm())
      return false;
    int64_t Value;
    return !getImm()->evaluateAsAbsolute(Value) ||
           (Value >= Min && Value <= Max);
  }

  void print(raw_ostream &OS, const MCAsmInfo &) const override {
    if (isToken())
      OS << TokenValue;
    else if (isReg())
      OS << "register";
    else
      OS << "immediate";
  }

  bool isGPR64Op() const { return isReg(); }
  bool isFPR64Op() const { return isReg(); }
  bool isSPR64Op() const { return isReg(); }
  bool isMMIXImm64() const { return isAbsoluteInRange(INT64_MIN, INT64_MAX); }
  bool isMMIXUImm8() const { return isAbsoluteInRange(0, UINT8_MAX); }
  bool isMMIXUImm16() const {
    return isAbsoluteInRangeOrSymbolic(0, UINT16_MAX);
  }
  bool isMMIXUImm24() const { return isAbsoluteInRange(0, 0xffffff); }
  bool isMMIXRoundingMode() const { return isAbsoluteInRange(0, 4); }
  bool isMMIXResumeMode() const { return isAbsoluteInRange(0, 1); }
  bool isMMIXSyncMode() const { return isAbsoluteInRange(0, 7); }
  bool isMMIXRegOrImm8() const {
    return isReg() || isAbsoluteInRange(0, UINT8_MAX);
  }
  bool isMMIXBranchTarget() const { return isImm(); }
  bool isMMIXJumpTarget() const { return isImm(); }
  bool isMMIXMemOffset8() const { return isAbsoluteInRange(0, UINT8_MAX); }
  bool isMMIXMemAddr() const { return isAbsoluteInRange(0, UINT8_MAX); }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && isReg());
    Inst.addOperand(MCOperand::createReg(getReg()));
  }
  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && isImm());
    int64_t Value;
    if (getImm()->evaluateAsAbsolute(Value))
      Inst.addOperand(MCOperand::createImm(Value));
    else
      Inst.addOperand(MCOperand::createExpr(getImm()));
  }
  void addGPR64OpOperands(MCInst &Inst, unsigned N) const {
    addRegOperands(Inst, N);
  }
  void addFPR64OpOperands(MCInst &Inst, unsigned N) const {
    addRegOperands(Inst, N);
  }
  void addSPR64OpOperands(MCInst &Inst, unsigned N) const {
    addRegOperands(Inst, N);
  }
  void addMMIXImm64Operands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXUImm8Operands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXUImm16Operands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXUImm24Operands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXRoundingModeOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXResumeModeOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXSyncModeOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXRegOrImm8Operands(MCInst &Inst, unsigned N) const {
    if (isReg())
      addRegOperands(Inst, N);
    else
      addImmOperands(Inst, N);
  }
  void addMMIXBranchTargetOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXJumpTargetOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXMemOffset8Operands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
  void addMMIXMemAddrOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }
};

class MMIXAsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

#define GET_ASSEMBLER_HEADER
#include "MMIXGenAsmMatcher.inc"

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  void onBeginOfFile() override;

public:
  MMIXAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                const MCInstrInfo &MII)
      : MCTargetAsmParser(STI, MII), Parser(Parser) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }
};

} // namespace

void MMIXAsmParser::onBeginOfFile() {
  const unsigned AsmVariant =
      getContext().getAsmInfo().getOutputAssemblerDialect();
  if (AsmVariant == MMIXII::CanonicalAsmVariant)
    return;
  if (AsmVariant == MMIXII::MMIXALAsmVariant) {
    if (!Parser.getStreamer().isObj())
      return;
    Error(getLexer().getLoc(),
          "MMIXAL complete-source emission is not available");
    return;
  }
  Error(
      getLexer().getLoc(),
      Twine(
          "MMIX complete-source emission does not support assembly variant ") +
          Twine(AsmVariant));
}

bool MMIXAsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {
  StartLoc = Parser.getTok().getLoc();
  StringRef Name = Parser.getTok().getString();
  Reg = MatchRegisterName(Name);
  if (!Reg)
    return true;
  EndLoc = Parser.getTok().getEndLoc();
  Parser.Lex();
  return false;
}

ParseStatus MMIXAsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                             SMLoc &EndLoc) {
  if (Parser.getTok().isNot(AsmToken::Identifier))
    return ParseStatus::NoMatch;
  if (parseRegister(Reg, StartLoc, EndLoc))
    return ParseStatus::NoMatch;
  return ParseStatus::Success;
}

bool MMIXAsmParser::parseInstruction(ParseInstructionInfo &, StringRef Name,
                                     SMLoc NameLoc,
                                     OperandVector &Operands) {
  Operands.push_back(MMIXOperand::createToken(Name, NameLoc));
  if (Parser.getTok().is(AsmToken::EndOfStatement))
    return false;

  while (true) {
    SMLoc Start = Parser.getTok().getLoc();
    SMLoc End = Parser.getTok().getEndLoc();
    MCRegister Reg;
    if (Parser.getTok().is(AsmToken::Identifier) &&
        !parseRegister(Reg, Start, End)) {
      Operands.push_back(MMIXOperand::createReg(Reg, Start, End));
    } else {
      const MCExpr *Expr = nullptr;
      if (Parser.parseExpression(Expr))
        return true;
      Operands.push_back(MMIXOperand::createImm(Expr, Start, End));
    }

    if (Parser.getTok().is(AsmToken::EndOfStatement))
      break;
    if (Parser.getTok().isNot(AsmToken::Comma))
      return Error(Parser.getTok().getLoc(), "expected comma");
    Parser.Lex();
  }
  return false;
}

bool MMIXAsmParser::matchAndEmitInstruction(
    SMLoc IDLoc, unsigned &Opcode, OperandVector &Operands, MCStreamer &Out,
    uint64_t &ErrorInfo, bool MatchingInlineAsm) {
  MCInst Inst;
  FeatureBitset MissingFeatures;
  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MissingFeatures,
                              MatchingInlineAsm)) {
  case Match_Success:
    Inst.setLoc(IDLoc);
    Opcode = Inst.getOpcode();
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MissingFeature: {
    assert(MissingFeatures.any() && "missing feature was not reported");
    std::string Message = "instruction requires:";
    for (unsigned Feature : MissingFeatures) {
      Message += " ";
      Message += getSubtargetFeatureName(Feature);
    }
    return Error(IDLoc, Message);
  }
  case Match_InvalidOperand:
    return Error(IDLoc, "invalid operand for instruction");
  default:
    return Error(IDLoc, "invalid instruction");
  }
}

#define GET_REGISTER_MATCHER
#define GET_SUBTARGET_FEATURE_NAME
#define GET_MATCHER_IMPLEMENTATION
#include "MMIXGenAsmMatcher.inc"

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMMIXAsmParser() {
  RegisterMCAsmParser<MMIXAsmParser> X(getTheMMIXTarget());
}
