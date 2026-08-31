// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -O0 -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -O2 -o - %s | FileCheck %s

typedef enum SignedEnum { SignedValue = -1 } SignedEnum;
typedef enum UnsignedEnum { UnsignedValue = 0xffffffffU } UnsignedEnum;

typedef int (*NarrowCallback)(_Bool, char, signed char, unsigned char, short,
                              unsigned short, int, unsigned int, SignedEnum,
                              UnsignedEnum);

extern int external_narrow(_Bool, char, signed char, unsigned char, short,
                           unsigned short, int, unsigned int, SignedEnum,
                           UnsignedEnum);

__attribute__((noinline)) int
define_and_call(_Bool b, char c, signed char sc, unsigned char uc, short s,
                unsigned short us, int i, unsigned int ui, SignedEnum se,
                UnsignedEnum ue) {
  return external_narrow(b, c, sc, uc, s, us, i, ui, se, ue);
}

// CHECK-LABEL: define dso_local i32 @define_and_call(
// CHECK-SAME: i1 noundef zeroext %b, i8 noundef signext %c,
// CHECK-SAME: i8 noundef signext %sc, i8 noundef zeroext %uc,
// CHECK-SAME: i16 noundef signext %s, i16 noundef zeroext %us,
// CHECK-SAME: i32 noundef signext %i, i32 noundef zeroext %ui,
// CHECK-SAME: i32 noundef signext %se, i32 noundef zeroext %ue)
// CHECK: call i32 @external_narrow(i1 noundef zeroext
// CHECK-SAME: i8 noundef signext
// CHECK-SAME: i8 noundef signext
// CHECK-SAME: i8 noundef zeroext
// CHECK-SAME: i16 noundef signext
// CHECK-SAME: i16 noundef zeroext
// CHECK-SAME: i32 noundef signext
// CHECK-SAME: i32 noundef zeroext
// CHECK-SAME: i32 noundef signext
// CHECK-SAME: i32 noundef zeroext

// CHECK: declare dso_local i32 @external_narrow(i1 noundef zeroext,
// CHECK-SAME: i8 noundef signext, i8 noundef signext, i8 noundef zeroext,
// CHECK-SAME: i16 noundef signext, i16 noundef zeroext, i32 noundef signext,
// CHECK-SAME: i32 noundef zeroext, i32 noundef signext, i32 noundef zeroext)

__attribute__((noinline)) int
call_callback(NarrowCallback callback, _Bool b, char c, signed char sc,
              unsigned char uc, short s, unsigned short us, int i,
              unsigned int ui, SignedEnum se, UnsignedEnum ue) {
  return callback(b, c, sc, uc, s, us, i, ui, se, ue);
}

// CHECK-LABEL: define dso_local i32 @call_callback(
// CHECK-SAME: ptr noundef %callback, i1 noundef zeroext %b,
// CHECK-SAME: i8 noundef signext %c, i8 noundef signext %sc,
// CHECK-SAME: i8 noundef zeroext %uc, i16 noundef signext %s,
// CHECK-SAME: i16 noundef zeroext %us, i32 noundef signext %i,
// CHECK-SAME: i32 noundef zeroext %ui, i32 noundef signext %se,
// CHECK-SAME: i32 noundef zeroext %ue)
// CHECK: call i32 %{{[^ (]+}}(i1 noundef zeroext
// CHECK-SAME: i8 noundef signext
// CHECK-SAME: i8 noundef signext
// CHECK-SAME: i8 noundef zeroext
// CHECK-SAME: i16 noundef signext
// CHECK-SAME: i16 noundef zeroext
// CHECK-SAME: i32 noundef signext
// CHECK-SAME: i32 noundef zeroext
// CHECK-SAME: i32 noundef signext
// CHECK-SAME: i32 noundef zeroext

_Bool return_bool(_Bool value) { return value; }
signed char return_schar(signed char value) { return value; }
unsigned char return_uchar(unsigned char value) { return value; }
short return_short(short value) { return value; }
unsigned short return_ushort(unsigned short value) { return value; }
int return_int(int value) { return value; }
unsigned int return_uint(unsigned int value) { return value; }
SignedEnum return_signed_enum(SignedEnum value) { return value; }
UnsignedEnum return_unsigned_enum(UnsignedEnum value) { return value; }

// CHECK-LABEL: define dso_local i1 @return_bool(i1 noundef zeroext
// CHECK-LABEL: define dso_local i8 @return_schar(i8 noundef signext
// CHECK-LABEL: define dso_local i8 @return_uchar(i8 noundef zeroext
// CHECK-LABEL: define dso_local i16 @return_short(i16 noundef signext
// CHECK-LABEL: define dso_local i16 @return_ushort(i16 noundef zeroext
// CHECK-LABEL: define dso_local i32 @return_int(i32 noundef signext
// CHECK-LABEL: define dso_local i32 @return_uint(i32 noundef zeroext
// CHECK-LABEL: define dso_local i32 @return_signed_enum(i32 noundef signext
// CHECK-LABEL: define dso_local i32 @return_unsigned_enum(i32 noundef zeroext
