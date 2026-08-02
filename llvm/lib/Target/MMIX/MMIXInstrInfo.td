//===-- MMIXInstrInfo.td - MMIX Instruction Definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def SDT_MMIXCallSeqStart
    : SDCallSeqStart<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def SDT_MMIXCallSeqEnd
    : SDCallSeqEnd<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def MMIXcallseq_start
    : SDNode<"ISD::CALLSEQ_START", SDT_MMIXCallSeqStart,
             [SDNPHasChain, SDNPOutGlue]>;
def MMIXcallseq_end
    : SDNode<"ISD::CALLSEQ_END", SDT_MMIXCallSeqEnd,
             [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def SDT_MMIXLoadStackArg
    : SDTypeProfile<1, 1, [SDTCisVT<1, i64>]>;
def MMIXload_stack_arg
    : SDNode<"MMIXISD::LOAD_STACK_ARG", SDT_MMIXLoadStackArg,
             [SDNPHasChain, SDNPMayLoad]>;
def MMIXret
    : SDNode<"MMIXISD::RET_GLUE", SDTNone, [SDNPHasChain]>;
def MMIXret_value
    : SDNode<"MMIXISD::RET_VALUE_GLUE", SDTNone,
             [SDNPHasChain, SDNPInGlue]>;

// Common operand categories. These records carry the architectural encoding
// constraints used by later MC emitters and parsers, without installing any
// target-specific parser or encoder methods yet.
class MMIXRegisterOperand<RegisterClassLike RegClass, int Width>
    : RegisterOperand<RegClass> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = false;
  string MMIXEncodingKind = "register";
}

def GPR64Op : MMIXRegisterOperand<GPR64, 8>;
def FPR64Op : MMIXRegisterOperand<FPR64, 8>;
def SPR64Op : MMIXRegisterOperand<SPR64, 5>;

class MMIXImmediateOperand<int Width, bit Signed, string Range>
    : Operand<i64> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = Signed;
  string MMIXEncodingKind = "immediate";
  string MMIXEncodingRange = Range;
}

class MMIXImmediateParserOperand<string OperandName> : AsmOperandClass {
  let Name = OperandName;
  let RenderMethod = "addImmOperands";
}

def MMIXImm64AsmOperand : MMIXImmediateParserOperand<"MMIXImm64">;
def MMIXUImm8AsmOperand : MMIXImmediateParserOperand<"MMIXUImm8">;
def MMIXUImm16AsmOperand : MMIXImmediateParserOperand<"MMIXUImm16">;
def MMIXUImm24AsmOperand : MMIXImmediateParserOperand<"MMIXUImm24">;
def MMIXRoundingModeAsmOperand
    : MMIXImmediateParserOperand<"MMIXRoundingMode">;
def MMIXResumeModeAsmOperand
    : MMIXImmediateParserOperand<"MMIXResumeMode">;
def MMIXSyncModeAsmOperand
    : MMIXImmediateParserOperand<"MMIXSyncMode">;

def MMIXImm64 : MMIXImmediateOperand<64, true, "signed-64"> {
  let ParserMatchClass = MMIXImm64AsmOperand;
}
def MMIXUImm8 : MMIXImmediateOperand<8, false, "unsigned-8"> {
  let ParserMatchClass = MMIXUImm8AsmOperand;
}
def MMIXUImm16 : MMIXImmediateOperand<16, false, "unsigned-16"> {
  let ParserMatchClass = MMIXUImm16AsmOperand;
}
def MMIXUImm24 : MMIXImmediateOperand<24, false, "unsigned-24"> {
  let ParserMatchClass = MMIXUImm24AsmOperand;
}
def MMIXRoundingMode : MMIXImmediateOperand<3, false, "rounding-mode"> {
  let ParserMatchClass = MMIXRoundingModeAsmOperand;
  let DecoderMethod = "decodeRoundingMode";
}
def MMIXResumeMode : MMIXImmediateOperand<1, false, "resume-mode"> {
  let ParserMatchClass = MMIXResumeModeAsmOperand;
}
def MMIXSyncMode : MMIXImmediateOperand<3, false, "sync-mode"> {
  let ParserMatchClass = MMIXSyncModeAsmOperand;
}

class MMIXRegOrImmOperand<int Width, string Range>
    : Operand<i64> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = false;
  string MMIXEncodingKind = "register-or-immediate";
  string MMIXEncodingRange = Range;
}

def MMIXRegOrImm8AsmOperand
    : MMIXImmediateParserOperand<"MMIXRegOrImm8"> {
  let RenderMethod = "addMMIXRegOrImm8Operands";
}
def MMIXRegOrImm8 : MMIXRegOrImmOperand<8, "unsigned-8"> {
  let ParserMatchClass = MMIXRegOrImm8AsmOperand;
}

class MMIXPCRelativeOperand<int Width, string Range> : Operand<OtherVT> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = true;
  string MMIXEncodingKind = "pc-relative";
  string MMIXEncodingRange = Range;
  string MMIXFixupKind = "pc-relative";
  int MMIXFixupScale = 4;
  let EncoderMethod = "getPCRelativeOpValue";
  let DecoderMethod = "decodePCRelativeOperand";
  let OperandType = "OPERAND_PCREL";
}

def MMIXBranchTargetAsmOperand
    : MMIXImmediateParserOperand<"MMIXBranchTarget">;
def MMIXJumpTargetAsmOperand : MMIXImmediateParserOperand<"MMIXJumpTarget">;
def MMIXBranchTarget : MMIXPCRelativeOperand<16,
                                             "signed-16-instruction-words"> {
  let ParserMatchClass = MMIXBranchTargetAsmOperand;
}
def MMIXJumpTarget : MMIXPCRelativeOperand<24,
                                           "signed-24-instruction-words"> {
  let ParserMatchClass = MMIXJumpTargetAsmOperand;
}

class MMIXMemoryOffsetOperand<int Width, bit Signed, string Range>
    : Operand<i64> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = Signed;
  string MMIXEncodingKind = "memory-offset";
  string MMIXEncodingRange = Range;
}

def MMIXMemOffset8AsmOperand
    : MMIXImmediateParserOperand<"MMIXMemOffset8">;
def MMIXMemOffset8 : MMIXMemoryOffsetOperand<8, false, "unsigned-8"> {
  let ParserMatchClass = MMIXMemOffset8AsmOperand;
}

def MMIXMemAddrAsmOperand : MMIXImmediateParserOperand<"MMIXMemAddr">;
def MMIXMemAddr : Operand<i64> {
  int MMIXEncodingWidth = 8;
  bit MMIXEncodingSigned = false;
  string MMIXEncodingKind = "memory-address";
  string MMIXEncodingRange = "unsigned-8-offset";
  let ParserMatchClass = MMIXMemAddrAsmOperand;
  let OperandType = "OPERAND_MEMORY";
}

// MMIX instructions are fixed-width 32-bit words stored in big-endian byte
// order. In TableGen bit numbering, the first byte is Inst{31-24}, followed by
// the X, Y, and Z bytes. Concrete instruction formats bind every operand and
// fixed field below so the same Inst description can drive generated encoders
// and decoders.
class MMIXInstruction<dag outs, dag ins, string asmstr> : Instruction {
  let Namespace = "MMIX";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Size = 4;
  bits<32> Inst;
  bits<8> MMIXOpcode = 0;
  bits<8> MMIXX;
  bits<8> MMIXY;
  bits<8> MMIXZ;
  bit MMIXPCRelativeBackward = false;
  bits<5> MMIXPCRelativeWidth = 0;
  let Inst{31-24} = MMIXOpcode;
  let Inst{23-16} = MMIXX;
  let Inst{15-8} = MMIXY;
  let Inst{7-0} = MMIXZ;
  let TSFlags{7-0} = MMIXOpcode;
  let TSFlags{8} = MMIXPCRelativeBackward;
  let TSFlags{13-9} = MMIXPCRelativeWidth;
  string MMIXInventoryStatus = "modeled";
  string MMIXInventoryFamily = "unclassified";
}

