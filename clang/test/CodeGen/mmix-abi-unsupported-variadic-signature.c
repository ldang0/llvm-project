// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu17 \
// RUN:   -emit-llvm -o /dev/null -DDEFINE %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=FORMAL-ERR
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu17 \
// RUN:   -emit-llvm -o /dev/null -DCALL %s 2>&1 | FileCheck %s \
// RUN:   --check-prefix=CALL-ERR

struct Empty {};

void unsupported(struct Empty final_named, ...);

#ifdef DEFINE
void unsupported(struct Empty final_named, ...) {}
#endif

#ifdef CALL
void call_unsupported(void) { unsupported((struct Empty){}, 1); }
#endif

// FORMAL-ERR: error: MMIX GNU ABI does not support an empty final named parameter in a variadic function
// CALL-ERR: error: MMIX GNU ABI does not support an empty final named parameter in a variadic function
