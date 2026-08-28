// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -x c -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o - %s \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -x c++ -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o - %s \
// RUN:   | FileCheck %s
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -x c -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VARIADIC_PARAMETER %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=VARIADIC-PARAMETER
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -x c -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VARIADIC_ARGUMENT %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=VARIADIC-ARGUMENT

#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#if defined(__cplusplus)
typedef bool mask8 __attribute__((ext_vector_type(8)));
#else
typedef _Bool mask8 __attribute__((ext_vector_type(8)));
#endif
typedef signed char i8x1 __attribute__((ext_vector_type(1)));
typedef signed char i8x2 __attribute__((ext_vector_type(2)));
typedef short i16x2 __attribute__((ext_vector_type(2)));
typedef int i32x2 __attribute__((ext_vector_type(2)));
typedef float f32x2 __attribute__((ext_vector_type(2)));
typedef double f64x1 __attribute__((ext_vector_type(1)));

#if defined(TEST_VARIADIC_PARAMETER)
void invalid(i32x2 value, ...) {}
// VARIADIC-PARAMETER: error: MMIX GNU ABI does not support vector parameters in variadic functions
#elif defined(TEST_VARIADIC_ARGUMENT)
void sink(int named, ...);
void invalid(i32x2 value) { sink(0, value); }
// VARIADIC-ARGUMENT: error: MMIX GNU ABI does not support variadic vector argument type 'i32x2'
#else
EXTERN_C mask8 pass_mask(mask8 value) { return value; }
EXTERN_C i8x1 pass_byte(i8x1 value) { return value; }
EXTERN_C i8x2 pass_pair(i8x2 value) { return value; }
EXTERN_C i16x2 pass_halves(i16x2 value) { return value; }
EXTERN_C i32x2 pass_words(i32x2 value) { return value; }
EXTERN_C f32x2 pass_singles(f32x2 value) { return value; }
EXTERN_C f64x1 pass_double(f64x1 value) { return value; }

// CHECK-LABEL: define dso_local i8 @pass_mask(i8 noext noundef %value.coerce)
// CHECK: %retval = alloca <8 x i1>, align 1
// CHECK-LABEL: define dso_local i8 @pass_byte(i8 noext noundef %value.coerce)
// CHECK: %retval = alloca <1 x i8>, align 1
// CHECK-LABEL: define dso_local i16 @pass_pair(i16 noext noundef %value.coerce)
// CHECK: %retval = alloca <2 x i8>, align 2
// CHECK-LABEL: define dso_local i32 @pass_halves(i32 noext noundef %value.coerce)
// CHECK: %retval = alloca <2 x i16>, align 4
// CHECK-LABEL: define dso_local i64 @pass_words(i64 noundef %value.coerce)
// CHECK: %retval = alloca <2 x i32>, align 8
// CHECK-LABEL: define dso_local i64 @pass_singles(i64 noundef %value.coerce)
// CHECK: %retval = alloca <2 x float>, align 8
// CHECK-LABEL: define dso_local i64 @pass_double(i64 noundef %value.coerce)
// CHECK: %retval = alloca <1 x double>, align 8

EXTERN_C void take_seventeen(
    i32x2 v00, i32x2 v01, i32x2 v02, i32x2 v03, i32x2 v04, i32x2 v05,
    i32x2 v06, i32x2 v07, i32x2 v08, i32x2 v09, i32x2 v10, i32x2 v11,
    i32x2 v12, i32x2 v13, i32x2 v14, i32x2 v15, i8x2 v16);

EXTERN_C void forward_seventeen(
    i32x2 v00, i32x2 v01, i32x2 v02, i32x2 v03, i32x2 v04, i32x2 v05,
    i32x2 v06, i32x2 v07, i32x2 v08, i32x2 v09, i32x2 v10, i32x2 v11,
    i32x2 v12, i32x2 v13, i32x2 v14, i32x2 v15, i8x2 v16) {
  take_seventeen(v00, v01, v02, v03, v04, v05, v06, v07, v08, v09, v10,
                 v11, v12, v13, v14, v15, v16);
}

// CHECK-LABEL: define dso_local void @forward_seventeen(
// CHECK-SAME: i64 noundef %v00.coerce, i64 noundef %v01.coerce,
// CHECK-SAME: i64 noundef %v02.coerce, i64 noundef %v03.coerce,
// CHECK-SAME: i64 noundef %v04.coerce, i64 noundef %v05.coerce,
// CHECK-SAME: i64 noundef %v06.coerce, i64 noundef %v07.coerce,
// CHECK-SAME: i64 noundef %v08.coerce, i64 noundef %v09.coerce,
// CHECK-SAME: i64 noundef %v10.coerce, i64 noundef %v11.coerce,
// CHECK-SAME: i64 noundef %v12.coerce, i64 noundef %v13.coerce,
// CHECK-SAME: i64 noundef %v14.coerce, i64 noundef %v15.coerce,
// CHECK-SAME: i16 noext noundef %v16.coerce)
// CHECK: call void @take_seventeen(
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^,]+}},
// CHECK-SAME: i16 noext noundef %{{[^)]+}})
// CHECK: declare dso_local void @take_seventeen(
// CHECK-SAME: i64 noundef, i64 noundef, i64 noundef, i64 noundef,
// CHECK-SAME: i64 noundef, i64 noundef, i64 noundef, i64 noundef,
// CHECK-SAME: i64 noundef, i64 noundef, i64 noundef, i64 noundef,
// CHECK-SAME: i64 noundef, i64 noundef, i64 noundef, i64 noundef,
// CHECK-SAME: i16 noext noundef)
#endif
