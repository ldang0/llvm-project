//===-- MMIXRegisterInfo.td - MMIX register definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the architectural MMIX general-purpose registers. ABI
// roles and special registers are added separately from this base model.
//
//===----------------------------------------------------------------------===//

class MMIXReg<bits<8> Encoding, string Name> : Register<Name> {
  let Namespace = "MMIX";
  let HWEncoding{7-0} = Encoding;
}

foreach I = 0-255 in
  def R#I : MMIXReg<I, "r"#I>;

def GPR64 : RegisterClass<"MMIX", [i64], 64,
                          (sequence "R%u", 0, 255)>;
