// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/producer.ll %t/producer.cpp
// RUN: FileCheck %s --check-prefix=PRODUCER < %t/producer.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/consumer.ll %t/consumer.cpp
// RUN: FileCheck %s --check-prefix=CONSUMER < %t/consumer.ll
// RUN: llvm-link %t/producer.ll %t/consumer.ll -o %t/linked.bc
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj \
// RUN:   %t/linked.bc -o /dev/null
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/producer-opt.ll %t/producer.cpp
// RUN: FileCheck %s --check-prefix=OPT-PRODUCER < %t/producer-opt.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/consumer-opt.ll %t/consumer.cpp
// RUN: FileCheck %s --check-prefix=OPT-CONSUMER < %t/consumer-opt.ll
// RUN: llvm-link %t/producer-opt.ll %t/consumer-opt.ll -o %t/linked-opt.bc
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj \
// RUN:   %t/linked-opt.bc -o /dev/null

//--- record.h
struct Managed {
  long Value;

  explicit Managed(long Value);
  Managed(const Managed &Other);
  Managed(Managed &&Other);
  ~Managed();
};

Managed transform(Managed Input, long Delta);

//--- producer.cpp
#include "record.h"

extern "C" void observe(long);

Managed::Managed(long Value) : Value(Value) {}
Managed::Managed(const Managed &Other) : Value(Other.Value) {}
Managed::Managed(Managed &&Other) : Value(Other.Value + 1) {}
Managed::~Managed() { observe(Value); }

Managed transform(Managed Input, long Delta) {
  Input.Value += Delta;
  return static_cast<Managed &&>(Input);
}

// PRODUCER-LABEL: define dso_local void @_Z9transform7Managedl(
// PRODUCER-SAME: ptr dead_on_unwind noalias writable sret(%struct.Managed) align 8 %agg.result,
// PRODUCER-SAME: ptr nofreeobj noundef align 8
// PRODUCER-SAME: dereferenceable(8) %Input, i64 noundef %Delta)
// PRODUCER: call void @_ZN7ManagedC1EOS_(
// PRODUCER-SAME: ptr noundef nonnull align 8 dereferenceable(8) %agg.result,
// PRODUCER-SAME: ptr noundef nonnull align 8 dereferenceable(8) %Input)
// PRODUCER-NOT: call void @_ZN7ManagedD1Ev(ptr {{.*}}%Input)

// OPT-PRODUCER-LABEL: define dso_local void @_Z9transform7Managedl(
// OPT-PRODUCER-SAME: ptr {{[^,]*}}sret(%struct.Managed) align 8{{[^%]*}}%agg.result,
// OPT-PRODUCER-SAME: ptr {{[^,]*}}noundef align 8{{[^%]*}}%Input,
// OPT-PRODUCER-SAME: i64 noundef %Delta)

//--- consumer.cpp
#include "record.h"

long consume_copy(long Value) {
  Managed Source(Value);
  Managed Result = transform(Source, 2);
  return Result.Value;
}

long consume_move(long Value) {
  Managed Source(Value);
  Managed Result = transform(static_cast<Managed &&>(Source), 3);
  return Result.Value;
}

// CONSUMER-LABEL: define dso_local noundef i64 @_Z12consume_copyl(
// CONSUMER: call void @_ZN7ManagedC1El(ptr {{.*}}%Source, i64 noundef
// CONSUMER: call void @_ZN7ManagedC1ERKS_(ptr {{.*}}%agg.tmp, ptr {{.*}}%Source)
// CONSUMER: call void @_Z9transform7Managedl(
// CONSUMER-SAME: ptr {{.*}}sret(%struct.Managed) align 8 %Result,
// CONSUMER-SAME: ptr nofreeobj noundef align 8
// CONSUMER-SAME: dereferenceable(8) %agg.tmp, i64 noundef 2)
// CONSUMER: call void @_ZN7ManagedD1Ev(ptr {{.*}}%agg.tmp)
// CONSUMER: call void @_ZN7ManagedD1Ev(ptr {{.*}}%Result)
// CONSUMER: call void @_ZN7ManagedD1Ev(ptr {{.*}}%Source)
// CONSUMER-LABEL: define dso_local noundef i64 @_Z12consume_movel(
// CONSUMER: call void @_ZN7ManagedC1EOS_(ptr {{.*}}%agg.tmp, ptr {{.*}}%Source)
// CONSUMER: call void @_Z9transform7Managedl(
// CONSUMER-SAME: ptr {{.*}}sret(%struct.Managed) align 8 %Result,
// CONSUMER-SAME: ptr nofreeobj noundef align 8
// CONSUMER-SAME: dereferenceable(8) %agg.tmp, i64 noundef 3)
// CONSUMER: call void @_ZN7ManagedD1Ev(ptr {{.*}}%agg.tmp)
// CONSUMER: call void @_ZN7ManagedD1Ev(ptr {{.*}}%Result)
// CONSUMER: call void @_ZN7ManagedD1Ev(ptr {{.*}}%Source)

// OPT-CONSUMER-LABEL: define dso_local noundef i64 @_Z12consume_copyl(
// OPT-CONSUMER: call void @_Z9transform7Managedl(
// OPT-CONSUMER-SAME: ptr {{[^,]*}}sret(%struct.Managed) align 8 %Result,
// OPT-CONSUMER-SAME: ptr nofreeobj noundef nonnull align 8
// OPT-CONSUMER-SAME: dereferenceable(8) %agg.tmp, i64 noundef 2)
// OPT-CONSUMER-LABEL: define dso_local noundef i64 @_Z12consume_movel(
// OPT-CONSUMER: call void @_Z9transform7Managedl(
// OPT-CONSUMER-SAME: ptr {{[^,]*}}sret(%struct.Managed) align 8 %Result,
// OPT-CONSUMER-SAME: ptr nofreeobj noundef nonnull align 8
// OPT-CONSUMER-SAME: dereferenceable(8) %agg.tmp, i64 noundef 3)
