// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_MEMBER_POINTER %s 2>&1 | FileCheck %s --check-prefix=MEMBER
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VIRTUAL_DISPATCH %s 2>&1 | FileCheck %s --check-prefix=DISPATCH
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_OBJECT_LIFETIME %s 2>&1 | FileCheck %s --check-prefix=LIFETIME
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_DYNAMIC_INITIALIZATION %s 2>&1 | FileCheck %s --check-prefix=INIT

#if defined(TEST_MEMBER_POINTER)
struct Owner {
  long value;
};
long read(Owner *object, long Owner::*member) { return object->*member; }
// MEMBER: error: MMIX GNU ABI does not support argument type 'long Owner::*'
#elif defined(TEST_VIRTUAL_DISPATCH)
struct Base {
  virtual long value();
};
long dispatch(Base *object) { return object->value(); }
// DISPATCH: error: MMIX C++ producer profile does not support virtual dispatch
#elif defined(TEST_OBJECT_LIFETIME)
struct Base {
  virtual ~Base();
};
void construct() { Base object; }
// LIFETIME: error: MMIX C++ producer profile does not support polymorphic object lifetime
#elif defined(TEST_DYNAMIC_INITIALIZATION)
extern long make_value();
long value = make_value();
// INIT: error: MMIX C++ producer profile does not support dynamic initialization
#endif
