// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=IR \
// RUN:   --implicit-check-not=llvm.global_ctors
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=IR \
// RUN:   --implicit-check-not=llvm.global_ctors
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t.o %s
// RUN: llvm-readobj --sections --relocations %t.o \
// RUN:   | FileCheck %s --check-prefix=ELF \
// RUN:   --implicit-check-not=.init_array
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_DYNAMIC_INITIALIZER %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DYNAMIC

struct Source {
  long first;
  long second;
};

struct Target {
  unsigned long value;
};

using Unary = long (*)(long);
using Other = unsigned long (*)(unsigned long);

#if defined(TEST_DYNAMIC_INITIALIZER)
extern Target *make_pointer();
Target *dynamic_pointer = make_pointer();
// DYNAMIC: error: MMIX C++ producer profile does not support dynamic initialization
#else
extern Source external_source;
extern const Source external_const_source;
extern long external_function(long);

Target *object_pointer = reinterpret_cast<Target *>(&external_source);
const Target *const_pointer =
    reinterpret_cast<const Target *>(&external_const_source);
char *object_addend = reinterpret_cast<char *>(&external_source) + 8;
Other function_pointer = reinterpret_cast<Other>(&external_function);
Target *null_pointer = reinterpret_cast<Target *>(0);

// IR: @object_pointer = {{.*}}global ptr @external_source
// IR: @const_pointer = {{.*}}global ptr @external_const_source
// IR: @object_addend = {{.*}}global ptr getelementptr{{.*}}(i8, ptr @external_source, i64 8)
// IR: @function_pointer = {{.*}}global ptr @_Z17external_functionl
// IR: @null_pointer = {{.*}}global ptr null

// ELF: Name: .rela.data
// ELF: 0x0 R_MMIX_64 external_source 0x0
// ELF-NEXT: 0x8 R_MMIX_64 external_const_source 0x0
// ELF-NEXT: 0x10 R_MMIX_64 external_source 0x8
// ELF-NEXT: 0x18 R_MMIX_64 _Z17external_functionl 0x0
#endif
