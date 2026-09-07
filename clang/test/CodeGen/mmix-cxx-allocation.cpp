// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -fno-rtti -mrelocation-model static \
// RUN:   -emit-llvm -O0 -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -fno-rtti -mrelocation-model static \
// RUN:   -emit-obj -O2 -o %t.user.o %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 \
// RUN:   -ffreestanding -mrelocation-model static -I %S/../../../compiler-rt/lib/builtins \
// RUN:   -emit-obj -O2 -o %t.new.o \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_new.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 \
// RUN:   -ffreestanding -mrelocation-model static \
// RUN:   -emit-obj -O2 -o %t.delete.o \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_delete.c
// RUN: llvm-ar rc %t.runtime.a %t.new.o %t.delete.o
// RUN: llvm-nm --defined-only %t.runtime.a | FileCheck %s --check-prefix=RUNTIME
// RUN: ld.lld -m elf64mmix -r -o %t.linked.o %t.user.o %t.runtime.a
// RUN: llvm-nm --undefined-only %t.linked.o | FileCheck %s --check-prefix=DEPS

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

// RUNTIME-DAG: W _ZdaPv
// RUNTIME-DAG: W _ZdaPvm
// RUNTIME-DAG: W _ZdlPv
// RUNTIME-DAG: W _ZdlPvm
// RUNTIME-DAG: W _Znam
// RUNTIME-DAG: W _Znwm

// DEPS-DAG: U abort
// DEPS-DAG: U free
// DEPS-DAG: U malloc
// DEPS-NOT: U _Z
