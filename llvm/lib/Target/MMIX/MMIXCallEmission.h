//===-- MMIXCallEmission.h - Emit direct MMIX calls -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXCALLEMISSION_H
#define LLVM_LIB_TARGET_MMIX_MMIXCALLEMISSION_H

#include "MMIXEmissionMode.h"
#include "llvm/MC/MCInst.h"
#include <optional>

namespace llvm {

class MCExpr;

// ELF objects can preserve an unresolved direct call for the linker. Text
// outputs return no instruction so their existing conservative indirect-call
// path remains responsible for materializing the callee address.
std::optional<MCInst> createMMIXUnresolvedDirectCall(MMIXEmissionMode Mode,
                                                     MCRegister CallOperand,
                                                     const MCExpr *Callee);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXCALLEMISSION_H
