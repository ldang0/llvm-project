//===-- MMIXALStartup.cpp - Build MMIXAL bare-metal startup --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALStartup.h"
#include "MMIXALAsmStreamer.h"
#include "MMIXALLayout.h"
#include "llvm/ADT/Twine.h"
#include <string>

using namespace llvm;

void llvm::addMMIXALBareMetalGlobalRegisterPrelude(
    MMIXALAsmStreamer &Streamer) {
  for (unsigned Register = 254; Register >= 231; --Register) {
    std::string Name;
    if (Register == 254)
      Name = "__LLVM_G_SP";
    else if (Register == 253)
      Name = "__LLVM_G_FP";
    else
      Name = (Twine("__LLVM_G_R") + Twine(Register)).str();

    const uint64_t InitialValue =
        Register == 254 ? MMIXALBareMetalProfile::MemoryStackEnd : 0;
    Streamer.addPreludeGlobalRegister(Name, Register, InitialValue);
  }
  Streamer.finalizePrelude();
}
