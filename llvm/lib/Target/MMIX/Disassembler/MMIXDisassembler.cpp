//===-- MMIXDisassembler.cpp - MMIX disassembler -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/MMIXMCTargetDesc.h"
#include "TargetInfo/MMIXTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>
#include <memory>

using namespace llvm;

namespace {

class MMIXDisassembler final : public MCDisassembler {
  std::unique_ptr<MCInstrInfo> MCII;
  const MCRegisterInfo &MRI;

  static bool isBranch(unsigned Opcode) {
    return Opcode >= 0x40 && Opcode <= 0x5f;
  }

  static bool isBackward(unsigned Opcode) {
    return (isBranch(Opcode) && (Opcode & 1)) || Opcode == 0xf1 ||
           Opcode == 0xf3 || Opcode == 0xf5;
  }

  static unsigned getOperandShift(unsigned Opcode, unsigned Operand) {
    if (isBranch(Opcode))
      return Operand == 1 ? 0 : 16;
    if (Opcode == 0xf0 || Opcode == 0xf1)
      return 0;
    if (Opcode >= 0xf2 && Opcode <= 0xf5)
      return Operand == 0 ? 16 : 0;
    if (Opcode == 0xf8)
      return Operand == 0 ? 16 : 0;
    if (Opcode >= 0xe0 && Opcode <= 0xef && Operand == 1)
      return 0;
    if (Opcode == 0xfc || Opcode == 0xf9 || Opcode == 0xfb)
      return 0;
    if (Opcode == 0xfe && Operand == 1)
      return 0;
    if ((Opcode == 0xf6 || Opcode == 0xf7) && Operand == 1)
      return 0;
    return 16 - 8 * Operand;
  }

  static unsigned getOperandWidth(unsigned Opcode, unsigned Operand) {
    if (isBranch(Opcode) && Operand == 1)
      return 16;
    if (Opcode == 0xf0 || Opcode == 0xf1)
      return 24;
    if (Opcode >= 0xf2 && Opcode <= 0xf5 && Operand == 1)
      return 16;
    if (Opcode == 0xf8 && Operand == 1)
      return 16;
    if (Opcode >= 0xe0 && Opcode <= 0xef && Operand == 1)
      return 16;
    if (Opcode == 0xfc)
      return 24;
    return 8;
  }

  static uint32_t getField(uint32_t Word, unsigned Opcode, unsigned Operand) {
    const unsigned Width = getOperandWidth(Opcode, Operand);
    const unsigned Shift = getOperandShift(Opcode, Operand);
    return (Word >> Shift) & ((uint32_t(1) << Width) - 1);
  }

  MCRegister getGPR(unsigned Encoding) const {
    return MRI.getRegClass(MMIX::GPR64RegClassID).getRegister(Encoding);
  }

  MCRegister getSPR(unsigned Encoding) const {
    // MMIXware assigns the special-register numbers in an order different
    // from the spelling order used by the TableGen register class.
    static const MCRegister SPRs[] = {
        MMIX::RB,  MMIX::RD,  MMIX::RE,  MMIX::RH,  MMIX::RJ,  MMIX::RM,
        MMIX::RR,  MMIX::RBB, MMIX::RC,  MMIX::RN,  MMIX::RO,  MMIX::RS,
        MMIX::RI,  MMIX::RT,  MMIX::RTT, MMIX::RK,  MMIX::RQ,  MMIX::RU,
        MMIX::RV,  MMIX::RG,  MMIX::RL,  MMIX::RA,  MMIX::RF,  MMIX::RP,
        MMIX::RW,  MMIX::RX,  MMIX::RY,  MMIX::RZ,  MMIX::RWW, MMIX::RXX,
        MMIX::RYY, MMIX::RZZ};
    return SPRs[Encoding];
  }

  static int64_t getPCRelativeValue(uint32_t Value, unsigned Width,
                                    bool Backward) {
    if (Backward)
      return static_cast<int64_t>(Value) - (int64_t(1) << Width);
    return Value;
  }

