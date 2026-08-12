// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o /dev/null

struct Empty {};
struct One { char value; };
struct Two { short value; };
struct Four { int value; };
struct Eight { long value; };
union WordUnion {
  int word;
  char bytes[4];
};
struct Bits {
  unsigned char low : 3;
  unsigned char high : 5;
};
struct Flexible {
  int prefix;
  char data[];
};
struct ZeroWidthTail {
  char value;
  unsigned : 0;
};
struct PackedWord {
  int value;
} __attribute__((packed));
struct ReAlignedWord {
  struct PackedWord value;
} __attribute__((aligned(4)));

struct Empty make_empty(void) { return (struct Empty){}; }
struct One make_one(char value) { return (struct One){value}; }
struct Two make_two(short value) { return (struct Two){value}; }
struct Four make_four(int value) { return (struct Four){value}; }
struct Eight make_eight(long value) { return (struct Eight){value}; }
union WordUnion make_union(int value) { return (union WordUnion){value}; }
struct Bits make_bits(unsigned char low, unsigned char high) {
  return (struct Bits){low, high};
}
struct Flexible make_flexible(int value) {
  return (struct Flexible){value};
}
struct ZeroWidthTail make_zero_width(char value) {
  return (struct ZeroWidthTail){value};
}
struct ReAlignedWord make_realigned(int value) {
  return (struct ReAlignedWord){{value}};
}

// CHECK-LABEL: define dso_local void @make_empty()
// CHECK: ret void
// CHECK-LABEL: define dso_local i8 @make_one(i8 noundef signext %value)
// CHECK: ret i8 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i16 @make_two(i16 noundef signext %value)
// CHECK: ret i16 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i32 @make_four(i32 noundef signext %value)
// CHECK: ret i32 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i64 @make_eight(i64 noundef %value)
// CHECK: ret i64 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i32 @make_union(i32 noundef signext %value)
// CHECK: ret i32 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i8 @make_bits(
// CHECK: ret i8 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i32 @make_flexible(i32 noundef signext %value)
// CHECK: ret i32 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i64 @make_zero_width(i8 noundef signext %value)
// CHECK: ret i64 %{{[0-9]+}}
// CHECK-LABEL: define dso_local i32 @make_realigned(i32 noundef signext %value)
// CHECK: ret i32 %{{[0-9]+}}

struct One external_one(void);

char call_direct(void) { return external_one().value; }

// CHECK-LABEL: define dso_local i8 @call_direct()
// CHECK: [[RESULT:%[a-z0-9.]+]] = call i8 @external_one()
// CHECK: store i8 [[RESULT]], ptr %coerce.dive, align 1
// CHECK: [[VALUE:%[0-9]+]] = load i8, ptr %value, align 1
// CHECK: ret i8 [[VALUE]]
// CHECK: declare dso_local i8 @external_one()
