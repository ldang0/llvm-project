// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=IR

using uintptr_t = __UINTPTR_TYPE__;
using Unary = long (*)(long);
using Other = unsigned long (*)(unsigned long);

struct MemberOwner {
  long value;
  long method(long);
};

using MemberFunction = long (MemberOwner::*)(long);

MemberFunction convert_member(MemberFunction value) {
  return reinterpret_cast<MemberFunction>(value);
}

Other to_other_function(Unary value) {
  return reinterpret_cast<Other>(value);
}

Unary from_other_function(Other value) {
  return reinterpret_cast<Unary>(value);
}

uintptr_t function_to_integer(Unary value) {
  return reinterpret_cast<uintptr_t>(value);
}

Unary integer_to_function(uintptr_t value) {
  return reinterpret_cast<Unary>(value);
}

long call_after_type_round_trip(Unary value, long argument) {
  Other converted = to_other_function(value);
  Unary restored = from_other_function(converted);
  return restored(argument);
}

long call_after_integer_round_trip(Unary value, long argument) {
  Unary restored = integer_to_function(function_to_integer(value));
  return restored(argument);
}

// IR-LABEL: define {{.*}}void @_Z14convert_memberM11MemberOwnerFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}%agg.result, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// IR-LABEL: define {{.*}} @_Z17to_other_functionPFllE(
// IR-LABEL: define {{.*}} @_Z19from_other_functionPFmmE
// IR-LABEL: define {{.*}} @_Z19function_to_integerPFllE(
// IR: ptrtoint ptr {{%.*}} to i64
// IR-LABEL: define {{.*}} @_Z19integer_to_functionm(
// IR: inttoptr i64 {{%.*}} to ptr
// IR-LABEL: define {{.*}} @_Z26call_after_type_round_tripPFllEl(
// IR: call {{.*}}i64 %{{.*}}(i64 {{.*}})
// IR-LABEL: define {{.*}} @_Z29call_after_integer_round_tripPFllEl(
// IR: call {{.*}}i64 %{{.*}}(i64 {{.*}})
