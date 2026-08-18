// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DALIGN_DEFINE %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=ALIGNDEF
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DALIGN_CALL %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=ALIGNCALL

struct __attribute__((aligned(16))) OverAligned {
  long values[3];
};

struct OverAligned aligned_result(void);

#ifdef ALIGN_DEFINE
struct OverAligned aligned_result(void) { return (struct OverAligned){0}; }
#endif

#ifdef ALIGN_CALL
void call_aligned(void) { (void)aligned_result(); }
#endif

// ALIGNDEF: error: MMIX GNU ABI does not support over-aligned aggregate return type 'struct OverAligned'
// ALIGNCALL: error: MMIX GNU ABI does not support over-aligned aggregate return type 'struct OverAligned'
