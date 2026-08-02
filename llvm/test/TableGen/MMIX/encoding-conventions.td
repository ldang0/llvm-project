// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s
// RUN: llvm-tblgen -gen-emitter -I %p/../../../include \
// RUN:   -I %p/../../../lib/Target/MMIX \
// RUN:   %p/../../../lib/Target/MMIX/MMIX.td -o /dev/null
// RUN: llvm-tblgen -gen-disassembler -I %p/../../../include \
// RUN:   -I %p/../../../lib/Target/MMIX \
// RUN:   %p/../../../lib/Target/MMIX/MMIX.td -o /dev/null

// Verify the shared MMIX instruction-word layout and representative operand
// mappings. Running both generators ensures that every architectural record
// has a complete encoding description before either generated implementation
// is integrated into the target.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXInventoryRecords = !instances<MMIXInstruction>();
defvar MMIXNonFourByteRecords =
    !filter(Record, MMIXInventoryRecords, !ne(Record.Size, 4));

assert !eq(!size(MMIXNonFourByteRecords), 0),
       "MMIX instructions must use four-byte encodings";

// Bind representative operands to concrete values so assertions can inspect
// the resulting instruction word rather than unresolved operand variables.
def TestXYZ : MMIXIntRegRegOpcode<0x12, "TESTXYZ"> {
  let dst = 0x34;
  let lhs = 0x56;
  let rhs = 0x78;
}
assert !eq(TestXYZ.Inst, 0x12345678),
       "ordinary operands are not mapped to X, Y, and Z";

def TestIntConst : MMIXIntConstOpcode<0x12, "TESTCONST"> {
  let dst = 0x34;
  let value = 0x5678;
}
assert !eq(TestIntConst.Inst, 0x12345678),
       "sixteen-bit immediate is not mapped to YZ";

def TestBranch : MMIXCondBranchOpcode<0x12, "TESTBR"> {
  let condition = 0x34;
  let target = 0x5678;
}
assert !eq(TestBranch.Inst, 0x12345678),
       "branch target is not mapped to YZ";

def TestJump : MMIXJumpOpcode<0x12, "TESTJMP"> {
  let target = 0x345678;
}
assert !eq(TestJump.Inst, 0x12345678),
       "jump target is not mapped to XYZ";

def TestGetAddress : MMIXGetAddressOpcode<0x12, "TESTGETA"> {
  let dst = 0x34;
  let target = 0x5678;
}
assert !eq(TestGetAddress.Inst, 0x12345678),
       "address-generation target is not mapped to YZ";

def TestRounded : MMIXFPUnaryRoundedOpcode<0x12, "TESTROUND"> {
  let dst = 0x34;
  let round = 4;
  let src = 0x78;
}
assert !eq(TestRounded.Inst, 0x12340478),
       "rounded operand or its fixed bits are incorrect";

def TestPut : MMIXPutRegOpcode<0x12> {
  let dst = 0x14;
  let src = 0x78;
}
assert !eq(TestPut.Inst, 0x12140078),
       "PUT operands or fixed Y field are incorrect";

def TestGet : MMIXGet {
  let MMIXOpcode = 0x12;
  let dst = 0x34;
  let src = 0x14;
}
assert !eq(TestGet.Inst, 0x12340014),
       "GET operands or fixed Y field are incorrect";

def TestResume : MMIXResume {
  let MMIXOpcode = 0x12;
  let mode = 1;
}
assert !eq(TestResume.Inst, 0x12000001),
       "RESUME mode or reserved fields are incorrect";

def TestSync : MMIXSync {
  let MMIXOpcode = 0x12;
  let mode = 7;
}
assert !eq(TestSync.Inst, 0x12000007),
       "SYNC mode or reserved fields are incorrect";

def TestSave : MMIXSave {
  let MMIXOpcode = 0x12;
  let state = 0x34;
}
assert !eq(TestSave.Inst, 0x12340000),
       "SAVE state or reserved fields are incorrect";

def TestUnsave : MMIXUnsave {
  let MMIXOpcode = 0x12;
  let state = 0x34;
}
assert !eq(TestUnsave.Inst, 0x12000034),
       "UNSAVE state or reserved fields are incorrect";
