//===-- MMIXAsmPrinter.cpp - MMIX assembly printer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXALAsmStreamer.h"
#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXInstPrinter.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "MMIXMCInstLower.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace {

class MMIXAsmPrinter final : public AsmPrinter {
  MMIXALAsmStreamer *MMIXALStreamer;

  void reportSymbolRegistrationError(Error Err) {
    if (Err)
      OutContext.reportError(SMLoc(), toString(std::move(Err)));
  }

  void registerModuleSymbols(Module &M) {
    if (!MMIXALStreamer)
      return;

    for (Function &F : M)
      if (!F.isIntrinsic())
        reportSymbolRegistrationError(
            MMIXALStreamer->registerUserSymbol(*getSymbol(&F), F.getName()));
    for (GlobalVariable &Global : M.globals())
      reportSymbolRegistrationError(MMIXALStreamer->registerUserSymbol(
          *getSymbol(&Global), Global.getName()));
    for (GlobalAlias &Alias : M.aliases())
      reportSymbolRegistrationError(MMIXALStreamer->registerUserSymbol(
          *getSymbol(&Alias), Alias.getName()));
  }

  void registerFunctionSymbols(MachineFunction &MF) {
    if (!MMIXALStreamer)
      return;

    using Kind = MMIXALSymbolTable::PrivateSymbolKind;
    const Function &F = MF.getFunction();
    const StringRef FunctionName = F.getName();
    reportSymbolRegistrationError(
        MMIXALStreamer->beginFunctionSymbols(FunctionName));

    SmallPtrSet<const BasicBlock *, 8> RegisteredSourceBlocks;
    uint64_t BasicBlockOrdinal = 0;
    for (MachineBasicBlock &MBB : MF) {
      reportSymbolRegistrationError(
          MMIXALStreamer->registerFunctionPrivateSymbol(
              *MBB.getSymbol(), FunctionName, Kind::BasicBlock,
              BasicBlockOrdinal++));

      const BasicBlock *BB = MBB.getBasicBlock();
      if (!BB || !BB->hasName() || !RegisteredSourceBlocks.insert(BB).second)
        continue;
      const MCSymbol *AddressSymbol =
          MBB.isEntryBlock() ? CurrentFnSym : MBB.getSymbol();
      reportSymbolRegistrationError(MMIXALStreamer->registerSourceBlock(
          *AddressSymbol, FunctionName, BB->getName()));
    }

    const auto &Constants = MF.getConstantPool()->getConstants();
    for (uint64_t I = 0; I != Constants.size(); ++I)
      reportSymbolRegistrationError(
          MMIXALStreamer->registerFunctionPrivateSymbol(
              *GetCPISymbol(I), FunctionName, Kind::ConstantPool, I));

    if (const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo())
      for (uint64_t I = 0; I != MJTI->getJumpTables().size(); ++I)
        reportSymbolRegistrationError(
            MMIXALStreamer->registerFunctionPrivateSymbol(
                *GetJTISymbol(I), FunctionName, Kind::JumpTable, I));

    uint64_t BlockAddressOrdinal = 0;
    for (const BasicBlock &BB : F)
      if (BB.hasAddressTaken())
        reportSymbolRegistrationError(
            MMIXALStreamer->registerFunctionPrivateSymbol(
                *GetBlockAddressSymbol(&BB), FunctionName, Kind::BlockAddress,
                BlockAddressOrdinal++));
  }

public:
  explicit MMIXAsmPrinter(TargetMachine &TM,
                          std::unique_ptr<MCStreamer> Streamer,
                          MMIXALAsmStreamer *MMIXALStreamer = nullptr)
      : AsmPrinter(TM, std::move(Streamer)), MMIXALStreamer(MMIXALStreamer) {}

  StringRef getPassName() const override { return "MMIX Assembly Printer"; }

  void emitStartOfAsmFile(Module &M) override { registerModuleSymbols(M); }

  bool runOnMachineFunction(MachineFunction &MF) override {
    SetupMachineFunction(MF);
    registerFunctionSymbols(MF);
    emitFunctionBody();
    if (MMIXALStreamer)
      reportSymbolRegistrationError(
          MMIXALStreamer->endFunctionSymbols(getFunctionEnd()));
    return false;
  }

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &OS) override {
    if (!AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS))
      return false;
    if (ExtraCode && ExtraCode[0])
      return true;

    const MachineOperand &MO = MI->getOperand(OpNo);
    if (MO.isReg()) {
      if (!MO.getReg())
        return true;
      OS << MMIXInstPrinter::getRegisterName(MO.getReg());
      return false;
    }
    if (MO.isImm()) {
      OS << MO.getImm();
      return false;
    }
    if (MO.isGlobal()) {
      PrintSymbolOperand(MO, OS);
      return false;
    }
    if (MO.isMBB()) {
      MO.getMBB()->getSymbol()->print(OS, MAI);
      return false;
    }
    if (MO.isBlockAddress()) {
      GetBlockAddressSymbol(MO.getBlockAddress())->print(OS, MAI);
      return false;
    }
    if (MO.isSymbol()) {
      GetExternalSymbolSymbol(MO.getSymbolName())->print(OS, MAI);
      return false;
    }
    return true;
  }

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

static AsmPrinter *
createMMIXAsmPrinter(TargetMachine &TM,
                     std::unique_ptr<MCStreamer> &&Streamer) {
  if (TM.getMCAsmInfo().getOutputAssemblerDialect() ==
      MMIXII::MMIXALAsmVariant) {
    auto *MMIXALStreamer = static_cast<MMIXALAsmStreamer *>(Streamer.get());
    MMIXALStreamer->beginModuleEmission();
    return new MMIXAsmPrinter(TM, std::move(Streamer), MMIXALStreamer);
  }
  return new MMIXAsmPrinter(TM, std::move(Streamer));
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeMMIXAsmPrinter() {
  TargetRegistry::RegisterAsmPrinter(getTheMMIXTarget(), createMMIXAsmPrinter);
}
