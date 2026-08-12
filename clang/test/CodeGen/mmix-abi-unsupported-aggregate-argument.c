// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DDEFINE %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=FORMAL-ERR
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DCALL %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=CALL-ERR

struct __attribute__((aligned(16))) OverAligned {
  long words[2];
};

void unsupported(struct OverAligned value);

#ifdef DEFINE
void unsupported(struct OverAligned value) {}
#endif

#ifdef CALL
void call_unsupported(struct OverAligned *value) { unsupported(*value); }
#endif

// FORMAL-ERR: error: MMIX GNU ABI does not support over-aligned aggregate argument type 'struct OverAligned'
// CALL-ERR: error: MMIX GNU ABI does not support over-aligned aggregate argument type 'struct OverAligned'