// The ordinary integer form has a register or unsigned-byte Z operand. The
// two forms remain separate records so later MC encodings can distinguish
// their opcode fields.
class MMIXIntRegReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$lhs, GPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  bits<8> dst;
  bits<8> lhs;
  bits<8> rhs;
  let MMIXX = dst;
  let MMIXY = lhs;
  let MMIXZ = rhs;
  let MMIXInventoryFamily = "integer";
}

class MMIXIntRegImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$lhs, MMIXUImm8:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  bits<8> dst;
  bits<8> lhs;
  bits<8> rhs;
  let MMIXX = dst;
  let MMIXY = lhs;
  let MMIXZ = rhs;
  let MMIXInventoryFamily = "integer";
}

// NEG and NEGU use an unsigned byte in the Y field even in their register-Z
// form, unlike the ordinary X,Y,Z instruction family.
class MMIXNegReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm8:$lhs, GPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  bits<8> dst;
  bits<8> lhs;
  bits<8> rhs;
  let MMIXX = dst;
  let MMIXY = lhs;
  let MMIXZ = rhs;
  let MMIXInventoryFamily = "integer";
}

class MMIXNegImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm8:$lhs, MMIXUImm8:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  bits<8> dst;
  bits<8> lhs;
  bits<8> rhs;
  let MMIXX = dst;
  let MMIXY = lhs;
  let MMIXZ = rhs;
  let MMIXInventoryFamily = "integer";
}

class MMIXIntConst<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm16:$value),
                      Mnemonic # " $dst, $value"> {
  bits<8> dst;
  bits<16> value;
  let MMIXX = dst;
  let MMIXY = value{15-8};
  let MMIXZ = value{7-0};
  let MMIXInventoryFamily = "integer";
}

class MMIXIntRegRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXIntRegReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXIntRegImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXIntRegImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

// CS* keeps the old X register when its condition is false. Model that X
// operand as both an input and an output so register allocation preserves the
// architectural two-address dependency.
class MMIXCondSetRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$old, GPR64Op:$cond, GPR64Op:$value),
                      Mnemonic # " $dst, $cond, $value"> {
  bits<8> dst;
  bits<8> cond;
  bits<8> value;
  let MMIXX = dst;
  let MMIXY = cond;
  let MMIXZ = value;
  let MMIXOpcode = Opcode;
  let Constraints = "$dst = $old";
  let MMIXInventoryFamily = "integer";
}

class MMIXCondSetImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$old, GPR64Op:$cond, MMIXUImm8:$value),
                      Mnemonic # " $dst, $cond, $value"> {
  bits<8> dst;
  bits<8> cond;
  bits<8> value;
  let MMIXX = dst;
  let MMIXY = cond;
  let MMIXZ = value;
  let MMIXOpcode = Opcode;
  let Constraints = "$dst = $old";
  let MMIXInventoryFamily = "integer";
}

class MMIXNegRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXNegReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXNegImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXNegImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXIntConstOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXIntConst<Mnemonic> {
  let MMIXOpcode = Opcode;
}

// Arithmetic, shifts, comparisons, conditional sets, and bit operations all
// have the architectural register and immediate variants listed here.
def MUL : MMIXIntRegRegOpcode<0x18, "MUL">;
def MULI : MMIXIntRegImmOpcode<0x19, "MUL">;
let Defs = [RH] in {
  def MULU : MMIXIntRegRegOpcode<0x1A, "MULU">;
  def MULUI : MMIXIntRegImmOpcode<0x1B, "MULU">;
}
let Defs = [RR] in {
  def DIV : MMIXIntRegRegOpcode<0x1C, "DIV">;
  def DIVI : MMIXIntRegImmOpcode<0x1D, "DIV">;
}
let Defs = [RR], Uses = [RD] in {
  def DIVU : MMIXIntRegRegOpcode<0x1E, "DIVU">;
  def DIVUI : MMIXIntRegImmOpcode<0x1F, "DIVU">;
}
def ADD : MMIXIntRegRegOpcode<0x20, "ADD">;
def ADDI : MMIXIntRegImmOpcode<0x21, "ADD">;
def ADDU : MMIXIntRegRegOpcode<0x22, "ADDU">;
def ADDUI : MMIXIntRegImmOpcode<0x23, "ADDU">;
def SUB : MMIXIntRegRegOpcode<0x24, "SUB">;
def SUBI : MMIXIntRegImmOpcode<0x25, "SUB">;
def SUBU : MMIXIntRegRegOpcode<0x26, "SUBU">;
def SUBUI : MMIXIntRegImmOpcode<0x27, "SUBU">;
def CMP : MMIXIntRegRegOpcode<0x30, "CMP">;
def CMPI : MMIXIntRegImmOpcode<0x31, "CMP">;
def CMPU : MMIXIntRegRegOpcode<0x32, "CMPU">;
def CMPUI : MMIXIntRegImmOpcode<0x33, "CMPU">;
def NEG : MMIXNegRegOpcode<0x34, "NEG">;
def NEGI : MMIXNegImmOpcode<0x35, "NEG">;
def NEGU : MMIXNegRegOpcode<0x36, "NEGU">;
def NEGUI : MMIXNegImmOpcode<0x37, "NEGU">;
def SL : MMIXIntRegRegOpcode<0x38, "SL">;
def SLI : MMIXIntRegImmOpcode<0x39, "SL">;
def SLU : MMIXIntRegRegOpcode<0x3A, "SLU">;
def SLUI : MMIXIntRegImmOpcode<0x3B, "SLU">;
def SR : MMIXIntRegRegOpcode<0x3C, "SR">;
def SRI : MMIXIntRegImmOpcode<0x3D, "SR">;
def SRU : MMIXIntRegRegOpcode<0x3E, "SRU">;
def SRUI : MMIXIntRegImmOpcode<0x3F, "SRU">;

def TWOADDU : MMIXIntRegRegOpcode<0x28, "2ADDU">;
def TWOADDUI : MMIXIntRegImmOpcode<0x29, "2ADDU">;
def FOURADDU : MMIXIntRegRegOpcode<0x2A, "4ADDU">;
def FOURADDUI : MMIXIntRegImmOpcode<0x2B, "4ADDU">;
def EIGHTADDU : MMIXIntRegRegOpcode<0x2C, "8ADDU">;
def EIGHTADDUI : MMIXIntRegImmOpcode<0x2D, "8ADDU">;
def SIXTEENADDU : MMIXIntRegRegOpcode<0x2E, "16ADDU">;
def SIXTEENADDUI : MMIXIntRegImmOpcode<0x2F, "16ADDU">;

def mmix_uimm8 : PatLeaf<(imm), [{
  return isUInt<8>(N->getZExtValue());
}]>;

def mmix_neg_uimm8 : PatLeaf<(imm), [{
  int64_t Value = N->getSExtValue();
  return Value < 0 && Value >= -255;
}]>;

def mmix_neg_imm : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-N->getSExtValue(), SDLoc(N), MVT::i64);
}]>;

def mmix_uimm6 : PatLeaf<(imm), [{
  return isUInt<6>(N->getZExtValue());
}]>;

def mmix_inv_uimm8 : PatLeaf<(imm), [{
  return isUInt<8>(~N->getZExtValue());
}]>;

