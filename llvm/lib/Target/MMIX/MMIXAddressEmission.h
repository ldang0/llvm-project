//===-- MMIXAddressEmission.h - Emit static MMIX addresses ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXADDRESSEMISSION_H
#define LLVM_LIB_TARGET_MMIX_MMIXADDRESSEMISSION_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInst.h"

namespace llvm {

class MCContext;
class MCExpr;

enum class MMIXStaticAddressOutput {
  CanonicalAssembly,
  MMIXALAssembly,
  ELFObject,
};

SmallVector<MCInst, 4>
createMMIXStaticAddressSequence(MMIXStaticAddressOutput Output,
                                MCRegister Destination, const MCExpr *Address,
                                MCContext &Ctx);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXADDRESSEMISSION_H
