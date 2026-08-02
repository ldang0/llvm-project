// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the operand metadata used by future MC encoding and validation code.
// These assertions do not require parser or emitter implementations.
include "../../../lib/Target/MMIX/MMIX.td"

assert !eq(GPR64Op.MMIXEncodingWidth, 8),
       "general-purpose registers must use eight-bit fields";
assert !eq(FPR64Op.MMIXEncodingWidth, 8),
       "floating-point registers must use eight-bit fields";
assert !eq(SPR64Op.MMIXEncodingWidth, 5),
       "special registers must use five-bit fields";

assert !eq(MMIXUImm8.MMIXEncodingWidth, 8),
       "MMIX eight-bit immediate metadata is incorrect";
assert !eq(MMIXUImm8.MMIXEncodingSigned, false),
       "MMIX eight-bit immediate must be unsigned";
assert !eq(MMIXUImm16.MMIXEncodingWidth, 16),
       "MMIX sixteen-bit immediate metadata is incorrect";
assert !eq(MMIXUImm24.MMIXEncodingWidth, 24),
       "MMIX twenty-four-bit immediate metadata is incorrect";
assert !eq(MMIXRoundingMode.MMIXEncodingWidth, 3),
       "MMIX rounding modes must use three-bit fields";
assert !eq(MMIXResumeMode.MMIXEncodingWidth, 1),
       "MMIX resume modes must use a one-bit field";
assert !eq(MMIXSyncMode.MMIXEncodingWidth, 3),
       "MMIX synchronization modes must use a three-bit field";

assert !eq(MMIXBranchTarget.MMIXEncodingKind, "pc-relative"),
       "MMIX branch targets must be PC-relative";
assert !eq(MMIXBranchTarget.MMIXEncodingRange,
           "signed-16-instruction-words"),
       "MMIX branch target range metadata is incorrect";
assert !eq(MMIXJumpTarget.MMIXEncodingRange,
           "signed-24-instruction-words"),
       "MMIX jump target range metadata is incorrect";
assert !eq(MMIXMemOffset8.MMIXEncodingKind, "memory-offset"),
       "MMIX memory offsets must have a dedicated operand kind";
assert !eq(MMIXRegOrImm8.MMIXEncodingKind, "register-or-immediate"),
       "MMIX mixed register/immediate metadata is incorrect";