def mmix_inv_imm : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(~N->getZExtValue(), SDLoc(N), MVT::i64);
}]>;

// LLVM integer arithmetic wraps modulo 2^64 unless overflow makes the result
// poison. Neither case permits the traps raised by MMIX ADD, SUB, and NEG, so
// instruction selection exclusively uses their unsigned, nontrapping forms.
def : Pat<(add i64:$lhs, mmix_uimm8:$rhs),
          (ADDUI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(add i64:$lhs, mmix_neg_uimm8:$rhs),
          (SUBUI $lhs, (mmix_neg_imm mmix_neg_uimm8:$rhs))>;
def : Pat<(add i64:$lhs, i64:$rhs), (ADDU $lhs, $rhs)>;

def : Pat<(sub i64:$lhs, mmix_uimm8:$rhs),
          (SUBUI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(sub i64:$lhs, mmix_neg_uimm8:$rhs),
          (ADDUI $lhs, (mmix_neg_imm mmix_neg_uimm8:$rhs))>;
def : Pat<(sub 0, i64:$rhs), (NEGU 0, $rhs)>;
def : Pat<(sub i64:$lhs, i64:$rhs), (SUBU $lhs, $rhs)>;

def : Pat<(mul i64:$lhs, mmix_uimm8:$rhs),
          (MULUI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(mul i64:$lhs, i64:$rhs), (MULU $lhs, $rhs)>;

def CSN : MMIXCondSetRegOpcode<0x60, "CSN">;
def CSNI : MMIXCondSetImmOpcode<0x61, "CSN">;
def CSZ : MMIXCondSetRegOpcode<0x62, "CSZ">;
def CSZI : MMIXCondSetImmOpcode<0x63, "CSZ">;
def CSP : MMIXCondSetRegOpcode<0x64, "CSP">;
def CSPI : MMIXCondSetImmOpcode<0x65, "CSP">;
def CSOD : MMIXCondSetRegOpcode<0x66, "CSOD">;
def CSODI : MMIXCondSetImmOpcode<0x67, "CSOD">;
def CSNN : MMIXCondSetRegOpcode<0x68, "CSNN">;
def CSNNI : MMIXCondSetImmOpcode<0x69, "CSNN">;
def CSNZ : MMIXCondSetRegOpcode<0x6A, "CSNZ">;
def CSNZI : MMIXCondSetImmOpcode<0x6B, "CSNZ">;
def CSNP : MMIXCondSetRegOpcode<0x6C, "CSNP">;
def CSNPI : MMIXCondSetImmOpcode<0x6D, "CSNP">;
def CSEV : MMIXCondSetRegOpcode<0x6E, "CSEV">;
def CSEVI : MMIXCondSetImmOpcode<0x6F, "CSEV">;
def ZSN : MMIXIntRegRegOpcode<0x70, "ZSN">;
def ZSNI : MMIXIntRegImmOpcode<0x71, "ZSN">;
def ZSZ : MMIXIntRegRegOpcode<0x72, "ZSZ">;
def ZSZI : MMIXIntRegImmOpcode<0x73, "ZSZ">;
def ZSP : MMIXIntRegRegOpcode<0x74, "ZSP">;
def ZSPI : MMIXIntRegImmOpcode<0x75, "ZSP">;
def ZSOD : MMIXIntRegRegOpcode<0x76, "ZSOD">;
def ZSODI : MMIXIntRegImmOpcode<0x77, "ZSOD">;
def ZSNN : MMIXIntRegRegOpcode<0x78, "ZSNN">;
def ZSNNI : MMIXIntRegImmOpcode<0x79, "ZSNN">;
def ZSNZ : MMIXIntRegRegOpcode<0x7A, "ZSNZ">;
def ZSNZI : MMIXIntRegImmOpcode<0x7B, "ZSNZ">;
def ZSNP : MMIXIntRegRegOpcode<0x7C, "ZSNP">;
def ZSNPI : MMIXIntRegImmOpcode<0x7D, "ZSNP">;
def ZSEV : MMIXIntRegRegOpcode<0x7E, "ZSEV">;
def ZSEVI : MMIXIntRegImmOpcode<0x7F, "ZSEV">;

// CMP and CMPU produce -1, 0, or 1. A zero-set instruction then maps the
// requested relation to LLVM's canonical 0-or-1 Boolean representation.
def : Pat<(setlt i64:$lhs, i64:$rhs), (ZSNI (CMP $lhs, $rhs), 1)>;
def : Pat<(setle i64:$lhs, i64:$rhs), (ZSNPI (CMP $lhs, $rhs), 1)>;
def : Pat<(setgt i64:$lhs, i64:$rhs), (ZSPI (CMP $lhs, $rhs), 1)>;
def : Pat<(setge i64:$lhs, i64:$rhs), (ZSNNI (CMP $lhs, $rhs), 1)>;
def : Pat<(setult i64:$lhs, i64:$rhs), (ZSNI (CMPU $lhs, $rhs), 1)>;
def : Pat<(setule i64:$lhs, i64:$rhs), (ZSNPI (CMPU $lhs, $rhs), 1)>;
def : Pat<(setugt i64:$lhs, i64:$rhs), (ZSPI (CMPU $lhs, $rhs), 1)>;
def : Pat<(setuge i64:$lhs, i64:$rhs), (ZSNNI (CMPU $lhs, $rhs), 1)>;
def : Pat<(seteq i64:$lhs, i64:$rhs), (ZSZI (CMPU $lhs, $rhs), 1)>;
def : Pat<(setne i64:$lhs, i64:$rhs), (ZSNZI (CMPU $lhs, $rhs), 1)>;

def : Pat<(setlt i64:$lhs, mmix_uimm8:$rhs),
          (ZSNI (CMPI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setle i64:$lhs, mmix_uimm8:$rhs),
          (ZSNPI (CMPI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setgt i64:$lhs, mmix_uimm8:$rhs),
          (ZSPI (CMPI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setge i64:$lhs, mmix_uimm8:$rhs),
          (ZSNNI (CMPI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setult i64:$lhs, mmix_uimm8:$rhs),
          (ZSNI (CMPUI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setule i64:$lhs, mmix_uimm8:$rhs),
          (ZSNPI (CMPUI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setugt i64:$lhs, mmix_uimm8:$rhs),
          (ZSPI (CMPUI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setuge i64:$lhs, mmix_uimm8:$rhs),
          (ZSNNI (CMPUI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(seteq i64:$lhs, mmix_uimm8:$rhs),
          (ZSZI (CMPUI $lhs, mmix_uimm8:$rhs), 1)>;
def : Pat<(setne i64:$lhs, mmix_uimm8:$rhs),
          (ZSNZI (CMPUI $lhs, mmix_uimm8:$rhs), 1)>;

// CSNZ selects its value for every true LLVM condition. When the false value
// is encodable instead, CSZ reverses the choice and keeps the true register in
// the tied destination.
def : Pat<(select i64:$cond, i64:$true, i64:$false),
          (CSNZ $false, $cond, $true)>;
def : Pat<(select i64:$cond, mmix_uimm8:$true, i64:$false),
          (CSNZI $false, $cond, mmix_uimm8:$true)>;
def : Pat<(select i64:$cond, i64:$true, mmix_uimm8:$false),
          (CSZI $true, $cond, mmix_uimm8:$false)>;

def OR : MMIXIntRegRegOpcode<0xC0, "OR">;
def ORI : MMIXIntRegImmOpcode<0xC1, "OR">;
def ORN : MMIXIntRegRegOpcode<0xC2, "ORN">;
def ORNI : MMIXIntRegImmOpcode<0xC3, "ORN">;
def NOR : MMIXIntRegRegOpcode<0xC4, "NOR">;
def NORI : MMIXIntRegImmOpcode<0xC5, "NOR">;
def XOR : MMIXIntRegRegOpcode<0xC6, "XOR">;
def XORI : MMIXIntRegImmOpcode<0xC7, "XOR">;
def AND : MMIXIntRegRegOpcode<0xC8, "AND">;
def ANDI : MMIXIntRegImmOpcode<0xC9, "AND">;
def ANDN : MMIXIntRegRegOpcode<0xCA, "ANDN">;
def ANDNI : MMIXIntRegImmOpcode<0xCB, "ANDN">;
def NAND : MMIXIntRegRegOpcode<0xCC, "NAND">;
def NANDI : MMIXIntRegImmOpcode<0xCD, "NAND">;
def NXOR : MMIXIntRegRegOpcode<0xCE, "NXOR">;
def NXORI : MMIXIntRegImmOpcode<0xCF, "NXOR">;

// SLU is the nontrapping left shift required by LLVM's poison-on-overflow
// semantics. MMIX register shifts are exact for every non-poison LLVM shift
// count; immediate forms are restricted to that valid 0..63 range.
def : Pat<(shl i64:$value, mmix_uimm6:$amount),
          (SLUI $value, mmix_uimm6:$amount)>;
def : Pat<(shl i64:$value, i64:$amount), (SLU $value, $amount)>;
def : Pat<(sra i64:$value, mmix_uimm6:$amount),
          (SRI $value, mmix_uimm6:$amount)>;
def : Pat<(sra i64:$value, i64:$amount), (SR $value, $amount)>;
def : Pat<(srl i64:$value, mmix_uimm6:$amount),
          (SRUI $value, mmix_uimm6:$amount)>;
def : Pat<(srl i64:$value, i64:$amount), (SRU $value, $amount)>;

def : Pat<(sext_inreg i64:$value, i8),
          (SRI (SLUI $value, 56), 56)>;
def : Pat<(sext_inreg i64:$value, i16),
          (SRI (SLUI $value, 48), 48)>;
def : Pat<(sext_inreg i64:$value, i32),
          (SRI (SLUI $value, 32), 32)>;

// Select complemented forms only when the complete Boolean identity is
// present. The immediate forms encode the complement in their Z byte.
def : Pat<(xor (or i64:$lhs, i64:$rhs), (i64 -1)),
          (NOR $lhs, $rhs)>;
def : Pat<(xor (or i64:$lhs, mmix_uimm8:$rhs), (i64 -1)),
          (NORI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(xor (and i64:$lhs, i64:$rhs), (i64 -1)),
          (NAND $lhs, $rhs)>;
def : Pat<(xor (and i64:$lhs, mmix_uimm8:$rhs), (i64 -1)),
          (NANDI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(xor (xor i64:$lhs, i64:$rhs), (i64 -1)),
          (NXOR $lhs, $rhs)>;
def : Pat<(xor (xor i64:$lhs, mmix_uimm8:$rhs), (i64 -1)),
          (NXORI $lhs, mmix_uimm8:$rhs)>;

def : Pat<(and i64:$lhs, (xor i64:$rhs, (i64 -1))),
          (ANDN $lhs, $rhs)>;
def : Pat<(and (xor i64:$rhs, (i64 -1)), i64:$lhs),
          (ANDN $lhs, $rhs)>;
def : Pat<(and (xor i64:$lhs, (i64 -1)), mmix_inv_uimm8:$rhs),
          (NORI $lhs, (mmix_inv_imm mmix_inv_uimm8:$rhs))>;
def : Pat<(and i64:$lhs, mmix_uimm8:$rhs),
          (ANDI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(and i64:$lhs, mmix_inv_uimm8:$rhs),
          (ANDNI $lhs, (mmix_inv_imm mmix_inv_uimm8:$rhs))>;
def : Pat<(and i64:$lhs, i64:$rhs), (AND $lhs, $rhs)>;

def : Pat<(or i64:$lhs, (xor i64:$rhs, (i64 -1))),
          (ORN $lhs, $rhs)>;
def : Pat<(or (xor i64:$rhs, (i64 -1)), i64:$lhs),
          (ORN $lhs, $rhs)>;
def : Pat<(or (xor i64:$lhs, (i64 -1)), mmix_inv_uimm8:$rhs),
          (NANDI $lhs, (mmix_inv_imm mmix_inv_uimm8:$rhs))>;
def : Pat<(or i64:$lhs, mmix_uimm8:$rhs),
          (ORI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(or i64:$lhs, mmix_inv_uimm8:$rhs),
          (ORNI $lhs, (mmix_inv_imm mmix_inv_uimm8:$rhs))>;
def : Pat<(or i64:$lhs, i64:$rhs), (OR $lhs, $rhs)>;

def : Pat<(xor i64:$lhs, (xor i64:$rhs, (i64 -1))),
          (NXOR $lhs, $rhs)>;
def : Pat<(xor (xor i64:$rhs, (i64 -1)), i64:$lhs),
          (NXOR $lhs, $rhs)>;
def : Pat<(xor i64:$lhs, mmix_uimm8:$rhs),
          (XORI $lhs, mmix_uimm8:$rhs)>;
def : Pat<(xor i64:$lhs, mmix_inv_uimm8:$rhs),
          (NXORI $lhs, (mmix_inv_imm mmix_inv_uimm8:$rhs))>;
def : Pat<(xor i64:$lhs, i64:$rhs), (XOR $lhs, $rhs)>;

def BDIF : MMIXIntRegRegOpcode<0xD0, "BDIF">;
def BDIFI : MMIXIntRegImmOpcode<0xD1, "BDIF">;
def WDIF : MMIXIntRegRegOpcode<0xD2, "WDIF">;
def WDIFI : MMIXIntRegImmOpcode<0xD3, "WDIF">;
def TDIF : MMIXIntRegRegOpcode<0xD4, "TDIF">;
def TDIFI : MMIXIntRegImmOpcode<0xD5, "TDIF">;
def ODIF : MMIXIntRegRegOpcode<0xD6, "ODIF">;
def ODIFI : MMIXIntRegImmOpcode<0xD7, "ODIF">;
def MUX : MMIXIntRegRegOpcode<0xD8, "MUX">;
def MUXI : MMIXIntRegImmOpcode<0xD9, "MUX">;
def SADD : MMIXIntRegRegOpcode<0xDA, "SADD">;
def SADDI : MMIXIntRegImmOpcode<0xDB, "SADD">;
def MOR : MMIXIntRegRegOpcode<0xDC, "MOR">;
def MORI : MMIXIntRegImmOpcode<0xDD, "MOR">;
def MXOR : MMIXIntRegRegOpcode<0xDE, "MXOR">;
def MXORI : MMIXIntRegImmOpcode<0xDF, "MXOR">;

def SETH : MMIXIntConstOpcode<0xE0, "SETH">;
def SETMH : MMIXIntConstOpcode<0xE1, "SETMH">;
def SETML : MMIXIntConstOpcode<0xE2, "SETML">;
def SETL : MMIXIntConstOpcode<0xE3, "SETL">;
def INCH : MMIXIntConstOpcode<0xE4, "INCH">;
def INCMH : MMIXIntConstOpcode<0xE5, "INCMH">;
def INCML : MMIXIntConstOpcode<0xE6, "INCML">;
def INCL : MMIXIntConstOpcode<0xE7, "INCL">;
def ORH : MMIXIntConstOpcode<0xE8, "ORH">;
def ORMH : MMIXIntConstOpcode<0xE9, "ORMH">;
def ORML : MMIXIntConstOpcode<0xEA, "ORML">;
def ORL : MMIXIntConstOpcode<0xEB, "ORL">;
def ANDNH : MMIXIntConstOpcode<0xEC, "ANDNH">;
def ANDNMH : MMIXIntConstOpcode<0xED, "ANDNMH">;
def ANDNML : MMIXIntConstOpcode<0xEE, "ANDNML">;
def ANDNL : MMIXIntConstOpcode<0xEF, "ANDNL">;

// Integer memory operations use a register base and either a register or an
// unsigned-byte offset. The load and store records keep width and signedness
// variants distinct for the later MC and lowering work.
class MMIXLoadReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayLoad = true;
  let MMIXInventoryFamily = "memory";
}

class MMIXLoadImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                      Mnemonic # " $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayLoad = true;
  let MMIXInventoryFamily = "memory";
}

class MMIXStoreReg<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayStore = true;
  let MMIXInventoryFamily = "memory";
}

class MMIXStoreImm<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base,
                           MMIXMemOffset8:$offset),
                      Mnemonic # " $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayStore = true;
  let MMIXInventoryFamily = "memory";
}

class MMIXLoadRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXLoadReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXLoadImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXLoadImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXStoreRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXStoreReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXStoreImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXStoreImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def LDB : MMIXLoadRegOpcode<0x80, "LDB">;
def LDBI : MMIXLoadImmOpcode<0x81, "LDB">;
def LDBU : MMIXLoadRegOpcode<0x82, "LDBU">;
def LDBUI : MMIXLoadImmOpcode<0x83, "LDBU">;
def LDW : MMIXLoadRegOpcode<0x84, "LDW">;
def LDWI : MMIXLoadImmOpcode<0x85, "LDW">;
def LDWU : MMIXLoadRegOpcode<0x86, "LDWU">;
def LDWUI : MMIXLoadImmOpcode<0x87, "LDWU">;
def LDT : MMIXLoadRegOpcode<0x88, "LDT">;
def LDTI : MMIXLoadImmOpcode<0x89, "LDT">;
def LDTU : MMIXLoadRegOpcode<0x8A, "LDTU">;
def LDTUI : MMIXLoadImmOpcode<0x8B, "LDTU">;
def LDO : MMIXLoadRegOpcode<0x8C, "LDO">;
def LDOI : MMIXLoadImmOpcode<0x8D, "LDO">;
def LDOU : MMIXLoadRegOpcode<0x8E, "LDOU">;
def LDOUI : MMIXLoadImmOpcode<0x8F, "LDOU">;
def LDHT : MMIXLoadRegOpcode<0x92, "LDHT">;
def LDHTI : MMIXLoadImmOpcode<0x93, "LDHT">;

def STB : MMIXStoreRegOpcode<0xA0, "STB">;
def STBI : MMIXStoreImmOpcode<0xA1, "STB">;
def STBU : MMIXStoreRegOpcode<0xA2, "STBU">;
def STBUI : MMIXStoreImmOpcode<0xA3, "STBU">;
def STW : MMIXStoreRegOpcode<0xA4, "STW">;
def STWI : MMIXStoreImmOpcode<0xA5, "STW">;
def STWU : MMIXStoreRegOpcode<0xA6, "STWU">;
def STWUI : MMIXStoreImmOpcode<0xA7, "STWU">;
def STT : MMIXStoreRegOpcode<0xA8, "STT">;
def STTI : MMIXStoreImmOpcode<0xA9, "STT">;
def STTU : MMIXStoreRegOpcode<0xAA, "STTU">;
def STTUI : MMIXStoreImmOpcode<0xAB, "STTU">;
def STO : MMIXStoreRegOpcode<0xAC, "STO">;
def STOI : MMIXStoreImmOpcode<0xAD, "STO">;
def STOU : MMIXStoreRegOpcode<0xAE, "STOU">;
def STOUI : MMIXStoreImmOpcode<0xAF, "STOU">;
def STHT : MMIXStoreRegOpcode<0xB2, "STHT">;
def STHTI : MMIXStoreImmOpcode<0xB3, "STHT">;

// CSWAP uses the ordinary three-field memory form. Its comparison and result
// dependency is left for later lowering.
class MMIXCompareSwapReg
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      "CSWAP $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
  let MMIXInventoryFamily = "memory";
}

class MMIXCompareSwapImm
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base,
                           MMIXMemOffset8:$offset),
                           "CSWAP $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
  let MMIXInventoryFamily = "memory";
}

class MMIXCompareSwapRegOpcode<bits<8> Opcode>
    : MMIXCompareSwapReg {
  let MMIXOpcode = Opcode;
}

class MMIXCompareSwapImmOpcode<bits<8> Opcode>
    : MMIXCompareSwapImm {
  let MMIXOpcode = Opcode;
}

def CSWAP : MMIXCompareSwapRegOpcode<0x94>;
def CSWAPI : MMIXCompareSwapImmOpcode<0x95>;

class MMIXCondBranch<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$condition, MMIXBranchTarget:$target),
                      Mnemonic # " $condition, $target"> {
  bits<8> condition;
  bits<16> target;
  let MMIXX = condition;
  let MMIXY = target{15-8};
  let MMIXZ = target{7-0};
  let MMIXPCRelativeWidth = 16;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXCondBranchOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXCondBranch<Mnemonic> {
  let MMIXOpcode = Opcode;
  let MMIXPCRelativeBackward = Opcode{0};
}

def BN   : MMIXCondBranchOpcode<0x40, "BN">;
def BNB  : MMIXCondBranchOpcode<0x41, "BNB">;
def BZ   : MMIXCondBranchOpcode<0x42, "BZ">;
def BZB  : MMIXCondBranchOpcode<0x43, "BZB">;
def BP   : MMIXCondBranchOpcode<0x44, "BP">;
def BPB  : MMIXCondBranchOpcode<0x45, "BPB">;
def BOD  : MMIXCondBranchOpcode<0x46, "BOD">;
def BODB : MMIXCondBranchOpcode<0x47, "BODB">;
def BNN  : MMIXCondBranchOpcode<0x48, "BNN">;
def BNNB : MMIXCondBranchOpcode<0x49, "BNNB">;
def BNZ  : MMIXCondBranchOpcode<0x4A, "BNZ">;
def BNZB : MMIXCondBranchOpcode<0x4B, "BNZB">;
def BNP  : MMIXCondBranchOpcode<0x4C, "BNP">;
def BNPB : MMIXCondBranchOpcode<0x4D, "BNPB">;
def BEV  : MMIXCondBranchOpcode<0x4E, "BEV">;
def BEVB : MMIXCondBranchOpcode<0x4F, "BEVB">;

def PBN   : MMIXCondBranchOpcode<0x50, "PBN">;
def PBNB  : MMIXCondBranchOpcode<0x51, "PBNB">;
def PBZ   : MMIXCondBranchOpcode<0x52, "PBZ">;
def PBZB  : MMIXCondBranchOpcode<0x53, "PBZB">;
def PBP   : MMIXCondBranchOpcode<0x54, "PBP">;
def PBPB  : MMIXCondBranchOpcode<0x55, "PBPB">;
def PBOD  : MMIXCondBranchOpcode<0x56, "PBOD">;
def PBODB : MMIXCondBranchOpcode<0x57, "PBODB">;
def PBNN  : MMIXCondBranchOpcode<0x58, "PBNN">;
def PBNNB : MMIXCondBranchOpcode<0x59, "PBNNB">;
def PBNZ  : MMIXCondBranchOpcode<0x5A, "PBNZ">;
def PBNZB : MMIXCondBranchOpcode<0x5B, "PBNZB">;
def PBNP  : MMIXCondBranchOpcode<0x5C, "PBNP">;
def PBNPB : MMIXCondBranchOpcode<0x5D, "PBNPB">;
def PBEV  : MMIXCondBranchOpcode<0x5E, "PBEV">;
def PBEVB : MMIXCondBranchOpcode<0x5F, "PBEVB">;

class MMIXJump<string Mnemonic>
    : MMIXInstruction<(outs), (ins MMIXJumpTarget:$target),
                      Mnemonic # " $target"> {
  bits<24> target;
  let MMIXX = target{23-16};
  let MMIXY = target{15-8};
  let MMIXZ = target{7-0};
  let MMIXPCRelativeWidth = 24;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXJumpOpcode<bits<8> Opcode, string Mnemonic> : MMIXJump<Mnemonic> {
  let MMIXOpcode = Opcode;
  let MMIXPCRelativeBackward = Opcode{0};
}

def JMP  : MMIXJumpOpcode<0xF0, "JMP">;
def JMPB : MMIXJumpOpcode<0xF1, "JMPB">;

class MMIXGetAddress<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst), (ins MMIXBranchTarget:$target),
                      Mnemonic # " $dst, $target"> {
  bits<8> dst;
  bits<16> target;
  let MMIXX = dst;
  let MMIXY = target{15-8};
  let MMIXZ = target{7-0};
  let MMIXPCRelativeWidth = 16;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXGetAddressOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXGetAddress<Mnemonic> {
  let MMIXOpcode = Opcode;
  let MMIXPCRelativeBackward = Opcode{0};
}

def GETA  : MMIXGetAddressOpcode<0xF4, "GETA">;
def GETAB : MMIXGetAddressOpcode<0xF5, "GETAB">;

class MMIXPushJump<string Mnemonic>
    : MMIXInstruction<(outs), (ins GPR64Op:$link, MMIXBranchTarget:$target),
                      Mnemonic # " $link, $target"> {
  bits<8> link;
  bits<16> target;
  let MMIXX = link;
  let MMIXY = target{15-8};
  let MMIXZ = target{7-0};
  let MMIXPCRelativeWidth = 16;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXPushJumpOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXPushJump<Mnemonic> {
  let MMIXOpcode = Opcode;
  let MMIXPCRelativeBackward = Opcode{0};
}

def PUSHJ  : MMIXPushJumpOpcode<0xF2, "PUSHJ">;
def PUSHJB : MMIXPushJumpOpcode<0xF3, "PUSHJB">;

class MMIXGoReg
    : MMIXInstruction<(outs GPR64Op:$link),
                      (ins GPR64Op:$base, GPR64Op:$offset),
                      "GO $link, $base, $offset"> {
  bits<8> link;
  bits<8> base;
  bits<8> offset;
  let MMIXX = link;
  let MMIXY = base;
  let MMIXZ = offset;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXGoImm
    : MMIXInstruction<(outs GPR64Op:$link),
                      (ins GPR64Op:$base, MMIXUImm8:$offset),
                      "GO $link, $base, $offset"> {
  bits<8> link;
  bits<8> base;
  bits<8> offset;
  let MMIXX = link;
  let MMIXY = base;
  let MMIXZ = offset;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXGoRegOpcode<bits<8> Opcode> : MMIXGoReg {
  let MMIXOpcode = Opcode;
}

class MMIXGoImmOpcode<bits<8> Opcode> : MMIXGoImm {
  let MMIXOpcode = Opcode;
}

def GO  : MMIXGoRegOpcode<0x9E>;
def GOI : MMIXGoImmOpcode<0x9F>;

class MMIXPushGoReg
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$count, GPR64Op:$base, GPR64Op:$offset),
                      "PUSHGO $count, $base, $offset"> {
  bits<8> count;
  bits<8> base;
  bits<8> offset;
  let MMIXX = count;
  let MMIXY = base;
  let MMIXZ = offset;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXPushGoImm
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$count, GPR64Op:$base, MMIXUImm8:$offset),
                      "PUSHGO $count, $base, $offset"> {
  bits<8> count;
  bits<8> base;
  bits<8> offset;
  let MMIXX = count;
  let MMIXY = base;
  let MMIXZ = offset;
  let MMIXInventoryFamily = "control-flow";
}

class MMIXPushGoRegOpcode<bits<8> Opcode> : MMIXPushGoReg {
  let MMIXOpcode = Opcode;
}

class MMIXPushGoImmOpcode<bits<8> Opcode> : MMIXPushGoImm {
  let MMIXOpcode = Opcode;
}

def PUSHGO  : MMIXPushGoRegOpcode<0xBE>;
def PUSHGOI : MMIXPushGoImmOpcode<0xBF>;

class MMIXPop : MMIXInstruction<(outs),
                                (ins MMIXUImm8:$count, MMIXUImm16:$target),
                                "POP $count, $target"> {
  bits<8> count;
  bits<16> target;
  let MMIXX = count;
  let MMIXY = target{15-8};
  let MMIXZ = target{7-0};
  let MMIXInventoryFamily = "control-flow";
}

def POP : MMIXPop {
  let MMIXOpcode = 0xF8;
  let isReturn = true;
  let isTerminator = true;
  let isBarrier = true;
}

// The provisional ABI always returns global results with POP 0,0. Keep these
// fixed CodeGen forms separate from the general architectural POP instruction.
class MMIXReturn<list<Register> ResultUses>
    : MMIXInstruction<(outs), (ins), "POP 0, 0"> {
  let MMIXOpcode = 0xF8;
  let MMIXX = 0;
  let MMIXY = 0;
  let MMIXZ = 0;
  let isCodeGenOnly = true;
  let isReturn = true;
  let isTerminator = true;
  let isBarrier = true;
  let Uses = !listconcat([RJ, RL, RO, RG], ResultUses);
}

def RET : MMIXReturn<[]>;
def RET_VALUE : MMIXReturn<[R231]>;

// Floating-point arithmetic operates on the 64-bit floating-point view of
// the general-purpose register file. These operations have no immediate
// variants in the MMIX instruction set.
class MMIXFPBinary<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins FPR64Op:$lhs, FPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  bits<8> dst;
  bits<8> lhs;
  bits<8> rhs;
  let MMIXX = dst;
  let MMIXY = lhs;
  let MMIXZ = rhs;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPBinaryOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPBinary<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def FADD : MMIXFPBinaryOpcode<0x04, "FADD">;
def FSUB : MMIXFPBinaryOpcode<0x06, "FSUB">;
def FMUL : MMIXFPBinaryOpcode<0x10, "FMUL">;
def FDIV : MMIXFPBinaryOpcode<0x14, "FDIV">;
def FREM : MMIXFPBinaryOpcode<0x16, "FREM">;

class MMIXFPCompare<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins FPR64Op:$lhs, FPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  bits<8> dst;
  bits<8> lhs;
  bits<8> rhs;
  let MMIXX = dst;
  let MMIXY = lhs;
  let MMIXZ = rhs;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPCompareOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPCompare<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def FCMP  : MMIXFPCompareOpcode<0x01, "FCMP">;
def FCMPE : MMIXFPCompareOpcode<0x11, "FCMPE">;
def FUN   : MMIXFPCompareOpcode<0x02, "FUN">;
def FUNE  : MMIXFPCompareOpcode<0x12, "FUNE">;
def FEQL  : MMIXFPCompareOpcode<0x03, "FEQL">;
def FEQLE : MMIXFPCompareOpcode<0x13, "FEQLE">;

class MMIXFPUnaryRounded<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins MMIXRoundingMode:$round, FPR64Op:$src),
                      Mnemonic # " $dst, $round, $src"> {
  bits<8> dst;
  bits<3> round;
  bits<8> src;
  let MMIXX = dst;
  let MMIXY{7-3} = 0;
  let MMIXY{2-0} = round;
  let MMIXZ = src;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPUnaryRoundedOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPUnaryRounded<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def FSQRT : MMIXFPUnaryRoundedOpcode<0x15, "FSQRT">;
def FINT  : MMIXFPUnaryRoundedOpcode<0x17, "FINT">;

class MMIXFix<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXRoundingMode:$round, FPR64Op:$src),
                      Mnemonic # " $dst, $round, $src"> {
  bits<8> dst;
  bits<3> round;
  bits<8> src;
  let MMIXX = dst;
  let MMIXY{7-3} = 0;
  let MMIXY{2-0} = round;
  let MMIXZ = src;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFixOpcode<bits<8> Opcode, string Mnemonic> : MMIXFix<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def FIX  : MMIXFixOpcode<0x05, "FIX">;
def FIXU : MMIXFixOpcode<0x07, "FIXU">;

class MMIXFloatFromIntReg<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins MMIXRoundingMode:$round, GPR64Op:$src),
                      Mnemonic # " $dst, $round, $src"> {
  bits<8> dst;
  bits<3> round;
  bits<8> src;
  let MMIXX = dst;
  let MMIXY{7-3} = 0;
  let MMIXY{2-0} = round;
  let MMIXZ = src;
  let MMIXInventoryFamily = "floating-point";
  string MMIXSourceForm = "register";
}

class MMIXFloatFromIntImm<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins MMIXRoundingMode:$round, MMIXUImm8:$src),
                      Mnemonic # " $dst, $round, $src"> {
  bits<8> dst;
  bits<3> round;
  bits<8> src;
  let MMIXX = dst;
  let MMIXY{7-3} = 0;
  let MMIXY{2-0} = round;
  let MMIXZ = src;
  let MMIXInventoryFamily = "floating-point";
  string MMIXSourceForm = "immediate";
}

class MMIXFloatFromIntRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFloatFromIntReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXFloatFromIntImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFloatFromIntImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def FLOT   : MMIXFloatFromIntRegOpcode<0x08, "FLOT">;
def FLOTI  : MMIXFloatFromIntImmOpcode<0x09, "FLOT">;
def FLOTU  : MMIXFloatFromIntRegOpcode<0x0A, "FLOTU">;
def FLOTUI : MMIXFloatFromIntImmOpcode<0x0B, "FLOTU">;
def SFLOT  : MMIXFloatFromIntRegOpcode<0x0C, "SFLOT">;
def SFLOTI : MMIXFloatFromIntImmOpcode<0x0D, "SFLOT">;
def SFLOTU : MMIXFloatFromIntRegOpcode<0x0E, "SFLOTU">;
def SFLOTUI : MMIXFloatFromIntImmOpcode<0x0F, "SFLOTU">;

// Short floating-point values are converted to and from the 64-bit floating
// representation when crossing memory.
class MMIXFPLoadReg<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayLoad = true;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPLoadImm<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                      Mnemonic # " $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayLoad = true;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPStoreReg<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins FPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayStore = true;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPStoreImm<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins FPR64Op:$value, GPR64Op:$base,
                           MMIXMemOffset8:$offset),
                      Mnemonic # " $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
  let mayStore = true;
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPLoadRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPLoadReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXFPLoadImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPLoadImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXFPStoreRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPStoreReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXFPStoreImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPStoreImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def LDSF : MMIXFPLoadRegOpcode<0x90, "LDSF">;
def LDSFI : MMIXFPLoadImmOpcode<0x91, "LDSF">;
def STSF : MMIXFPStoreRegOpcode<0xB0, "STSF">;
def STSFI : MMIXFPStoreImmOpcode<0xB1, "STSF">;

