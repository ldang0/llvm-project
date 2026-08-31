// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=IR
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_QUALIFIER %s 2>&1 | FileCheck %s --check-prefix=QUALIFIER
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_PRVALUE_REFERENCE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PRVALUE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ADDRESS_SPACE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ADDRESS-SPACE

struct Source {
  long Value;
};

struct Target {
  unsigned long Value;
};

#if defined(TEST_QUALIFIER)
unsigned char *drop_const(const Source *Value) {
  return reinterpret_cast<unsigned char *>(Value);
}
// QUALIFIER: error: reinterpret_cast from 'const Source *' to 'unsigned char *' casts away qualifiers
#elif defined(TEST_PRVALUE_REFERENCE)
Target &bind_prvalue() { return reinterpret_cast<Target &>(Source{1}); }
// PRVALUE: error: reinterpret_cast from rvalue to reference type 'Target &'
#elif defined(TEST_ADDRESS_SPACE)
using AS1Source = Source __attribute__((address_space(1)));
Target *leave_address_space(AS1Source *Value) {
  return reinterpret_cast<Target *>(Value);
}
// ADDRESS-SPACE: error: reinterpret_cast from 'AS1Source *' {{.*}}to 'Target *' is not allowed
#else
void *to_void(Source *Value) { return reinterpret_cast<void *>(Value); }

char *to_character(Source *Value) { return reinterpret_cast<char *>(Value); }

const unsigned char *to_const_character(const Source *Value) {
  return reinterpret_cast<const unsigned char *>(Value);
}

Target *to_unrelated(Source *Value) {
  return reinterpret_cast<Target *>(Value);
}

Target &to_reference(Source &Value) {
  return reinterpret_cast<Target &>(Value);
}

Target &&to_rvalue_reference(Source &&Value) {
  return reinterpret_cast<Target &&>(Value);
}

// IR-LABEL: define {{.*}} @_Z7to_voidP6Source(
// IR: ret ptr {{%.*}}
// IR-LABEL: define {{.*}} @_Z12to_characterP6Source(
// IR: ret ptr {{%.*}}
// IR-LABEL: define {{.*}} @_Z18to_const_characterPK6Source(
// IR: ret ptr {{%.*}}
// IR-LABEL: define {{.*}} @_Z12to_unrelatedP6Source(
// IR: ret ptr {{%.*}}
// IR-LABEL: define {{.*}} @_Z12to_referenceR6Source(
// IR: ret ptr {{%.*}}
// IR-LABEL: define {{.*}} @_Z19to_rvalue_referenceO6Source(
// IR: ret ptr {{%.*}}
#endif
