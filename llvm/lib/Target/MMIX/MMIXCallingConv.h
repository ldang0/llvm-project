//===-- MMIXCallingConv.h - MMIX calling convention policy -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MMIXCALLINGCONV_H
#define LLVM_LIB_TARGET_MMIX_MMIXCALLINGCONV_H

#include "llvm/IR/CallingConv.h"

namespace llvm {

inline bool isSupportedMMIXCallingConv(CallingConv::ID CC) {
  return CC == CallingConv::C || CC == CallingConv::Fast;
}

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MMIXCALLINGCONV_H
