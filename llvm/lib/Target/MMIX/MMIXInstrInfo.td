//===-- MMIXInstrInfo.td - MMIX Instruction Definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

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

def MMIXImm64 : MMIXImmediateOperand<64, true, "signed-64">;
def MMIXUImm8 : MMIXImmediateOperand<8, false, "unsigned-8">;
def MMIXUImm16 : MMIXImmediateOperand<16, false, "unsigned-16">;
def MMIXUImm24 : MMIXImmediateOperand<24, false, "unsigned-24">;
def MMIXRoundingMode : MMIXImmediateOperand<2, false, "rounding-mode">;

class MMIXRegOrImmOperand<int Width, string Range>
    : Operand<i64> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = false;
  string MMIXEncodingKind = "register-or-immediate";
  string MMIXEncodingRange = Range;
}

def MMIXRegOrImm8 : MMIXRegOrImmOperand<8, "unsigned-8">;

class MMIXPCRelativeOperand<int Width, string Range> : Operand<OtherVT> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = true;
  string MMIXEncodingKind = "pc-relative";
  string MMIXEncodingRange = Range;
  string MMIXFixupKind = "pc-relative";
  int MMIXFixupScale = 4;
  let OperandType = "OPERAND_PCREL";
}

def MMIXBranchTarget : MMIXPCRelativeOperand<8, "signed-8-instruction-words">;

class MMIXMemoryOffsetOperand<int Width, bit Signed, string Range>
    : Operand<i64> {
  int MMIXEncodingWidth = Width;
  bit MMIXEncodingSigned = Signed;
  string MMIXEncodingKind = "memory-offset";
  string MMIXEncodingRange = Range;
}

def MMIXMemOffset8 : MMIXMemoryOffsetOperand<8, false, "unsigned-8">;

def MMIXMemAddr : Operand<i64> {
  int MMIXEncodingWidth = 8;
  bit MMIXEncodingSigned = false;
  string MMIXEncodingKind = "memory-address";
  string MMIXEncodingRange = "unsigned-8-offset";
  let OperandType = "OPERAND_MEMORY";
}

// MMIX instructions are fixed-width 32-bit words stored in big-endian byte
// order. In TableGen bit numbering, the first byte is Inst{31-24}, followed by
// the X, Y, and Z bytes. The default zero opcode is intentional until a
// concrete instruction family assigns its MMIXware opcode.
class MMIXInstruction<dag outs, dag ins, string asmstr> : Instruction {
  let Namespace = "MMIX";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Size = 4;
  bits<32> Inst;
  bits<8> MMIXOpcode = 0;
  bits<8> MMIXX = 0;
  bits<8> MMIXY = 0;
  bits<8> MMIXZ = 0;
  let Inst{31-24} = MMIXOpcode;
  let Inst{23-16} = MMIXX;
  let Inst{15-8} = MMIXY;
  let Inst{7-0} = MMIXZ;
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
  let MMIXInventoryFamily = "integer";
}

class MMIXIntRegImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$lhs, MMIXUImm8:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  let MMIXInventoryFamily = "integer";
}

// NEG and NEGU use an unsigned byte in the Y field even in their register-Z
// form, unlike the ordinary X,Y,Z instruction family.
class MMIXNegReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm8:$lhs, GPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  let MMIXInventoryFamily = "integer";
}

class MMIXNegImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm8:$lhs, MMIXUImm8:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
  let MMIXInventoryFamily = "integer";
}

class MMIXIntConst<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm16:$value),
                      Mnemonic # " $dst, $value"> {
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
def MULU : MMIXIntRegRegOpcode<0x1A, "MULU">;
def MULUI : MMIXIntRegImmOpcode<0x1B, "MULU">;
def DIV : MMIXIntRegRegOpcode<0x1C, "DIV">;
def DIVI : MMIXIntRegImmOpcode<0x1D, "DIV">;
def DIVU : MMIXIntRegRegOpcode<0x1E, "DIVU">;
def DIVUI : MMIXIntRegImmOpcode<0x1F, "DIVU">;
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

def CSN : MMIXIntRegRegOpcode<0x60, "CSN">;
def CSNI : MMIXIntRegImmOpcode<0x61, "CSN">;
def CSZ : MMIXIntRegRegOpcode<0x62, "CSZ">;
def CSZI : MMIXIntRegImmOpcode<0x63, "CSZ">;
def CSP : MMIXIntRegRegOpcode<0x64, "CSP">;
def CSPI : MMIXIntRegImmOpcode<0x65, "CSP">;
def CSOD : MMIXIntRegRegOpcode<0x66, "CSOD">;
def CSODI : MMIXIntRegImmOpcode<0x67, "CSOD">;
def CSNN : MMIXIntRegRegOpcode<0x68, "CSNN">;
def CSNNI : MMIXIntRegImmOpcode<0x69, "CSNN">;
def CSNZ : MMIXIntRegRegOpcode<0x6A, "CSNZ">;
def CSNZI : MMIXIntRegImmOpcode<0x6B, "CSNZ">;
def CSNP : MMIXIntRegRegOpcode<0x6C, "CSNP">;
def CSNPI : MMIXIntRegImmOpcode<0x6D, "CSNP">;
def CSEV : MMIXIntRegRegOpcode<0x6E, "CSEV">;
def CSEVI : MMIXIntRegImmOpcode<0x6F, "CSEV">;
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
  let MMIXInventoryFamily = "memory";
}

class MMIXLoadImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                      Mnemonic # " $dst, $base, $offset"> {
  let MMIXInventoryFamily = "memory";
}

class MMIXStoreReg<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $value, $base, $offset"> {
  let MMIXInventoryFamily = "memory";
}

class MMIXStoreImm<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base,
                           MMIXMemOffset8:$offset),
                      Mnemonic # " $value, $base, $offset"> {
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

// CSWAP has the ordinary memory address forms but also updates its X operand
// with the comparison result. The data dependency is left for later lowering.
class MMIXCompareSwapReg
    : MMIXInstruction<(outs GPR64Op:$result),
                      (ins GPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      "CSWAP $result, $base, $offset"> {
  let MMIXInventoryFamily = "memory";
}

class MMIXCompareSwapImm
    : MMIXInstruction<(outs GPR64Op:$result),
                      (ins GPR64Op:$value, GPR64Op:$base,
                           MMIXMemOffset8:$offset),
                      "CSWAP $result, $base, $offset"> {
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
  let MMIXInventoryFamily = "control-flow";
}

class MMIXCondBranchOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXCondBranch<Mnemonic> {
  let MMIXOpcode = Opcode;
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
    : MMIXInstruction<(outs), (ins MMIXBranchTarget:$target),
                      Mnemonic # " $target"> {
  let MMIXInventoryFamily = "control-flow";
}

class MMIXJumpOpcode<bits<8> Opcode, string Mnemonic> : MMIXJump<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def JMP  : MMIXJumpOpcode<0xF0, "JMP">;
def JMPB : MMIXJumpOpcode<0xF1, "JMPB">;

class MMIXGetAddress<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst), (ins MMIXBranchTarget:$target),
                      Mnemonic # " $dst, $target"> {
  let MMIXInventoryFamily = "control-flow";
}

class MMIXGetAddressOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXGetAddress<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def GETA  : MMIXGetAddressOpcode<0xF4, "GETA">;
def GETAB : MMIXGetAddressOpcode<0xF5, "GETAB">;

class MMIXPushJump<string Mnemonic>
    : MMIXInstruction<(outs), (ins GPR64Op:$link, MMIXBranchTarget:$target),
                      Mnemonic # " $link, $target"> {
  let MMIXInventoryFamily = "control-flow";
}

class MMIXPushJumpOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXPushJump<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def PUSHJ  : MMIXPushJumpOpcode<0xF2, "PUSHJ">;
def PUSHJB : MMIXPushJumpOpcode<0xF3, "PUSHJB">;

class MMIXGoReg
    : MMIXInstruction<(outs GPR64Op:$link),
                      (ins GPR64Op:$base, GPR64Op:$offset),
                      "GO $link, $base, $offset"> {
  let MMIXInventoryFamily = "control-flow";
}

class MMIXGoImm
    : MMIXInstruction<(outs GPR64Op:$link),
                      (ins GPR64Op:$base, MMIXUImm8:$offset),
                      "GO $link, $base, $offset"> {
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
  let MMIXInventoryFamily = "control-flow";
}

class MMIXPushGoImm
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$count, GPR64Op:$base, MMIXUImm8:$offset),
                      "PUSHGO $count, $base, $offset"> {
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
  let MMIXInventoryFamily = "control-flow";
}

def POP : MMIXPop {
  let MMIXOpcode = 0xF8;
}

// Floating-point arithmetic operates on the 64-bit floating-point view of
// the general-purpose register file. These operations have no immediate
// variants in the MMIX instruction set.
class MMIXFPBinary<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins FPR64Op:$lhs, FPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs"> {
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
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPUnaryRoundedOpcode<bits<8> Opcode, string Mnemonic>
    : MMIXFPUnaryRounded<Mnemonic> {
  let MMIXOpcode = Opcode;
}

def FSQRT : MMIXFPUnaryRoundedOpcode<0x15, "FSQRT">;
def FINT  : MMIXFPUnaryRoundedOpcode<0x17, "FINT">;

class MMIXFix<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst), (ins FPR64Op:$src),
                      Mnemonic # " $dst, $src"> {
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
  let MMIXInventoryFamily = "floating-point";
  string MMIXSourceForm = "register";
}

