// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -fno-rtti -mrelocation-model static \
// RUN:   -emit-llvm -O0 -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -fno-rtti -mrelocation-model static \
// RUN:   -emit-obj -O2 -o %t.user.o %s
// RUN: llvm-nm --undefined-only %t.user.o | FileCheck %s --check-prefix=DEPS

using size_t = decltype(sizeof(0));
void *operator new(size_t);
void *operator new[](size_t);
void operator delete(void *) noexcept;
void operator delete[](void *) noexcept;
void operator delete(void *, size_t) noexcept;
void operator delete[](void *, size_t) noexcept;

struct Value {
  long field;
};

Value *allocate_scalar() { return new Value{1}; }
Value *allocate_array(size_t count) { return new Value[count]; }
void release_scalar(Value *value) { delete value; }
void release_array(Value *value) { delete[] value; }

// IR-DAG: call noalias noundef nonnull ptr @_Znwm(i64 noundef 8)
// IR-DAG: call noalias noundef nonnull ptr @_Znam(i64 noundef
// IR-DAG: call void @_ZdlPvm(ptr noundef
// IR-DAG: call void @_ZdaPv(ptr noundef

// DEPS-DAG: U _Znwm
// DEPS-DAG: U _Znam
// DEPS-DAG: U _ZdlPvm
// DEPS-DAG: U _ZdaPv
