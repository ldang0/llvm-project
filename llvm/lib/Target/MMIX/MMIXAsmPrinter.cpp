//===-- MMIXAsmPrinter.cpp - MMIX assembly printer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXMCInstLower.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace {

class MMIXAsmPrinter final : public AsmPrinter {
public:
  explicit MMIXAsmPrinter(TargetMachine &TM,
                          std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "MMIX Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->isPseudo() && MI->getOpcode() != MMIX::PseudoB &&
        MI->getOpcode() != MMIX::PseudoJMP &&
        MI->getOpcode() != MMIX::PseudoPUSHJ &&
        MI->getOpcode() != MMIX::PseudoPUSHGO)
      report_fatal_error(
          "MMIX CodeGen pseudo reached canonical assembly emission");

    MCInst OutMI;
    MMIXMCInstLower(OutContext, *this).lower(*MI, OutMI);
    if (!MMIX_MC::isOpcodeAvailable(OutMI.getOpcode(),
                                    getSubtargetInfo().getFeatureBits()))
      report_fatal_error(Twine("cannot emit ") +
                         TM.getMCInstrInfo()->getName(OutMI.getOpcode()) +
                         ": required target feature is disabled");
    MMIX_MC::verifyInstructionPredicates(OutMI.getOpcode(),
                                         getSubtargetInfo().getFeatureBits());
    EmitToStreamer(*OutStreamer, OutMI);
  }
};

} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeMMIXAsmPrinter() {
  RegisterAsmPrinter<MMIXAsmPrinter> X(getTheMMIXTarget());
}
