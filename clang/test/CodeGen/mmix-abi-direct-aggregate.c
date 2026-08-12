// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

struct Empty {};
union EmptyUnion {};
struct One { char a; };
struct Two { short a; };
struct Four { int a; };
struct Eight { long a; };
struct PackedThree { char bytes[3]; } __attribute__((packed));
struct PackedFive { char a; int b; } __attribute__((packed));
struct NestedSix {
  char a;
  struct {
    short b;
    char c;
  } nested;
};
union WordUnion {
  int word;
  char bytes[4];
};
struct Bits {
  unsigned char a : 3;
  unsigned char b : 5;
};
struct Flexible {
  int prefix;
  char data[];
};

void take_all(long first, struct Empty empty, union EmptyUnion empty_union,
              struct One one, struct Two two, struct Four four,
              struct Eight eight,
              struct PackedThree three, struct PackedFive five,
              struct NestedSix nested, union WordUnion u, struct Bits bits,
              struct Flexible flexible, long last);

void forward_all(long first, struct Empty empty, union EmptyUnion empty_union,
                 struct One one, struct Two two, struct Four four,
                 struct Eight eight,
                 struct PackedThree three, struct PackedFive five,
                 struct NestedSix nested, union WordUnion u, struct Bits bits,
                 struct Flexible flexible, long last) {
  take_all(first, empty, empty_union, one, two, four, eight, three, five,
           nested, u, bits, flexible, last);
}

// CHECK-LABEL: define dso_local void @forward_all(
// CHECK-SAME: i64 noundef %first, i8 noext %one.coerce,
// CHECK-SAME: i16 noext %two.coerce, i32 noext %four.coerce,
// CHECK-SAME: i64 %eight.coerce, i64 %three.target_coerce,
// CHECK-SAME: i64 %five.target_coerce, i64 %nested.target_coerce,
// CHECK-SAME: i32 noext %u.coerce, i8 noext %bits.coerce,
// CHECK-SAME: i32 noext %flexible.coerce,
// CHECK-SAME: i64 noundef %last)
// CHECK: [[THREE0:%[0-9]+]] = lshr i64 %three.target_coerce, 16
// CHECK: [[THREEBYTE0:%[0-9]+]] = trunc i64 [[THREE0]] to i8
// CHECK: store i8 [[THREEBYTE0]], ptr %{{[0-9]+}}, align 1
// CHECK: [[THREE1:%[0-9]+]] = lshr i64 %three.target_coerce, 8
// CHECK: [[THREEBYTE1:%[0-9]+]] = trunc i64 [[THREE1]] to i8
// CHECK: store i8 [[THREEBYTE1]], ptr %{{[0-9]+}}, align 1
// CHECK: [[THREEBYTE2:%[0-9]+]] = trunc i64 %three.target_coerce to i8
// CHECK: store i8 [[THREEBYTE2]], ptr %{{[0-9]+}}, align 1
// CHECK: call void @take_all(i64 noundef %{{[0-9]+}},
// CHECK-SAME: i8 noext %{{[0-9]+}}, i16 noext %{{[0-9]+}},
// CHECK-SAME: i32 noext %{{[0-9]+}}, i64 %{{[0-9]+}},
// CHECK-SAME: i64 %{{[0-9]+}}, i64 %{{[0-9]+}}, i64 %{{[0-9]+}},
// CHECK-SAME: i32 noext %{{[0-9]+}}, i8 noext %{{[0-9]+}},
// CHECK-SAME: i32 noext %{{[0-9]+}},
// CHECK-SAME: i64 noundef %{{[0-9]+}})

// CHECK: declare dso_local void @take_all(i64 noundef, i8 noext, i16 noext,
// CHECK-SAME: i32 noext, i64, i64, i64, i64, i32 noext, i8 noext,
// CHECK-SAME: i32 noext, i64 noundef)

void make_three(char a, char b, char c) {
  struct PackedThree value = {{a, b, c}};
  take_all(0, (struct Empty){}, (union EmptyUnion){}, (struct One){0},
           (struct Two){0}, (struct Four){0}, (struct Eight){0}, value,
           (struct PackedFive){0}, (struct NestedSix){0}, (union WordUnion){0},
           (struct Bits){0}, (struct Flexible){0}, 0);
}

// CHECK-LABEL: define dso_local void @make_three(
// CHECK: [[AADDR:%[0-9]+]] = getelementptr inbounds i8, ptr %value, i64 0
// CHECK: [[A:%[0-9]+]] = load i8, ptr [[AADDR]], align 1
// CHECK: [[AZ:%[a-z0-9.]+]] = zext i8 [[A]] to i64
// CHECK: [[AS:%[a-z0-9.]+]] = shl i64 [[AZ]], 16
// CHECK: [[BADDR:%[0-9]+]] = getelementptr inbounds i8, ptr %value, i64 1
// CHECK: [[B:%[0-9]+]] = load i8, ptr [[BADDR]], align 1
// CHECK: [[BZ:%[a-z0-9.]+]] = zext i8 [[B]] to i64
// CHECK: [[BS:%[a-z0-9.]+]] = shl i64 [[BZ]], 8
// CHECK: [[CADDR:%[0-9]+]] = getelementptr inbounds i8, ptr %value, i64 2
// CHECK: [[C:%[0-9]+]] = load i8, ptr [[CADDR]], align 1
// CHECK: [[CZ:%[a-z0-9.]+]] = zext i8 [[C]] to i64
// CHECK: call void @take_all({{.*}}i64 %{{[0-9]+}}{{.*}})
