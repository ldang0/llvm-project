// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the shared MMIX instruction-word layout. Concrete opcode and
// operand-field assignments are covered by later MC inventory tasks.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXInventoryRecords = !instances<MMIXInstruction>();
defvar MMIXNonFourByteRecords =
    !filter(Record, MMIXInventoryRecords, !ne(Record.Size, 4));

assert !eq(!size(MMIXNonFourByteRecords), 0),
       "MMIX instructions must use four-byte encodings";

foreach Record = MMIXInventoryRecords in {
  assert !eq(Record.Inst{31-24}, Record.MMIXOpcode),
         "MMIX opcode is not mapped to Inst{31-24}";
  assert !eq(Record.Inst{23-16}, Record.MMIXX),
         "MMIX X field is not mapped to Inst{23-16}";
  assert !eq(Record.Inst{15-8}, Record.MMIXY),
         "MMIX Y field is not mapped to Inst{15-8}";
  assert !eq(Record.Inst{7-0}, Record.MMIXZ),
         "MMIX Z field is not mapped to Inst{7-0}";
}
