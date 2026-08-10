//===-- MMIXAddressEmission.cpp - Emit static MMIX addresses ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXAddressEmission.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"
#include <array>

using namespace llvm;

static const MCExpr *createWydeExpression(const MCExpr *Address, unsigned Shift,
                                          MCContext &Ctx) {
  const MCExpr *Expr = Address;
  if (Shift)
    Expr =
        MCBinaryExpr::createLShr(Expr, MCConstantExpr::create(Shift, Ctx), Ctx);
  return MCBinaryExpr::createAnd(Expr, MCConstantExpr::create(0xffff, Ctx),
                                 Ctx);
}

SmallVector<MCInst, 4>
llvm::createMMIXStaticAddressSequence(MMIXStaticAddressOutput Output,
                                      MCRegister Destination,
                                      const MCExpr *Address, MCContext &Ctx) {
  SmallVector<MCInst, 4> Sequence;
  switch (Output) {
  case MMIXStaticAddressOutput::CanonicalAssembly:
  case MMIXStaticAddressOutput::MMIXALAssembly: {
    static constexpr std::array<unsigned, 4> Opcodes = {
        MMIX::SETH, MMIX::INCMH, MMIX::INCML, MMIX::INCL};
    static constexpr std::array<unsigned, 4> Shifts = {48, 32, 16, 0};
    for (unsigned I = 0; I != Opcodes.size(); ++I) {
      MCInst Inst;
      Inst.setOpcode(Opcodes[I]);
      Inst.addOperand(MCOperand::createReg(Destination));
      Inst.addOperand(
          MCOperand::createExpr(createWydeExpression(Address, Shifts[I], Ctx)));
      Sequence.push_back(Inst);
    }
    return Sequence;
  }
  case MMIXStaticAddressOutput::ELFObject: {
    MCInst Inst;
    Inst.setOpcode(MMIX::GETA);
    Inst.addOperand(MCOperand::createReg(Destination));
    Inst.addOperand(MCOperand::createExpr(
        MCSpecifierExpr::create(Address, MMIXII::S_GETA, Ctx)));
    Inst.addOperand(MCOperand::createImm(MMIXII::GETARelocationReservedSlots));
    Sequence.push_back(Inst);
    return Sequence;
  }
  }
  llvm_unreachable("invalid MMIX static-address output kind");
}
