// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the MMIXware floating-point opcode allocation and operand families.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXFPInstructions = !listconcat(
  !instances<MMIXFPBinaryOpcode>(), !instances<MMIXFPCompareOpcode>(),
  !instances<MMIXFPUnaryRoundedOpcode>(), !instances<MMIXFixOpcode>(),
  !instances<MMIXFloatFromIntRegOpcode>(),
  !instances<MMIXFloatFromIntImmOpcode>(),
  !instances<MMIXFPLoadRegOpcode>(), !instances<MMIXFPLoadImmOpcode>(),
  !instances<MMIXFPStoreRegOpcode>(), !instances<MMIXFPStoreImmOpcode>());

assert !eq(!size(!instances<MMIXFPBinaryOpcode>()), 5),
       "unexpected MMIX floating-point binary forms";
assert !eq(!size(!instances<MMIXFPCompareOpcode>()), 6),
       "unexpected MMIX floating-point comparison forms";
assert !eq(!size(!instances<MMIXFPUnaryRoundedOpcode>()), 2),
       "unexpected MMIX floating-point unary forms";
assert !eq(!size(!instances<MMIXFixOpcode>()), 2),
       "unexpected MMIX floating-point integer conversion forms";
assert !eq(!size(!instances<MMIXFloatFromIntRegOpcode>()), 4),
       "unexpected MMIX register floating conversion forms";
assert !eq(!size(!instances<MMIXFloatFromIntImmOpcode>()), 4),
       "unexpected MMIX immediate floating conversion forms";

foreach Record = MMIXFPInstructions in {
  assert !ne(Record.MMIXOpcode, 0),
         "MMIX floating-point instruction has no opcode allocation";
}

assert !eq(FADD.MMIXOpcode, 0x04), "FADD opcode is incorrect";
assert !eq(FCMP.MMIXOpcode, 0x01), "FCMP opcode is incorrect";
assert !eq(FSQRT.MMIXOpcode, 0x15), "FSQRT opcode is incorrect";
assert !eq(FIXU.MMIXOpcode, 0x07), "FIXU opcode is incorrect";
assert !eq(FLOTUI.MMIXOpcode, 0x0B), "FLOTUI opcode is incorrect";
assert !eq(SFLOTUI.MMIXOpcode, 0x0F), "SFLOTUI opcode is incorrect";
assert !eq(LDSFI.MMIXOpcode, 0x91), "LDSFI opcode is incorrect";
assert !eq(STSFI.MMIXOpcode, 0xB1), "STSFI opcode is incorrect";

assert !eq(FADD.MMIXInventoryFamily, "floating-point"),
       "floating-point instructions must retain their family metadata";
assert !eq(FLOT.MMIXSourceForm, "register"),
       "FLOT must use a register source operand";
assert !eq(FLOTI.MMIXSourceForm, "immediate"),
       "FLOTI must use an immediate source operand";
