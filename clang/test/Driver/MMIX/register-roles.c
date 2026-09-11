// Verify the public C-to-LLVM boundary before checking the target register
// roles selected below.
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -S -emit-llvm \
// RUN:   %S/Inputs/register-roles.c -o %t.ll
// RUN: FileCheck %s --check-prefix=IR < %t.ll

// Verify argument and result roles at instruction selection, before register
// allocation can obscure them.
// RUN: llc -mtriple=mmix-unknown-unknown -stop-after=mmix-isel %t.ll \
// RUN:   -o - | FileCheck %s --check-prefix=MIR

// Verify that ordinary function lowering uses, rather than allocates, the
// dedicated procedure, frame, stack, and scratch registers.
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -S \
// RUN:   %S/Inputs/register-roles.c -o - | FileCheck %s --check-prefix=ASM

// Invalid names and fixed global roles must be rejected through the public
// frontend path.
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -DTEST_INVALID_NAME -fsyntax-only %S/Inputs/register-roles.c 2>&1 \
// RUN:   | FileCheck %s --check-prefix=INVALID
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -DTEST_RESERVED_VALUES -S %S/Inputs/register-roles.c \
// RUN:   -o %t.reserved.s 2>&1 | FileCheck %s --check-prefix=RESERVED
// RUN: not test -s %t.reserved.s

// Function-local inline assembly may neither overwrite machine state roles nor
// perform procedure transitions outside the compiler's call-frame lowering.
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -DTEST_UNSAFE_STATE -S %S/Inputs/register-roles.c \
// RUN:   -o %t.unsafe.s 2>&1 | FileCheck %s --check-prefix=UNSAFE
// RUN: not test -s %t.unsafe.s
// RUN: not %clang --target=mmix -ffreestanding -DTEST_STATE_WRITE -S \
// RUN:   %S/Inputs/register-roles.c -o /dev/null 2>&1 | FileCheck %s --check-prefix=WRITE
// RUN: not %clang --target=mmix -ffreestanding -DTEST_PROCEDURE_CALL -S \
// RUN:   %S/Inputs/register-roles.c -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL

// IR: define dso_local i64 @scalar_roles(i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}}, i64 noundef %{{.*}})
// IR: call i64 @external_many(i64 noundef
// IR: define dso_local void @indirect_result_role(ptr dead_on_unwind noalias writable sret(%struct.Large) align 8
// IR: call void @external_large(ptr dead_on_unwind writable sret(%struct.Large) align 8

// MIR-LABEL: name: scalar_roles
// MIR: liveins: $r231, $r232, $r233, $r234, $r235, $r236, $r237, $r238, $r239, $r240, $r241, $r242, $r243, $r244, $r245, $r246
// MIR: DIRECT_CALL_STATE @external_many
// MIR-SAME: implicit $r231, implicit $r232, implicit $r233, implicit $r234
// MIR-SAME: implicit $r235, implicit $r236, implicit $r237, implicit $r238
// MIR-SAME: implicit $r239, implicit $r240, implicit $r241, implicit $r242
// MIR-SAME: implicit $r243, implicit $r244, implicit $r245, implicit $r246
// MIR-SAME: implicit-def $r254, implicit-def $r231
// MIR-LABEL: name: indirect_result_role
// MIR: liveins: $r251, $r231
// MIR: DIRECT_CALL_STATE @external_large
// MIR-SAME: implicit $r251, implicit $r231, implicit-def $r254

// ASM-LABEL: scalar_roles:
// ASM: GET r30, rJ
// ASM: SUBU r254, r254,
// ASM: STOU r253, r254,
// ASM: ADDU r253, r254,
// ASM: NEGU r255, 0,
// ASM: PUSHGO r31,
// ASM: PUT rJ, r30
// ASM-LABEL: indirect_result_role:
// ASM: OR {{r[0-9]+}}, r251, 0
// ASM: OR r251, {{r[0-9]+}}, 0
// ASM: PUSHGO r31,

// INVALID: error: unknown register name '$256' in asm
// RESERVED-DAG: error: could not allocate output register for constraint '{r32}'
// RESERVED-DAG: error: could not allocate output register for constraint '{r251}'
// RESERVED-DAG: error: could not allocate output register for constraint '{r252}'
// RESERVED-DAG: error: could not allocate output register for constraint '{r253}'
// RESERVED-DAG: error: could not allocate output register for constraint '{r254}'
// RESERVED-DAG: error: could not allocate output register for constraint '{r255}'
// UNSAFE: error: error in backend: MMIX inline assembly may not clobber register 'r254' in an ordinary function
// WRITE: error: error in backend: MMIX instruction 'PUT rJ' is only permitted in module-level inline assembly
// CALL: error: error in backend: MMIX instruction 'PUSHJ' is only permitted in module-level inline assembly
