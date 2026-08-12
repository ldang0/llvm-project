// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

struct Packed {
  char c;
  short s;
} __attribute__((packed));

char global_char;
short global_short;
char global_array[3];
struct Packed global_packed;
long global_long;
char global_aligned[1] __attribute__((aligned(16)));
char global_max_aligned[1] __attribute__((aligned(32768)));

// CHECK-DAG: @global_char ={{.*}} global i8 0, align 4
// CHECK-DAG: @global_short ={{.*}} global i16 0, align 4
// CHECK-DAG: @global_array ={{.*}} global [3 x i8] zeroinitializer, align 4
// CHECK-DAG: @global_packed ={{.*}} global %struct.Packed zeroinitializer, align 4
// CHECK-DAG: @global_long ={{.*}} global i64 0, align 8
// CHECK-DAG: @global_aligned ={{.*}} global [1 x i8] zeroinitializer, align 16
// CHECK-DAG: @global_max_aligned ={{.*}} global [1 x i8] zeroinitializer, align 32768
// CHECK-DAG: @bit_values ={{.*}} global %struct.Bits { i8 -79 }, align 4

void locals(void) {
  char local_char;
  short local_short;
  char local_array[3];
  struct Packed local_packed;
  long local_long;

  // CHECK-LABEL: define{{.*}} void @locals()
  // CHECK: %local_char = alloca i8, align 1
  // CHECK: %local_short = alloca i16, align 2
  // CHECK: %local_array = alloca [3 x i8], align 1
  // CHECK: %local_packed = alloca %struct.Packed, align 1
  // CHECK: %local_long = alloca i64, align 8
}

struct Bits {
  unsigned a : 3;
  unsigned b : 5;
};

struct Bits bit_values = {5, 17};
