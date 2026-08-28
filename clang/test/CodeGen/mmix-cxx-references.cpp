// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=COMMON,O0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=COMMON,O2
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VECTOR_REFERENCE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=VECTOR-REFERENCE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_POLYMORPHIC_CONSTRUCTION %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=POLYMORPHIC-CONSTRUCTION

#if defined(TEST_VECTOR_REFERENCE)
using int2 = int __attribute__((ext_vector_type(2)));

int first(int2 &Value) { return Value[0]; }
// VECTOR-REFERENCE: error: MMIX GNU ABI does not support vector value CodeGen involving type 'int2 &'
#elif defined(TEST_POLYMORPHIC_CONSTRUCTION)
struct Polymorphic {
  Polymorphic();
  virtual long value() const;
};

void construct() { Polymorphic Value; }
// POLYMORPHIC-CONSTRUCTION: error: MMIX C++ producer profile does not support polymorphic object lifetime
#else
struct Counter {
  long Value;

  long add(long Delta) { return Value + Delta; }
};

long update(long &Value, long Delta) {
  Value += Delta;
  return Value;
}

long consume(long &&Value) { return Value; }

using nullptr_t = decltype(nullptr);

bool is_null(nullptr_t Value) { return Value == nullptr; }

extern "C" long reference_entry(Counter *Object, long *Value, long Delta) {
  return Object->add(Delta) + update(*Value, Delta) +
         consume(static_cast<long &&>(*Value)) + is_null(nullptr);
}

// COMMON-LABEL: define dso_local noundef i64 @_Z6updateRll(ptr {{[^,]*}}noundef nonnull {{[^,]*}}%Value,
// COMMON-LABEL: define dso_local noundef i64 @_Z7consumeOl(ptr {{[^,]*}}noundef nonnull {{[^,]*}}%Value)
// COMMON-LABEL: define dso_local noundef i1 @_Z7is_nullDn(ptr {{[^,]*}}%Value)
// COMMON-LABEL: define dso_local {{.*}}i64 @reference_entry(
// COMMON-SAME: ptr {{[^,]*}}noundef{{[^,]*}} %Object,
// COMMON-SAME: ptr {{[^,]*}}noundef{{[^,]*}} %Value,
// COMMON-SAME: i64 noundef %Delta)
// O0: call noundef i64 {{.*}}(ptr noundef nonnull align 8 dereferenceable(8)
// O0: call noundef i64 {{.*}}(ptr noundef nonnull align 8 dereferenceable(8)
// O0: call noundef i1 {{.*}}(ptr null)
// O2: ret i64
#endif
