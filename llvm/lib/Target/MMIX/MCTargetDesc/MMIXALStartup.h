//===-- MMIXALStartup.h - Build MMIXAL bare-metal startup -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSTARTUP_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSTARTUP_H

namespace llvm {

class MMIXALAsmStreamer;

void addMMIXALBareMetalGlobalRegisterPrelude(MMIXALAsmStreamer &Streamer);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSTARTUP_H
