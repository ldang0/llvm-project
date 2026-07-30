//===-- MMIXMCTargetDesc.h - MMIX target descriptions ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXMCTARGETDESC_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXMCTARGETDESC_H

#include "llvm/ADT/StringRef.h"

namespace llvm {

class MCInstrInfo;
class MCRegisterInfo;
class MCSubtargetInfo;
class Triple;

MCInstrInfo *createMMIXMCInstrInfo();
MCRegisterInfo *createMMIXMCRegisterInfo(const Triple &TT);
MCSubtargetInfo *createMMIXMCSubtargetInfo(const Triple &TT, StringRef CPU,
                                           StringRef FS);

} // namespace llvm

#define GET_SUBTARGETINFO_ENUM
#include "MMIXGenSubtargetInfo.inc"

// Defines symbolic names for MMIX registers.
#define GET_REGINFO_ENUM
#include "MMIXGenRegisterInfo.inc"

// Defines symbolic names for MMIX instructions.
#define GET_INSTRINFO_ENUM
#include "MMIXGenInstrInfo.inc"

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXMCTARGETDESC_H