class MMIXFloatFromIntImm<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins MMIXRoundingMode:$round, MMIXUImm8:$src),
                      Mnemonic # " $dst, $round, $src"> {
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
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPLoadImm<string Mnemonic>
    : MMIXInstruction<(outs FPR64Op:$dst),
                      (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                      Mnemonic # " $dst, $base, $offset"> {
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPStoreReg<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins FPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $value, $base, $offset"> {
  let MMIXInventoryFamily = "floating-point";
}

class MMIXFPStoreImm<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins FPR64Op:$value, GPR64Op:$base,
                           MMIXMemOffset8:$offset),
                      Mnemonic # " $value, $base, $offset"> {
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
                             "STCO $value, $base, $offset">;

class MMIXStoreConstantImm
    : MMIXMachineInstruction<(outs),
                             (ins MMIXUImm8:$value, GPR64Op:$base,
                                  MMIXMemOffset8:$offset),
                             "STCO $value, $base, $offset">;

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
                             Mnemonic # " $count, $base, $offset">;

class MMIXCacheImm<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins MMIXUImm8:$count, GPR64Op:$base,
                                  MMIXMemOffset8:$offset),
                             Mnemonic # " $count, $base, $offset">;

foreach Mnemonic = ["PRELD", "PREGO", "PREST", "SYNCD", "SYNCID"] in {
  def Mnemonic : MMIXCacheReg<Mnemonic>;
  def Mnemonic # "I" : MMIXCacheImm<Mnemonic>;
}

class MMIXUncachedLoad<string Mnemonic>
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, GPR64Op:$offset),
                             Mnemonic # " $dst, $base, $offset">;

class MMIXUncachedLoadImm<string Mnemonic>
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                             Mnemonic # " $dst, $base, $offset">;

class MMIXUncachedStore<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins GPR64Op:$value, GPR64Op:$base,
                                  GPR64Op:$offset),
                             Mnemonic # " $value, $base, $offset">;

class MMIXUncachedStoreImm<string Mnemonic>
    : MMIXMachineInstruction<(outs),
                             (ins GPR64Op:$value, GPR64Op:$base,
                                  MMIXMemOffset8:$offset),
                             Mnemonic # " $value, $base, $offset">;

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
                             "LDVTS $dst, $base, $offset">;

class MMIXVirtualTranslationImm
    : MMIXMachineInstruction<(outs GPR64Op:$dst),
                             (ins GPR64Op:$base, MMIXMemOffset8:$offset),
                             "LDVTS $dst, $base, $offset">;

def LDVTS : MMIXVirtualTranslationReg;
def LDVTSI : MMIXVirtualTranslationImm;

def SYNC : MMIXMachineInstruction<(outs), (ins MMIXUImm24:$mode),
                                   "SYNC $mode">;

def SWYM : MMIXMachineInstruction<
    (outs),
    (ins MMIXRegOrImm8:$x, MMIXRegOrImm8:$y, MMIXRegOrImm8:$z),
    "SWYM $x, $y, $z">;

class MMIXTrapInstruction<string Mnemonic>
    : MMIXMachineInstruction<
          (outs),
          (ins MMIXRegOrImm8:$x, MMIXRegOrImm8:$y, MMIXRegOrImm8:$z),
          Mnemonic # " $x, $y, $z">;

def TRAP : MMIXTrapInstruction<"TRAP">;
def TRIP : MMIXTrapInstruction<"TRIP">;

def RESUME : MMIXMachineInstruction<(outs), (ins MMIXUImm8:$mode),
                                     "RESUME $mode">;

def GET : MMIXMachineInstruction<(outs GPR64Op:$dst), (ins SPR64Op:$src),
                                 "GET $dst, $src">;

class MMIXPutReg
    : MMIXMachineInstruction<(outs), (ins SPR64Op:$dst, GPR64Op:$src),
                             "PUT $dst, $src">;

class MMIXPutImm
    : MMIXMachineInstruction<(outs), (ins SPR64Op:$dst, MMIXUImm8:$src),
                             "PUT $dst, $src">;

def PUT : MMIXPutReg;
def PUTI : MMIXPutImm;

class MMIXSave : MMIXMachineInstruction<(outs GPR64Op:$state), (ins),
                                         "SAVE $state">;
class MMIXUnsave : MMIXMachineInstruction<(outs), (ins GPR64Op:$state),
                                           "UNSAVE $state">;

def SAVE : MMIXSave {
  let MMIXOpcode = 0xFA;
}
def UNSAVE : MMIXUnsave {
  let MMIXOpcode = 0xFB;
}

// Keep pseudo instructions separate from architectural instructions.  This
// base is intentionally not used by the current inventory until lowering and
// expansion semantics are defined.
class MMIXPseudoInstruction<dag outs, dag ins, string asmstr>
    : MMIXInstruction<outs, ins, asmstr> {
  let isPseudo = true;
  let isCodeGenOnly = true;
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
