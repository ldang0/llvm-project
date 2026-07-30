//===-- MMIXInstrInfo.td - MMIX Instruction Definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Common operand categories. These definitions intentionally contain no
// encoding or parser methods; those are added with the corresponding MC
// instruction families.
def GPR64Op : RegisterOperand<GPR64>;
def FPR64Op : RegisterOperand<FPR64>;
def SPR64Op : RegisterOperand<SPR64>;

def MMIXImm64 : Operand<i64>;
def MMIXUImm8 : Operand<i64>;
def MMIXUImm16 : Operand<i64>;

def MMIXBranchTarget : Operand<OtherVT> {
  let OperandType = "OPERAND_PCREL";
}

def MMIXMemAddr : Operand<i64> {
  let OperandType = "OPERAND_MEMORY";
}

// Base record for architectural MMIX instructions. Concrete instruction
// families supply operand lists, assembly spelling, and later encodings.
class MMIXInstruction<dag outs, dag ins, string asmstr> : Instruction {
  let Namespace = "MMIX";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Size = 4;
}

// The ordinary integer form has a register or unsigned-byte Z operand. The
// two forms remain separate records so later MC encodings can distinguish
// their opcode fields.
class MMIXIntRegReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$lhs, GPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs">;

class MMIXIntRegImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$lhs, MMIXUImm8:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs">;

// NEG and NEGU use an unsigned byte in the Y field even in their register-Z
// form, unlike the ordinary X,Y,Z instruction family.
class MMIXNegReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm8:$lhs, GPR64Op:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs">;

class MMIXNegImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm8:$lhs, MMIXUImm8:$rhs),
                      Mnemonic # " $dst, $lhs, $rhs">;

class MMIXIntConst<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins MMIXUImm16:$value),
                      Mnemonic # " $dst, $value">;

// Arithmetic, shifts, comparisons, conditional sets, and bit operations all
// have the architectural register and immediate variants listed here.
foreach Mnemonic = [
  "MUL", "MULU", "DIV", "DIVU",
  "ADD", "ADDU", "SUB", "SUBU",
  "CMP", "CMPU", "SL", "SLU", "SR", "SRU",
  "CSN", "CSZ", "CSP", "CSOD", "CSNN", "CSNZ", "CSNP", "CSEV",
  "ZSN", "ZSZ", "ZSP", "ZSOD", "ZSNN", "ZSNZ", "ZSNP", "ZSEV",
  "OR", "ORN", "NOR", "XOR", "AND", "ANDN", "NAND", "NXOR",
  "BDIF", "WDIF", "TDIF", "ODIF",
  "MUX", "SADD", "MOR", "MXOR"
] in {
  def Mnemonic : MMIXIntRegReg<Mnemonic>;
  def Mnemonic # "I" : MMIXIntRegImm<Mnemonic>;
}

def TWOADDU : MMIXIntRegReg<"2ADDU">;
def TWOADDUI : MMIXIntRegImm<"2ADDU">;
def FOURADDU : MMIXIntRegReg<"4ADDU">;
def FOURADDUI : MMIXIntRegImm<"4ADDU">;
def EIGHTADDU : MMIXIntRegReg<"8ADDU">;
def EIGHTADDUI : MMIXIntRegImm<"8ADDU">;
def SIXTEENADDU : MMIXIntRegReg<"16ADDU">;
def SIXTEENADDUI : MMIXIntRegImm<"16ADDU">;

foreach Mnemonic = ["NEG", "NEGU"] in {
  def Mnemonic : MMIXNegReg<Mnemonic>;
  def Mnemonic # "I" : MMIXNegImm<Mnemonic>;
}

foreach Mnemonic = [
  "SETH", "SETMH", "SETML", "SETL",
  "INCH", "INCMH", "INCML", "INCL",
  "ORH", "ORMH", "ORML", "ORL",
  "ANDNH", "ANDNMH", "ANDNML", "ANDNL"
] in
  def Mnemonic : MMIXIntConst<Mnemonic>;

// Integer memory operations use a register base and either a register or an
// unsigned-byte offset. The load and store records keep width and signedness
// variants distinct for the later MC and lowering work.
class MMIXLoadReg<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $dst, $base, $offset">;

class MMIXLoadImm<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst),
                      (ins GPR64Op:$base, MMIXUImm8:$offset),
                      Mnemonic # " $dst, $base, $offset">;

