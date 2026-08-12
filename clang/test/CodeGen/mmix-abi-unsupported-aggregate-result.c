// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DWIDE_DEFINE %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=WIDEDEF
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DWIDE_CALL %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=WIDECALL
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DALIGN_DEFINE %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=ALIGNDEF
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o /dev/null -DALIGN_CALL %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=ALIGNCALL

struct Pair {
  long first;
  long second;
};
struct __attribute__((aligned(16))) OverAligned {
  long values[3];
};

struct Pair wide_result(void);
struct OverAligned aligned_result(void);

#ifdef WIDE_DEFINE
struct Pair wide_result(void) { return (struct Pair){0}; }
#endif

#ifdef WIDE_CALL
void call_wide(void) { (void)wide_result(); }
#endif

#ifdef ALIGN_DEFINE
struct OverAligned aligned_result(void) { return (struct OverAligned){0}; }
#endif

#ifdef ALIGN_CALL
void call_aligned(void) { (void)aligned_result(); }
#endif

// WIDEDEF: error: MMIX GNU ABI does not support wide scalar-mode aggregate return type 'struct Pair'
// WIDECALL: error: MMIX GNU ABI does not support wide scalar-mode aggregate return type 'struct Pair'
// ALIGNDEF: error: MMIX GNU ABI does not support over-aligned aggregate return type 'struct OverAligned'
// ALIGNCALL: error: MMIX GNU ABI does not support over-aligned aggregate return type 'struct OverAligned'
