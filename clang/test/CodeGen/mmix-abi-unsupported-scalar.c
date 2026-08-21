// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -emit-llvm -o /dev/null \
// RUN:   -DTEST_INT128 %s 2>&1 | FileCheck %s --check-prefix=INT128
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -emit-llvm -o /dev/null \
// RUN:   -DTEST_COMPLEX %s 2>&1 | FileCheck %s --check-prefix=COMPLEX
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -emit-llvm -o /dev/null \
// RUN:   -DTEST_VARIADIC_COMPLEX %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=VARIADIC-COMPLEX
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -emit-llvm -o /dev/null \
// RUN:   -DTEST_VECTOR %s 2>&1 | FileCheck %s --check-prefix=VECTOR
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -emit-llvm -o /dev/null \
// RUN:   -DTEST_ADDRESS_SPACE %s 2>&1 | FileCheck %s --check-prefix=AS
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -emit-llvm -o /dev/null \
// RUN:   -DTEST_INDIRECT_CALL %s 2>&1 | FileCheck %s --check-prefix=CALL

#if defined(TEST_INT128)
__int128 unsupported(__int128 value) { return value; }
// INT128-COUNT-2: error: __int128 is not supported on this target
#elif defined(TEST_COMPLEX)
_Complex int unsupported(_Complex int value) { return value; }
// COMPLEX: error: MMIX GNU ABI does not support return type '_Complex int'
// COMPLEX: error: MMIX GNU ABI does not support argument type '_Complex int'
#elif defined(TEST_VARIADIC_COMPLEX)
extern void variadic_sink(int, ...);
void unsupported(_Complex int value) { variadic_sink(0, value); }
// VARIADIC-COMPLEX: error: MMIX GNU ABI does not support argument type '_Complex int'
#elif defined(TEST_VECTOR)
typedef int int2 __attribute__((ext_vector_type(2)));
int2 unsupported(int2 value) { return value; }
// VECTOR: error: MMIX GNU ABI does not support return type 'int2'
// VECTOR: error: MMIX GNU ABI does not support argument type 'int2'
#elif defined(TEST_ADDRESS_SPACE)
typedef int __attribute__((address_space(1))) as1_int;
as1_int *unsupported(as1_int *value) { return value; }
// AS: error: MMIX GNU ABI does not support return type 'as1_int *'
// AS: error: MMIX GNU ABI does not support argument type 'as1_int *'
#elif defined(TEST_INDIRECT_CALL)
typedef __int128 (*unsupported_function)(__int128);
void call_unsupported(unsupported_function fn) { (void)fn(0); }
// CALL-COUNT-2: error: __int128 is not supported on this target
#endif
