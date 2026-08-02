// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the integer opcode allocation from the MMIXware instruction table.
// The MC emitter and byte-level tests are added by later Milestone 2 tasks.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXIntegerRegReg = !instances<MMIXIntRegRegOpcode>();
defvar MMIXIntegerRegImm = !instances<MMIXIntRegImmOpcode>();
defvar MMIXNegReg = !instances<MMIXNegRegOpcode>();
defvar MMIXNegImm = !instances<MMIXNegImmOpcode>();
defvar MMIXIntegerConstants = !instances<MMIXIntConstOpcode>();

assert !eq(!size(MMIXIntegerRegReg), 50),
       "unexpected number of MMIX integer register forms";
assert !eq(!size(MMIXIntegerRegImm), 50),
       "unexpected number of MMIX integer immediate forms";
assert !eq(!size(MMIXNegReg), 2),
       "unexpected number of MMIX register negation forms";
assert !eq(!size(MMIXNegImm), 2),
       "unexpected number of MMIX immediate negation forms";
assert !eq(!size(MMIXIntegerConstants), 16),
       "unexpected number of MMIX integer constant forms";

foreach Record = !listconcat(MMIXIntegerRegReg, MMIXIntegerRegImm,
                             MMIXNegReg, MMIXNegImm,
                             MMIXIntegerConstants) in {
  assert !ne(Record.MMIXOpcode, 0),
         "MMIX integer instruction has no opcode allocation";
}

assert !eq(ADD.MMIXOpcode, 0x20), "ADD opcode is incorrect";
assert !eq(ADDI.MMIXOpcode, 0x21), "ADDI opcode is incorrect";
assert !eq(MUL.MMIXOpcode, 0x18), "MUL opcode is incorrect";
assert !eq(MULI.MMIXOpcode, 0x19), "MULI opcode is incorrect";
assert !eq(TWOADDU.MMIXOpcode, 0x28), "2ADDU opcode is incorrect";
assert !eq(CMP.MMIXOpcode, 0x30), "CMP opcode is incorrect";
assert !eq(NEG.MMIXOpcode, 0x34), "NEG opcode is incorrect";
assert !eq(SL.MMIXOpcode, 0x38), "SL opcode is incorrect";
assert !eq(CSN.MMIXOpcode, 0x60), "CSN opcode is incorrect";
assert !eq(ZSNN.MMIXOpcode, 0x78), "ZSNN opcode is incorrect";
assert !eq(OR.MMIXOpcode, 0xC0), "OR opcode is incorrect";
assert !eq(AND.MMIXOpcode, 0xC8), "AND opcode is incorrect";
assert !eq(MUX.MMIXOpcode, 0xD8), "MUX opcode is incorrect";
assert !eq(SETH.MMIXOpcode, 0xE0), "SETH opcode is incorrect";
assert !eq(ANDNL.MMIXOpcode, 0xEF), "ANDNL opcode is incorrect";
