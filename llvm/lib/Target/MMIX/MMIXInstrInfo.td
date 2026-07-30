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
