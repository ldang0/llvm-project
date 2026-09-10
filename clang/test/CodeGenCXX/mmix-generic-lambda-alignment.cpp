// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -DINVALID -verify -o /dev/null %s

// CHECK-LABEL: define{{.*}} @_Z5applyi
int apply(int value) {
  auto forward = [](auto &&arg) { return arg; };
  auto local = [](auto arg) {
    decltype(arg) copy = arg;
    return copy;
  };
  return local(forward(value));
}

#ifdef INVALID
struct alignas(16) Overaligned { long value; };
void invalid(Overaligned &value) {
  auto copy = [](auto &arg) {
    auto local = arg; // expected-error {{MMIX does not support automatic object alignment greater than 8 bytes}}
    return local.value;
  };
  copy(value);
}
#endif