  static bool hasValidFixedFields(uint32_t Word, unsigned Opcode) {
    const unsigned X = (Word >> 16) & 0xff;
    const unsigned Y = (Word >> 8) & 0xff;
    const unsigned Z = Word & 0xff;

    if (Opcode == 0x05 || Opcode == 0x07 ||
        (Opcode >= 0x08 && Opcode <= 0x0f) || Opcode == 0x15 ||
        Opcode == 0x17)
      return Y <= 4;
    if (Opcode == 0xf6 || Opcode == 0xf7)
      return X < 32 && Y == 0;
    if (Opcode == 0xf9)
      return X == 0 && Y == 0 && Z <= 1;
    if (Opcode == 0xfa)
      return Y == 0 && Z == 0;
    if (Opcode == 0xfb)
      return X == 0 && Y == 0;
    if (Opcode == 0xfc)
      return (Word & 0xffffff) <= 7;
    if (Opcode == 0xfe)
      return Y == 0 && Z < 32;
    return true;
  }

public:
  MMIXDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                   std::unique_ptr<MCInstrInfo> MCII)
      : MCDisassembler(STI, Ctx), MCII(std::move(MCII)),
        MRI(*Ctx.getRegisterInfo()) {}

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                               ArrayRef<uint8_t> Bytes, uint64_t Address,
                               raw_ostream &CStream) const override {
    (void)Address;
    (void)CStream;
    Size = 0;
    if (Bytes.size() < 4)
      return Fail;

    const uint32_t Word = (uint32_t(Bytes[0]) << 24) |
                          (uint32_t(Bytes[1]) << 16) |
                          (uint32_t(Bytes[2]) << 8) | uint32_t(Bytes[3]);
    const unsigned ArchOpcode = Word >> 24;
    if (!hasValidFixedFields(Word, ArchOpcode))
      return Fail;
    unsigned Opcode = MMIX::TRAP;
    if (ArchOpcode != 0) {
      Opcode = 0;
      for (unsigned I = 0, E = MCII->getNumOpcodes(); I != E; ++I) {
        if ((MCII->get(I).TSFlags & 0xff) == ArchOpcode) {
          Opcode = I;
          break;
        }
      }
    }
    if (!Opcode)
      return Fail;

    const MCInstrDesc &Desc = MCII->get(Opcode);
    Instr.setOpcode(Opcode);
    for (unsigned I = 0, E = Desc.getNumOperands(); I != E; ++I) {
      const unsigned Field = getField(Word, ArchOpcode, I);
      const MCOperandInfo &Info = Desc.operands()[I];

      if (ArchOpcode == 0xfe && I == 1) {
        if (Field >= 32)
          return Fail;
        Instr.addOperand(MCOperand::createReg(getSPR(Field)));
      } else if ((ArchOpcode == 0xf6 || ArchOpcode == 0xf7) && I == 0) {
        if (Field >= 32)
          return Fail;
        Instr.addOperand(MCOperand::createReg(getSPR(Field)));
      } else if (Info.OperandType == MCOI::OPERAND_REGISTER) {
        Instr.addOperand(MCOperand::createReg(getGPR(Field)));
      } else if (Info.OperandType == MCOI::OPERAND_PCREL) {
        const unsigned Width = getOperandWidth(ArchOpcode, I);
        Instr.addOperand(MCOperand::createImm(
            getPCRelativeValue(Field, Width, isBackward(ArchOpcode))));
      } else {
        Instr.addOperand(MCOperand::createImm(Field));
      }
    }

    Size = 4;
    return Success;
  }
};

} // namespace

static MCDisassembler *createMMIXDisassembler(const Target &T,
                                              const MCSubtargetInfo &STI,
                                              MCContext &Ctx) {
  return new MMIXDisassembler(STI, Ctx,
                              std::unique_ptr<MCInstrInfo>(
                                  createMMIXMCInstrInfo()));
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeMMIXDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheMMIXTarget(),
                                         createMMIXDisassembler);
}
