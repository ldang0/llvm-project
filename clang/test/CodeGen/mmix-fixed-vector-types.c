// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o - \
// RUN:   -DTEST_SUPPORTED %s | FileCheck %s --check-prefix=SUPPORTED
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_WIDE %s 2>&1 | FileCheck %s --check-prefix=WIDE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ODD_LANES %s 2>&1 | FileCheck %s --check-prefix=ODD
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_PARTIAL_MASK %s 2>&1 | FileCheck %s --check-prefix=MASK
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_RECORD %s 2>&1 | FileCheck %s --check-prefix=RECORD
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_ARRAY %s 2>&1 | FileCheck %s --check-prefix=ARRAY

typedef _Bool mask8 __attribute__((ext_vector_type(8)));
typedef _Bool mask64 __attribute__((ext_vector_type(64)));
typedef signed char i8x1 __attribute__((ext_vector_type(1)));
typedef signed char i8x2 __attribute__((ext_vector_type(2)));
typedef short i16x2 __attribute__((ext_vector_type(2)));
typedef int i32x2 __attribute__((ext_vector_type(2)));
typedef unsigned int u32x2 __attribute__((vector_size(8)));
typedef long i64x1 __attribute__((ext_vector_type(1)));
typedef float f32x2 __attribute__((ext_vector_type(2)));
typedef double f64x1 __attribute__((ext_vector_type(1)));
typedef long double ld64x1 __attribute__((ext_vector_type(1)));

#if defined(TEST_SUPPORTED)
mask8 masks = {1, 0, 1, 0, 1, 0, 1, 0};
mask64 wide_masks;
i8x1 byte = {1};
i8x2 bytes = {1, 2};
i16x2 halves = {3, 4};
i32x2 words = {5, 6};
u32x2 unsigned_words = {7, 8};
i64x1 octa = {7};
f32x2 singles = {1.0f, 2.0f};
f64x1 doubles = {3.0};
ld64x1 long_doubles = {4.0L};

_Static_assert(sizeof(mask8) == 1 && _Alignof(mask8) == 1, "mask8 ABI");
_Static_assert(sizeof(mask64) == 8 && _Alignof(mask64) == 8, "mask64 ABI");
_Static_assert(sizeof(i8x1) == 1 && _Alignof(i8x1) == 1, "i8x1 ABI");
_Static_assert(sizeof(i32x2) == 8 && _Alignof(i32x2) == 8, "i32x2 ABI");

void copy_supported_vectors(void) {
  i32x2 local = words;
  words = local;
}

// SUPPORTED: @masks ={{.*}} global <8 x i1>
// SUPPORTED: @byte ={{.*}} global <1 x i8>
// SUPPORTED: @bytes ={{.*}} global <2 x i8>
// SUPPORTED: @halves ={{.*}} global <2 x i16>
// SUPPORTED: @words ={{.*}} global <2 x i32>
// SUPPORTED: @unsigned_words ={{.*}} global <2 x i32>
// SUPPORTED: @octa ={{.*}} global <1 x i64>
// SUPPORTED: @singles ={{.*}} global <2 x float>
// SUPPORTED: @doubles ={{.*}} global <1 x double>
// SUPPORTED: @long_doubles ={{.*}} global <1 x double>
// SUPPORTED: @wide_masks ={{.*}} global i64 0, align 8
// SUPPORTED-LABEL: define dso_local void @copy_supported_vectors()
// SUPPORTED: load <2 x i32>, ptr @words, align 8
// SUPPORTED: store <2 x i32>
#elif defined(TEST_WIDE)
typedef int i32x4 __attribute__((ext_vector_type(4)));
i32x4 wide;
// WIDE: error: MMIX GNU ABI does not support vector value CodeGen involving type 'i32x4'
#elif defined(TEST_ODD_LANES)
typedef short i16x3 __attribute__((ext_vector_type(3)));
i16x3 odd;
// ODD: error: MMIX GNU ABI does not support vector value CodeGen involving type 'i16x3'
#elif defined(TEST_PARTIAL_MASK)
typedef _Bool mask4 __attribute__((ext_vector_type(4)));
mask4 partial_mask;
// MASK: error: MMIX GNU ABI does not support vector value CodeGen involving type 'mask4'
#elif defined(TEST_RECORD)
struct vector_record {
  i32x2 field;
};
struct vector_record record;
// RECORD: error: MMIX GNU ABI does not support vector value CodeGen involving type 'struct vector_record'
#elif defined(TEST_ARRAY)
i32x2 array[2];
// ARRAY: error: MMIX GNU ABI does not support vector value CodeGen involving type 'i32x2[2]'
#endif