// Machine-control instructions are intentionally modeled as side-effecting
// operations. They are available to MC and target-specific mechanisms, but
// are not generic instruction-selection candidates in this phase.
class MMIXMachineInstruction<dag outs, dag ins, string asmstr>
    : MMIXInstruction<outs, ins, asmstr> {
  let hasSideEffects = true;
  let MMIXInventoryFamily = "system";
}

class MMIXStoreConstantReg
    : MMIXMachineInstruction<(outs),
                             (ins MMIXUImm8:$value, GPR64Op:$base,
                                  GPR64Op:$offset),
                             "STCO $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXStoreConstantImm
    : MMIXMachineInstruction<(outs),
                             (ins MMIXUImm8:$value, GPR64Op:$base,
                                  MMIXMemOffset8:$offset),
                             "STCO $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXStoreConstantRegOpcode<bits<8> Opcode>
    : MMIXStoreConstantReg {
  let MMIXOpcode = Opcode;
}

class MMIXStoreConstantImmOpcode<bits<8> Opcode>
    : MMIXStoreConstantImm {
  let MMIXOpcode = Opcode;
}

def STCO : MMIXStoreConstantRegOpcode<0xB4>;
def STCOI : MMIXStoreConstantImmOpcode<0xB5>;

class MMIXCacheReg<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins MMIXUImm8:$count, GPR64Op:$base,
                                  GPR64Op:$offset),
                             Mnemonic # " $count, $base, $offset"> {
  bits<8> count;
  bits<8> base;
  bits<8> offset;
  let MMIXX = count;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXCacheImm<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins MMIXUImm8:$count, GPR64Op:$base,
                                  MMIXMemOffset8:$offset),
                             Mnemonic # " $count, $base, $offset"> {
  bits<8> count;
  bits<8> base;
  bits<8> offset;
  let MMIXX = count;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXCacheRegOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXCacheReg<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXCacheImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXCacheImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def PRELD   : MMIXCacheRegOpcode<0x9A, "PRELD">;