class MMIXStoreReg<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      Mnemonic # " $value, $base, $offset">;

class MMIXStoreImm<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$value, GPR64Op:$base, MMIXUImm8:$offset),
                      Mnemonic # " $value, $base, $offset">;

foreach Mnemonic = [
  "LDB", "LDBU", "LDW", "LDWU", "LDT", "LDTU", "LDO", "LDOU", "LDHT"
] in {
  def Mnemonic : MMIXLoadReg<Mnemonic>;
  def Mnemonic # "I" : MMIXLoadImm<Mnemonic>;
}

foreach Mnemonic = [
  "STB", "STBU", "STW", "STWU", "STT", "STTU", "STO", "STOU", "STHT"
] in {
  def Mnemonic : MMIXStoreReg<Mnemonic>;
  def Mnemonic # "I" : MMIXStoreImm<Mnemonic>;
}

// CSWAP has the ordinary memory address forms but also updates its X operand
// with the comparison result. The data dependency is left for later lowering.
class MMIXCompareSwapReg
    : MMIXInstruction<(outs GPR64Op:$result),
                      (ins GPR64Op:$value, GPR64Op:$base, GPR64Op:$offset),
                      "CSWAP $result, $base, $offset">;

class MMIXCompareSwapImm
    : MMIXInstruction<(outs GPR64Op:$result),
                      (ins GPR64Op:$value, GPR64Op:$base, MMIXUImm8:$offset),
                      "CSWAP $result, $base, $offset">;

def CSWAP : MMIXCompareSwapReg;
def CSWAPI : MMIXCompareSwapImm;

class MMIXCondBranch<string Mnemonic>
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$condition, MMIXBranchTarget:$target),
                      Mnemonic # " $condition, $target">;

foreach Mnemonic = ["BN", "BZ", "BP", "BOD", "BNN", "BNZ", "BNP", "BEV"] in {
  def Mnemonic : MMIXCondBranch<Mnemonic>;
  def Mnemonic # "B" : MMIXCondBranch<Mnemonic # "B">;
}

foreach Mnemonic = [
  "PBN", "PBZ", "PBP", "PBOD", "PBNN", "PBNZ", "PBNP", "PBEV"
] in {
  def Mnemonic : MMIXCondBranch<Mnemonic>;
  def Mnemonic # "B" : MMIXCondBranch<Mnemonic # "B">;
}

class MMIXJump<string Mnemonic>
    : MMIXInstruction<(outs), (ins MMIXBranchTarget:$target),
                      Mnemonic # " $target">;

def JMP : MMIXJump<"JMP">;
def JMPB : MMIXJump<"JMPB">;

class MMIXGetAddress<string Mnemonic>
    : MMIXInstruction<(outs GPR64Op:$dst), (ins MMIXBranchTarget:$target),
                      Mnemonic # " $dst, $target">;

def GETA : MMIXGetAddress<"GETA">;
def GETAB : MMIXGetAddress<"GETAB">;

class MMIXPushJump<string Mnemonic>
    : MMIXInstruction<(outs), (ins GPR64Op:$link, MMIXBranchTarget:$target),
                      Mnemonic # " $link, $target">;

def PUSHJ : MMIXPushJump<"PUSHJ">;
def PUSHJB : MMIXPushJump<"PUSHJB">;

class MMIXGoReg
    : MMIXInstruction<(outs GPR64Op:$link),
                      (ins GPR64Op:$base, GPR64Op:$offset),
                      "GO $link, $base, $offset">;

class MMIXGoImm
    : MMIXInstruction<(outs GPR64Op:$link),
                      (ins GPR64Op:$base, MMIXUImm8:$offset),
                      "GO $link, $base, $offset">;

def GO : MMIXGoReg;
def GOI : MMIXGoImm;

class MMIXPushGoReg
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$count, GPR64Op:$base, GPR64Op:$offset),
                      "PUSHGO $count, $base, $offset">;

class MMIXPushGoImm
    : MMIXInstruction<(outs),
                      (ins GPR64Op:$count, GPR64Op:$base, MMIXUImm8:$offset),
                      "PUSHGO $count, $base, $offset">;

def PUSHGO : MMIXPushGoReg;
def PUSHGOI : MMIXPushGoImm;

def POP : MMIXInstruction<(outs),
                          (ins MMIXUImm8:$count, MMIXUImm16:$target),
                          "POP $count, $target">;
