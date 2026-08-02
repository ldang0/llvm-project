//===-- MMIXDisassembler.cpp - MMIX disassembler -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXBaseInfo.h"
#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDecoder.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include <cstdint>
#include <memory>

#define DEBUG_TYPE "mmix-disassembler"

using namespace llvm;
using namespace llvm::MCD;

using DecodeStatus = MCDisassembler::DecodeStatus;

namespace {

class MMIXDisassembler final : public MCDisassembler {
  std::unique_ptr<MCInstrInfo> MCII;

public:
  MMIXDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                   std::unique_ptr<MCInstrInfo> MCII)
      : MCDisassembler(STI, Ctx), MCII(std::move(MCII)) {}

  DecodeStatus decodePCRelativeOperand(MCInst &Inst, uint64_t Value) const;

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};

static DecodeStatus decodeRegisterClass(MCInst &Inst, uint64_t RegNo,
                                        unsigned RegClassID,
                                        const MCDisassembler *Decoder) {
  const MCRegisterInfo *MRI = Decoder->getContext().getRegisterInfo();
  const MCRegisterClass &RegClass = MRI->getRegClass(RegClassID);
  const unsigned NumRegs = RegClass.getNumRegs();
  if (RegNo < NumRegs) {
    const MCRegister Reg = RegClass.getRegister(RegNo);
    if (MRI->getEncodingValue(Reg) == RegNo) {
      Inst.addOperand(MCOperand::createReg(Reg));
      return MCDisassembler::Success;
    }
  }

  for (unsigned I = 0; I != NumRegs; ++I) {
    const MCRegister Reg = RegClass.getRegister(I);
    if (MRI->getEncodingValue(Reg) == RegNo) {
      Inst.addOperand(MCOperand::createReg(Reg));
      return MCDisassembler::Success;
    }
  }
  return MCDisassembler::Fail;
}

static DecodeStatus DecodeGPR64RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t /*Address*/,
                                             const MCDisassembler *Decoder) {
  return decodeRegisterClass(Inst, RegNo, MMIX::GPR64RegClassID, Decoder);
}

static DecodeStatus DecodeFPR64RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t /*Address*/,
                                             const MCDisassembler *Decoder) {
  return decodeRegisterClass(Inst, RegNo, MMIX::FPR64RegClassID, Decoder);
}

static DecodeStatus DecodeSPR64RegisterClass(MCInst &Inst, uint64_t RegNo,
                                             uint64_t /*Address*/,
                                             const MCDisassembler *Decoder) {
  return decodeRegisterClass(Inst, RegNo, MMIX::SPR64RegClassID, Decoder);
}

static DecodeStatus decodeRoundingMode(MCInst &Inst, uint64_t Value,
                                       uint64_t /*Address*/,
                                       const MCDisassembler * /*Decoder*/) {
  if (Value > 4)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Value));
  return MCDisassembler::Success;
}

static DecodeStatus decodePCRelativeOperand(MCInst &Inst, uint64_t Value,
                                            uint64_t /*Address*/,
                                            const MCDisassembler *Decoder) {
  return static_cast<const MMIXDisassembler *>(Decoder)
      ->decodePCRelativeOperand(Inst, Value);
}

} // namespace

#include "MMIXGenDisassemblerTables.inc"

namespace {

DecodeStatus MMIXDisassembler::decodePCRelativeOperand(MCInst &Inst,
                                                       uint64_t Value) const {
  const uint64_t TSFlags = MCII->get(Inst.getOpcode()).TSFlags;
  const unsigned Width = MMIXII::getPCRelativeWidth(TSFlags);
  if (Width != 16 && Width != 24)
    return MCDisassembler::Fail;

  int64_t DecodedValue = Value;
  if (TSFlags & MMIXII::PCRelativeBackward)
    DecodedValue -= int64_t(1) << Width;
  Inst.addOperand(MCOperand::createImm(DecodedValue));
  return MCDisassembler::Success;
}

DecodeStatus MMIXDisassembler::getInstruction(MCInst &Instr, uint64_t &Size,
                                              ArrayRef<uint8_t> Bytes,
                                              uint64_t Address,
                                              raw_ostream &CStream) const {
  (void)CStream;
  Size = 0;
  if (Bytes.size() < 4)
    return MCDisassembler::Fail;

  const uint32_t Word = (uint32_t(Bytes[0]) << 24) |
                        (uint32_t(Bytes[1]) << 16) | (uint32_t(Bytes[2]) << 8) |
                        uint32_t(Bytes[3]);
  const DecodeStatus Result = decodeInstruction(
      DecoderTable32, Instr, Word, Address, this, getSubtargetInfo());
  if (Result != MCDisassembler::Fail)
    Size = 4;
  return Result;
}

} // namespace

static MCDisassembler *createMMIXDisassembler(const Target & /*T*/,
                                              const MCSubtargetInfo &STI,
                                              MCContext &Ctx) {
  return new MMIXDisassembler(
      STI, Ctx, std::unique_ptr<MCInstrInfo>(createMMIXMCInstrInfo()));
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeMMIXDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheMMIXTarget(),
                                         createMMIXDisassembler);
}
