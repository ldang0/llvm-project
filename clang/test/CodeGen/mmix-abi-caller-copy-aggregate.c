// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel \
// RUN:   %t.ll -o - | FileCheck %s --check-prefix=ISEL

struct Pair {
  long first;
  long second;
};
struct PackedTwelve {
  int words[3];
} __attribute__((packed));
struct Large {
  long words[10];
};

void take_pair(struct Pair value);
void take_packed(struct PackedTwelve value);
void take_large(struct Large value);

long read_pair(struct Pair value) {
  value.second = 9;
  return value.first;
}

// CHECK-LABEL: define dso_local i64 @read_pair(
// CHECK-SAME: ptr noundef byval(%struct.Pair) align 8 %value)
// CHECK-NOT: byref
// CHECK-NOT: sret
// CHECK-NOT: inreg
// CHECK: store i64 9, ptr %second, align 8

void direct_call(struct Pair value, struct PackedTwelve packed,
                 struct Large large) {
  take_pair(value);
  take_packed(packed);
  take_large(large);
}

// CHECK-LABEL: define dso_local void @direct_call(
// CHECK-SAME: ptr noundef byval(%struct.Pair) align 8 %value,
// CHECK-SAME: ptr noundef byval(%struct.PackedTwelve) align 1 %packed,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %large)
// CHECK: call void @take_pair(ptr noundef byval(%struct.Pair) align 8 %value)
// CHECK: call void @take_packed(ptr noundef byval(%struct.PackedTwelve) align 1 %packed)
// CHECK: call void @take_large(ptr noundef byval(%struct.Large) align 8 %large)
// CHECK: declare dso_local void @take_pair(ptr noundef byval(%struct.Pair) align 8)
// CHECK: declare dso_local void @take_packed(ptr noundef byval(%struct.PackedTwelve) align 1)
// CHECK: declare dso_local void @take_large(ptr noundef byval(%struct.Large) align 8)

typedef void (*pair_consumer)(struct Pair);

void indirect_call(pair_consumer consume, struct Pair value) {
  consume(value);
}

// CHECK-LABEL: define dso_local void @indirect_call(
// CHECK-SAME: ptr noundef %consume, ptr noundef byval(%struct.Pair) align 8 %value)
// CHECK: call void %{{[0-9]+}}(ptr noundef byval(%struct.Pair) align 8 %value)

struct Pair *select_pair(struct Pair *value);

void nested_call(struct Pair value) {
  take_pair(*select_pair(&value));
}

// CHECK-LABEL: define dso_local void @nested_call(
// CHECK-SAME: ptr noundef byval(%struct.Pair) align 8 %value)
// CHECK: [[SELECTED:%[a-z0-9.]+]] = call ptr @select_pair(ptr noundef %value)
// CHECK: call void @take_pair(ptr noundef byval(%struct.Pair) align 8 [[SELECTED]])

void last_register_slot(long a0, long a1, long a2, long a3, long a4, long a5,
                        long a6, long a7, long a8, long a9, long a10, long a11,
                        long a12, long a13, long a14, struct Pair value) {
  take_pair(value);
}

// CHECK-LABEL: define dso_local void @last_register_slot(
// CHECK-SAME: i64 noundef %a14,
// CHECK-SAME: ptr noundef byval(%struct.Pair) align 8 %value)

void first_stack_slot(long a0, long a1, long a2, long a3, long a4, long a5,
                      long a6, long a7, long a8, long a9, long a10, long a11,
                      long a12, long a13, long a14, long a15,
                      struct Pair value) {
  take_pair(value);
}

// CHECK-LABEL: define dso_local void @first_stack_slot(
// CHECK-SAME: i64 noundef %a15,
// CHECK-SAME: ptr noundef byval(%struct.Pair) align 8 %value)

// ISEL-LABEL: name: indirect_call
// ISEL: stack:
// ISEL: - { id: [[INDIRECT_COPY:[0-9]+]], {{.*}}size: 16, alignment: 8,
// ISEL: STOUI {{.*}}%stack.[[INDIRECT_COPY]], 8
// ISEL: STOUI {{.*}}%stack.[[INDIRECT_COPY]], 0
// ISEL: CALL_STATE
// ISEL-LABEL: name: nested_call
// ISEL: stack:
// ISEL: - { id: [[NESTED_COPY:[0-9]+]], {{.*}}size: 16, alignment: 8,
// ISEL: DIRECT_CALL_STATE @select_pair
// ISEL: STOUI {{.*}}%stack.[[NESTED_COPY]], 8
// ISEL: STOUI {{.*}}%stack.[[NESTED_COPY]], 0
// ISEL: DIRECT_CALL_STATE @take_pair
// ISEL-LABEL: name: last_register_slot
// ISEL: liveins:
// ISEL: - { reg: '$r246', virtual-reg:
// ISEL-LABEL: name: first_stack_slot
// ISEL: fixedStack:
// ISEL-NEXT: - { id: {{[0-9]+}}, type: default, offset: 0, size: 8, alignment: 8,
