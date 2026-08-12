// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ATOMIC_VALUE %s 2>&1 | FileCheck %s --check-prefix=ATOMIC
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ATOMIC_BOUNDARY %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ATOMIC-BOUNDARY
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ATOMIC_BUILTIN %s 2>&1 | FileCheck %s --check-prefix=BUILTIN
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VECTOR %s 2>&1 | FileCheck %s --check-prefix=VECTOR
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_WIDE_OPERATION %s 2>&1 | FileCheck %s --check-prefix=WIDE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ADDRESS_SPACE %s 2>&1 | FileCheck %s --check-prefix=AS
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_NAKED %s 2>&1 | FileCheck %s --check-prefix=NAKED
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_TARGET %s 2>&1 | FileCheck %s --check-prefix=TARGET
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -o - \
// RUN:   -DTEST_SUPPORTED_BUILTINS %s | FileCheck %s --check-prefix=SUPPORTED

#if defined(TEST_ATOMIC_VALUE)
_Atomic int value;
// ATOMIC: error: MMIX GNU ABI does not support atomic value CodeGen involving type '_Atomic(int)'
#elif defined(TEST_ATOMIC_BOUNDARY)
int consume(_Atomic int value) { return 0; }
// ATOMIC-BOUNDARY: error: MMIX GNU ABI does not support atomic value CodeGen involving type '_Atomic(int)'
#elif defined(TEST_ATOMIC_BUILTIN)
int load(int *value) { return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
// BUILTIN: error: MMIX GNU ABI does not support atomic operation CodeGen
#elif defined(TEST_VECTOR)
typedef int int2 __attribute__((ext_vector_type(2)));
int2 value;
// VECTOR: error: MMIX GNU ABI does not support vector value CodeGen involving type 'int2'
#elif defined(TEST_WIDE_OPERATION)
long multiply(long value) {
  __int128 wide = value;
  wide *= wide;
  return (long)wide;
}
// WIDE: error: MMIX GNU ABI does not support extended scalar operation CodeGen involving type '__int128'
#elif defined(TEST_ADDRESS_SPACE)
int __attribute__((address_space(1))) value;
// AS: error: MMIX GNU ABI does not support nonzero-address-space value CodeGen involving type '__attribute__((address_space(1))) int'
#elif defined(TEST_NAKED)
__attribute__((naked)) void unsupported(void) {}
// NAKED: error: MMIX does not support the 'naked' function attribute
#elif defined(TEST_TARGET)
__attribute__((target("base"))) void unsupported(void) {}
// TARGET: error: MMIX does not support the 'target' function attribute
#elif defined(TEST_SUPPORTED_BUILTINS)
__int128 wide_object;

void copy_eight(char *destination, const char *source) {
  __builtin_memcpy(destination, source, 8);
}

double fused(double a, double b, double c) {
  return __builtin_fma(a, b, c);
}

// SUPPORTED: @wide_object ={{.*}} global i128 0, align 8
// SUPPORTED-LABEL: define dso_local void @copy_eight(
// SUPPORTED: call void @llvm.memcpy
// SUPPORTED-LABEL: define dso_local double @fused(
// SUPPORTED: call double @llvm.fma.f64(
#endif
