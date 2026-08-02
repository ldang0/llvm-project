// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify that the CodeGen register classes and preservation records implement
// the provisional ABI without narrowing the architectural MC register class.
include "../../../lib/Target/MMIX/MMIX.td"

assert !eq(!repr(GPR64.MemberList), "(sequence \"R%u\", 0, 255)"),
       "the architectural general-purpose register class changed";
assert !eq(!repr(GPR64CodeGen.MemberList),
           "(add (sequence \"R%u\", 250, 231), (sequence \"R%u\", 0, 30))"),
       "the CodeGen integer allocation order does not match the ABI";
assert !eq(!repr(FPR64CodeGen.MemberList),
           "(add (sequence \"R%u\", 250, 231), (sequence \"R%u\", 0, 30))"),
       "the CodeGen floating-point allocation order does not match the ABI";

assert !eq(!repr(CSR_MMIX.SaveList),
           "(add (sequence \"R%u\", 0, 30), R253)"),
       "the MMIX callee-save list does not match the ABI";
assert !eq(!repr(CSR_MMIX.OtherPreserved), "(add R254, RG, RL, RO)"),
       "the MMIX call-preserved state does not match the ABI";
