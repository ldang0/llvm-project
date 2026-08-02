//===-- MMIXCallingConv.td - MMIX calling conventions ------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Local registers are preserved by PUSHJ/PUSHGO and POP rather than by
// ordinary memory spills. The frame pointer is saved only when a function uses
// it. Stack and register-stack state is preserved without appearing in the
// software callee-save list.
def CSR_MMIX : CalleeSavedRegs<
    (add (sequence "R%u", 0, 30), R253)> {
  let OtherPreserved = (add R254, RG, RL, RO);
}
