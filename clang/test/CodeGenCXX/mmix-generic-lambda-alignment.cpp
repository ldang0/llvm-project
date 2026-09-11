// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-obj -o %t.o %s

// CHECK-LABEL: define{{.*}} @_Z5applyi
int apply(int value) {
  auto forward = [](auto &&arg) { return arg; };
  auto local = [](auto arg) {
    decltype(arg) copy = arg;
    return copy;
  };
  return local(forward(value));
}

struct alignas(16) Overaligned { long value; };
long aligned_copy(Overaligned &value) {
  auto copy = [](auto &arg) {
    auto local = arg;
    return local.value;
  };
  return copy(value);
}
// CHECK: alloca %struct.Overaligned, align 16
