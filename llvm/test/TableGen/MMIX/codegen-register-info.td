// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the semantic register roles required by the frozen GNU C ABI.  The
// exact allocatable member list excludes the unstable globals $32-$230 and the
// fixed result, static-chain, frame, stack, and scratch roles $251-$255 without
// relying on how the register classes inherit from one another.
include "../../../lib/Target/MMIX/MMIX.td"

// Shared physical registers must remain type-compatible after class
// intersection, irrespective of which equivalent class TableGen selects.
foreach RC = [GPR64, FPR64, GPR64CodeGen, FPR64CodeGen, F32BitsCodeGen] in {
  assert !empty(!listremove([i64, f64, f32], RC.RegTypes)),
         "shared scalar register views must accept all three scalar types";
  assert !eq(RC.Size, 64), "scalar register spills must preserve a full octa";
  assert !eq(RC.Alignment, 64), "scalar register spills must be octa aligned";
}

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
