//===-- MMIXCallEmission.cpp - Emit direct MMIX calls --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXCallEmission.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace llvm;

std::optional<MCInst> llvm::createMMIXUnresolvedDirectCall(
    MMIXEmissionMode Mode, MCRegister CallOperand, const MCExpr *Callee) {
  assert(CallOperand && Callee && "invalid MMIX direct call operands");
  switch (Mode) {
  case MMIXEmissionMode::CanonicalAssembly:
  case MMIXEmissionMode::MMIXALAssembly:
    return std::nullopt;
  case MMIXEmissionMode::ELFObject:
    MCInst Inst;
    Inst.setOpcode(MMIX::PUSHJ);
    Inst.setFlags(MMIXII::DirectionNeutralCall);
    Inst.addOperand(MCOperand::createReg(CallOperand));
    Inst.addOperand(MCOperand::createExpr(Callee));
    return Inst;
  }
  llvm_unreachable("invalid MMIX emission mode");
}
