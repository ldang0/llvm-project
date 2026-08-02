// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the MMIXware control-flow and register-stack opcode allocation.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXBranches = !instances<MMIXCondBranchOpcode>();

assert !eq(!size(MMIXBranches), 32),
       "unexpected MMIX conditional branch forms";
assert !eq(!size(!instances<MMIXJumpOpcode>()), 2),
       "unexpected MMIX jump forms";
assert !eq(!size(!instances<MMIXGetAddressOpcode>()), 2),
       "unexpected MMIX address-generation forms";
assert !eq(!size(!instances<MMIXPushJumpOpcode>()), 2),
       "unexpected MMIX push-jump forms";
assert !eq(!size(!instances<MMIXGoRegOpcode>()), 1),
       "unexpected MMIX GO register forms";
assert !eq(!size(!instances<MMIXGoImmOpcode>()), 1),
       "unexpected MMIX GO immediate forms";
assert !eq(!size(!instances<MMIXPushGoRegOpcode>()), 1),
       "unexpected MMIX PUSHGO register forms";
assert !eq(!size(!instances<MMIXPushGoImmOpcode>()), 1),
       "unexpected MMIX PUSHGO immediate forms";

foreach Record = !listconcat(MMIXBranches,
                             !instances<MMIXJumpOpcode>(),
                             !instances<MMIXGetAddressOpcode>(),
                             !instances<MMIXPushJumpOpcode>(),
                             !instances<MMIXGoRegOpcode>(),
                             !instances<MMIXGoImmOpcode>(),
                             !instances<MMIXPushGoRegOpcode>(),
                             !instances<MMIXPushGoImmOpcode>(),
                             !instances<MMIXPop>(), !instances<MMIXSave>(),
                             !instances<MMIXUnsave>()) in {
  assert !ne(Record.MMIXOpcode, 0),
         "MMIX control-flow instruction has no opcode allocation";
}

assert !eq(BN.MMIXOpcode, 0x40), "BN opcode is incorrect";
assert !eq(BNB.MMIXOpcode, 0x41), "BNB opcode is incorrect";
assert !eq(PBNN.MMIXOpcode, 0x58), "PBNN opcode is incorrect";
assert !eq(PBEVB.MMIXOpcode, 0x5F), "PBEVB opcode is incorrect";
assert !eq(GO.MMIXOpcode, 0x9E), "GO opcode is incorrect";
assert !eq(PUSHGOI.MMIXOpcode, 0xBF), "PUSHGOI opcode is incorrect";
assert !eq(JMP.MMIXOpcode, 0xF0), "JMP opcode is incorrect";
assert !eq(PUSHJ.MMIXOpcode, 0xF2), "PUSHJ opcode is incorrect";
assert !eq(GETA.MMIXOpcode, 0xF4), "GETA opcode is incorrect";
assert !eq(POP.MMIXOpcode, 0xF8), "POP opcode is incorrect";
assert !eq(SAVE.MMIXOpcode, 0xFA), "SAVE opcode is incorrect";
assert !eq(UNSAVE.MMIXOpcode, 0xFB), "UNSAVE opcode is incorrect";

assert !eq(MMIXBranchTarget.MMIXEncodingKind, "pc-relative"),
       "branch targets must be PC-relative";
assert !eq(MMIXBranchTarget.MMIXEncodingWidth, 16),
       "branch targets must use a 16-bit displacement";
assert !eq(MMIXJumpTarget.MMIXEncodingWidth, 24),
       "jump targets must use a 24-bit displacement";
assert !eq(MMIXBranchTarget.MMIXFixupKind, "pc-relative"),
       "branch targets must carry PC-relative fixup metadata";
assert !eq(MMIXBranchTarget.MMIXFixupScale, 4),
       "branch targets must use instruction-word scaling";
assert !eq(MMIXBranchTarget.DecoderMethod, "decodePCRelativeOperand"),
       "branch targets must use the MMIX PC-relative decoder";
assert !eq(MMIXJumpTarget.DecoderMethod, "decodePCRelativeOperand"),
       "jump targets must use the MMIX PC-relative decoder";

assert !eq(BN.MMIXPCRelativeWidth, 16),
       "forward branches must carry a 16-bit displacement width";
assert !eq(BNB.MMIXPCRelativeWidth, 16),
       "backward branches must carry a 16-bit displacement width";
assert !eq(BN.MMIXPCRelativeBackward, false),
       "BN must use a forward displacement";
assert !eq(BNB.MMIXPCRelativeBackward, true),
       "BNB must use a backward displacement";
assert !eq(JMP.MMIXPCRelativeWidth, 24),
       "jumps must carry a 24-bit displacement width";
assert !eq(JMPB.MMIXPCRelativeBackward, true),
       "JMPB must use a backward displacement";
assert !eq(GETA.MMIXPCRelativeWidth, 16),
       "GETA must carry a 16-bit displacement width";
assert !eq(GETAB.MMIXPCRelativeBackward, true),
       "GETAB must use a backward displacement";
