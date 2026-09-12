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
  let DwarfNumbers =
      [!if(!ge(Encoding, 224), !sub(Encoding, 224), !add(Encoding, 48))];
}

foreach I = 0-255 in
  def R#I : MMIXReg<I, "r"#I>;

// Class intersections depend on physical registers and spill properties, not
// value types. All views must accept the same scalar types; keep the preferred
// type first. Even raw binary32 values use full-octa spills.
let Size = 64 in {
def GPR64 : RegisterClass<"MMIX", [i64, f64, f32], 64,
                          (sequence "R%u", 0, 255)>;

// MMIX uses the same 64-bit registers for integer and floating-point values.
def FPR64 : RegisterClass<"MMIX", [f64, i64, f32], 64,
                          (sequence "R%u", 0, 255)>;

// CodeGen uses only registers whose roles are fixed by the MMIX C ABI.
// Caller-clobbered globals precede hardware-preserved locals in allocation
// order. The architectural classes above remain complete for MC operands.
def GPR64CodeGen : RegisterClass<"MMIX", [i64, f64, f32], 64,
    (add (sequence "R%u", 250, 231), (sequence "R%u", 0, 30))>;

def FPR64CodeGen : RegisterClass<"MMIX", [f64, i64, f32], 64,
    (add (sequence "R%u", 250, 231), (sequence "R%u", 0, 30))>;

// Binary32 values held in registers are raw low-tetra encodings. Numerical
// operations first convert them to FPR64CodeGen through short-float memory.
def F32BitsCodeGen : RegisterClass<"MMIX", [f32, i64, f64], 64,
    (add (sequence "R%u", 250, 231), (sequence "R%u", 0, 30))>;
}

// MMIX special-register encodings are the values accepted by GET and PUT.
class MMIXSpecialReg<bits<5> Encoding, string Name,
                     list<int> DwarfNums = []> : Register<Name> {
  let Namespace = "MMIX";
  let PositionOrder = 1;
  let HWEncoding{4-0} = Encoding;
  let DwarfNumbers = DwarfNums;
}

def RA  : MMIXSpecialReg<21, "rA">;
def RB  : MMIXSpecialReg<0,  "rB">;
def RC  : MMIXSpecialReg<8,  "rC">;
def RD  : MMIXSpecialReg<1,  "rD", [32]>;
def RE  : MMIXSpecialReg<2,  "rE", [33]>;
def RF  : MMIXSpecialReg<22, "rF">;
def RG  : MMIXSpecialReg<19, "rG", [291]>;
def RH  : MMIXSpecialReg<3,  "rH", [34]>;
def RI  : MMIXSpecialReg<12, "rI">;
def RJ  : MMIXSpecialReg<4,  "rJ", [35]>;
def RK  : MMIXSpecialReg<15, "rK">;
def RL  : MMIXSpecialReg<20, "rL", [292]>;
def RM  : MMIXSpecialReg<5,  "rM">;
def RN  : MMIXSpecialReg<9,  "rN">;
def RO  : MMIXSpecialReg<10, "rO", [38]>;
def RP  : MMIXSpecialReg<23, "rP">;
def RQ  : MMIXSpecialReg<16, "rQ">;
def RR  : MMIXSpecialReg<6,  "rR", [36]>;
def RS  : MMIXSpecialReg<11, "rS", [283]>;
def RT  : MMIXSpecialReg<13, "rT">;
def RU  : MMIXSpecialReg<17, "rU">;
def RV  : MMIXSpecialReg<18, "rV">;
def RW  : MMIXSpecialReg<24, "rW">;
def RX  : MMIXSpecialReg<25, "rX">;
def RY  : MMIXSpecialReg<26, "rY">;
def RZ  : MMIXSpecialReg<27, "rZ">;

def RBB : MMIXSpecialReg<7,  "rBB">;
def RTT : MMIXSpecialReg<14, "rTT">;
def RWW : MMIXSpecialReg<28, "rWW">;
def RXX : MMIXSpecialReg<29, "rXX">;
def RYY : MMIXSpecialReg<30, "rYY">;
def RZZ : MMIXSpecialReg<31, "rZZ">;

def SPR64 : RegisterClass<"MMIX", [i64], 64,
                          (add RA, RB, RC, RD, RE, RF, RG, RH, RI, RJ, RK, RL,
                               RM, RN, RO, RP, RQ, RR, RS, RT, RU, RV, RW, RX,
                               RY, RZ, RBB, RTT, RWW, RXX, RYY, RZZ)> {
  let isAllocatable = false;
}
