// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-obj -o %t.o %s
using size_t = __SIZE_TYPE__;
template <class T> struct Cursor {
  T value;
  Cursor(T v) : value(v) {}
  static void *operator new(size_t, Cursor *storage) { return storage; }
};
extern "C" void construct(Cursor<long> *storage, long value) {
  new (storage) Cursor<long>(value);
}
// CHECK-LABEL: define{{.*}} @construct(
// CHECK: call{{.*}}ptr @_ZN6CursorIlEnwEmPS0_(
// CHECK: call void @_ZN6CursorIlEC1El(
