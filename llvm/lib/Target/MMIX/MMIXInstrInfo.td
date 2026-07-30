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