def PRELDI  : MMIXCacheImmOpcode<0x9B, "PRELD">;
def PREGO   : MMIXCacheRegOpcode<0x9C, "PREGO">;
def PREGOI  : MMIXCacheImmOpcode<0x9D, "PREGO">;
def PREST   : MMIXCacheRegOpcode<0xBA, "PREST">;
def PRESTI  : MMIXCacheImmOpcode<0xBB, "PREST">;
def SYNCD   : MMIXCacheRegOpcode<0xB8, "SYNCD">;
def SYNCDI  : MMIXCacheImmOpcode<0xB9, "SYNCD">;
def SYNCID  : MMIXCacheRegOpcode<0xBC, "SYNCID">;
def SYNCIDI : MMIXCacheImmOpcode<0xBD, "SYNCID">;

class MMIXUncachedLoad<string Mnemonic>
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, GPR64Op:$offset),
                             Mnemonic # " $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXUncachedLoadImm<string Mnemonic>
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                             Mnemonic # " $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXUncachedStore<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins GPR64Op:$value, GPR64Op:$base,
                                  GPR64Op:$offset),
                             Mnemonic # " $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXUncachedStoreImm<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins GPR64Op:$value, GPR64Op:$base,
                                  MMIXMemOffset8:$offset),
                             Mnemonic # " $value, $base, $offset"> {
  bits<8> value;
  bits<8> base;
  bits<8> offset;
  let MMIXX = value;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXUncachedLoadOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXUncachedLoad<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXUncachedLoadImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXUncachedLoadImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXUncachedStoreOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXUncachedStore<Mnemonic> {
  let MMIXOpcode = Opcode;
}

class MMIXUncachedStoreImmOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXUncachedStoreImm<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def LDUNC : MMIXUncachedLoadOpcode<0x96, "LDUNC">;
def LDUNCI : MMIXUncachedLoadImmOpcode<0x97, "LDUNC">;
def STUNC : MMIXUncachedStoreOpcode<0xB6, "STUNC">;
def STUNCI : MMIXUncachedStoreImmOpcode<0xB7, "STUNC">;

class MMIXVirtualTranslationReg
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, GPR64Op:$offset),
                             "LDVTS $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXVirtualTranslationImm
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                             "LDVTS $dst, $base, $offset"> {
  bits<8> dst;
  bits<8> base;
  bits<8> offset;
  let MMIXX = dst;
  let MMIXY = base;
  let MMIXZ = offset;
}

class MMIXVirtualTranslationRegOpcode<bits<8> Opcode>
    : MMIXVirtualTranslationReg {
  let MMIXOpcode = Opcode;
}

class MMIXVirtualTranslationImmOpcode<bits<8> Opcode>
    : MMIXVirtualTranslationImm {
  let MMIXOpcode = Opcode;
}

def LDVTS  : MMIXVirtualTranslationRegOpcode<0x98>;
def LDVTSI : MMIXVirtualTranslationImmOpcode<0x99>;

