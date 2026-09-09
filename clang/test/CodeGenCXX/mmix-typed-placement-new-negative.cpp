// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -DCASE=0 -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s
// RUN: not %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -DCASE=1 -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s
// RUN: not %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -DCASE=2 -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s
using size_t = __SIZE_TYPE__;
struct Array {
  static void *operator new[](size_t, Array *);
};
struct Untyped {
  static void *operator new(size_t, void *);
};
struct Different {
  static void *operator new(size_t, Array *);
};
#if CASE == 0
void array(Array *p) { new (p) Array[2]; }
#elif CASE == 1
void untyped(void *p) { new (p) Untyped; }
#else
void different(Array *p) { new (p) Different; }
#endif
// CHECK: error: MMIX does not support C++ allocation form
