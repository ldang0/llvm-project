//===-- MMIXFixupKinds.h - MMIX-specific fixup entries --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXFIXUPKINDS_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace MMIX {
enum FixupKind {
  fixup_mmix_branch_forward = FirstTargetFixupKind,
  fixup_mmix_branch_backward,
  fixup_mmix_jump_forward,
  fixup_mmix_jump_backward,
  fixup_mmix_addr19,
  fixup_mmix_addr27,
  fixup_mmix_call,
  fixup_mmix_direction_neutral_call,
  fixup_mmix_data_24,
  fixup_mmix_pcrel_24,
  fixup_mmix_geta,
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // namespace MMIX
} // namespace llvm

#endif