class MMIXSync : MMIXMachineInstruction<(outs), (ins MMIXSyncMode:$mode),
                                        "SYNC $mode"> {
  bits<3> mode;
  let MMIXX = 0;
  let MMIXY = 0;
  let MMIXZ{7-3} = 0;
  let MMIXZ{2-0} = mode;
}

def SYNC : MMIXSync {
  let MMIXOpcode = 0xFC;
}

class MMIXSwym : MMIXMachineInstruction<
    (outs),
    (ins MMIXRegOrImm8:$x, MMIXRegOrImm8:$y, MMIXRegOrImm8:$z),
    "SWYM $x, $y, $z"> {
  bits<8> x;
  bits<8> y;
  bits<8> z;
  let MMIXX = x;
  let MMIXY = y;
  let MMIXZ = z;
}

def SWYM : MMIXSwym {
  let MMIXOpcode = 0xFD;
}

class MMIXTrapInstruction<string Mnemonic>
    : MMIXMachineInstruction<
          (outs),
          (ins MMIXRegOrImm8:$x, MMIXRegOrImm8:$y, MMIXRegOrImm8:$z),
          Mnemonic # " $x, $y, $z"> {
  bits<8> x;
  bits<8> y;
  bits<8> z;
  let MMIXX = x;
  let MMIXY = y;
  let MMIXZ = z;
}

class MMIXTrapInstructionOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXTrapInstruction<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def TRAP : MMIXTrapInstructionOpcode<0x00, "TRAP">;
def TRIP : MMIXTrapInstructionOpcode<0xFF, "TRIP">;

class MMIXResume : MMIXMachineInstruction<(outs), (ins MMIXResumeMode:$mode),
                                           "RESUME $mode"> {
  bits<1> mode;
  let MMIXX = 0;
  let MMIXY = 0;
  let MMIXZ{7-1} = 0;
  let MMIXZ{0} = mode;
}

def RESUME : MMIXResume {
  let MMIXOpcode = 0xF9;
}

class MMIXGet : MMIXMachineInstruction<(outs GPR64Op:$dst), (ins SPR64Op:$src),
                                        "GET $dst, $src"> {
  bits<8> dst;
  bits<5> src;
  let MMIXX = dst;
  let MMIXY = 0;
  let MMIXZ{7-5} = 0;
  let MMIXZ{4-0} = src;
}

def GET : MMIXGet {
  let MMIXOpcode = 0xFE;
}

class MMIXPutReg
    : MMIXMachineInstruction<(outs SPR64Op:$dst), (ins GPR64Op:$src),
                             "PUT $dst, $src"> {
  bits<5> dst;
  bits<8> src;
  let MMIXX{7-5} = 0;
  let MMIXX{4-0} = dst;
  let MMIXY = 0;
  let MMIXZ = src;
}

class MMIXPutImm
    : MMIXMachineInstruction<(outs SPR64Op:$dst), (ins MMIXUImm8:$src),
                             "PUT $dst, $src"> {
  bits<5> dst;
  bits<8> src;
  let MMIXX{7-5} = 0;
  let MMIXX{4-0} = dst;
  let MMIXY = 0;
  let MMIXZ = src;
}

class MMIXPutRegOpcode<bits<8> Opcode> : MMIXPutReg {
  let MMIXOpcode = Opcode;
}

class MMIXPutImmOpcode<bits<8> Opcode> : MMIXPutImm {
  let MMIXOpcode = Opcode;
}

def PUT  : MMIXPutRegOpcode<0xF6>;
def PUTI : MMIXPutImmOpcode<0xF7>;

class MMIXSave : MMIXMachineInstruction<(outs GPR64Op:$state), (ins),
                                         "SAVE $state"> {
  bits<8> state;
  let MMIXX = state;
  let MMIXY = 0;
  let MMIXZ = 0;
}
class MMIXUnsave : MMIXMachineInstruction<(outs), (ins GPR64Op:$state),
                                           "UNSAVE $state"> {
  bits<8> state;
  let MMIXX = 0;
  let MMIXY = 0;
  let MMIXZ = state;
}

def SAVE : MMIXSave {
  let MMIXOpcode = 0xFA;
}
def UNSAVE : MMIXUnsave {
  let MMIXOpcode = 0xFB;
}

// Keep CodeGen-only pseudos separate from the architectural instruction
// inventory and its encoding fields.
class MMIXPseudoInstruction<dag outs, dag ins, string asmstr>
    : Instruction {
  let Namespace = "MMIX";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Size = 0;
  let isPseudo = true;
  let isCodeGenOnly = true;
}

def LOAD_IMM64
    : MMIXPseudoInstruction<(outs GPR64CodeGen:$dst),
                            (ins i64imm:$value), ""> {
  let Pattern = [(set i64:$dst, imm:$value)];
}

def SET_RD_ZERO : MMIXPseudoInstruction<(outs), (ins), ""> {
  let Defs = [RD];
}

let Defs = [R254], Uses = [R254], hasSideEffects = true in {
def ADJCALLSTACKDOWN
    : MMIXPseudoInstruction<(outs),
                            (ins i64imm:$amount, i64imm:$alignment),
                            "# ADJCALLSTACKDOWN $amount, $alignment"> {
  let Pattern = [(MMIXcallseq_start timm:$amount, timm:$alignment)];
}
def ADJCALLSTACKUP
    : MMIXPseudoInstruction<(outs),
                            (ins i64imm:$amount, i64imm:$alignment),
                            "# ADJCALLSTACKUP $amount, $alignment"> {
  let Pattern = [(MMIXcallseq_end timm:$amount, timm:$alignment)];
}
}

// These records describe expansion relationships without installing parser or
// printer aliases.  The canonical instruction records remain independently
// available for later encoding and lowering work.
class MMIXExpansionRelation<string Name, list<Instruction> Expansion> {
  string MMIXExpansionName = Name;
  list<Instruction> MMIXExpansionInstructions = Expansion;
}

class MMIXPseudoRelation<string PseudoName, list<Instruction> Expansion>
    : MMIXExpansionRelation<PseudoName, Expansion> {
  bit MMIXIsPseudo = true;
}

class MMIXAlias<string AliasName, list<Instruction> CanonicalInstructions>
    : MMIXExpansionRelation<AliasName, CanonicalInstructions> {
  bit MMIXIsAlias = true;
}

def MMIXSetAlias : MMIXAlias<"SET", [ORI]>;
def MMIXLDAAlias : MMIXAlias<"LDA", [ADDU, ADDUI]>;
