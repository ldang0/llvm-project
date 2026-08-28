// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=SUPPORTED
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VIRTUAL %s 2>&1 | FileCheck %s --check-prefix=VIRTUAL
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_RTTI %s 2>&1 | FileCheck %s --check-prefix=RTTI
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_DYNAMIC_INIT %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DYNAMIC-INIT

#if defined(TEST_VIRTUAL)
struct Polymorphic {
  virtual long value() const;
};

long read(Polymorphic *object) { return object->value(); }
// VIRTUAL: error: MMIX C++ producer profile does not support virtual dispatch
#elif defined(TEST_RTTI)
struct Base {
  virtual ~Base();
};

void *runtime_type(Base *object) { return dynamic_cast<void *>(object); }
// RTTI: error: MMIX C++ producer profile does not support RTTI
#elif defined(TEST_DYNAMIC_INIT)
extern int make_value();

int read_initialized() {
  static int value = make_value();
  return value;
}
// DYNAMIC-INIT: error: MMIX C++ producer profile does not support dynamic local initialization
#else
void empty() {}

long increment(long value) { return value + 1; }

extern "C" int add(int lhs, int rhs) { return lhs + rhs; }

// SUPPORTED-LABEL: define dso_local void @_Z5emptyv()
// SUPPORTED-LABEL: define dso_local noundef i64 @_Z9incrementl(i64 noundef %value)
// SUPPORTED-LABEL: define dso_local i32 @add(i32 noundef signext %lhs, i32 noundef signext %rhs)
#endif
