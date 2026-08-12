// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -fdump-record-layouts-simple \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s

struct Scalars {
  char c;
  long l;
};

struct __attribute__((packed)) Packed {
  char c;
  long l;
};

struct Nested {
  char c;
  struct Scalars value;
};

union Choice {
  char c;
  long l;
};

struct Array {
  char c;
  short values[2];
};

struct Flexible {
  char c;
  int values[];
};

struct Empty {};

struct EmptyMember {
  char before;
  struct Empty empty;
  char after;
};

struct Bits {
  unsigned a : 3;
  unsigned b : 30;
  unsigned c : 2;
};

struct ZeroWidth {
  unsigned a : 3;
  unsigned : 0;
  unsigned b : 3;
};

struct __attribute__((packed)) PackedZeroWidth {
  unsigned a : 3;
  unsigned : 0;
  unsigned b : 3;
};

struct Scalars scalars;
struct Packed packed;
struct Nested nested;
union Choice choice;
struct Array array;
struct Flexible flexible;
struct EmptyMember empty_member;
struct Bits bits;
struct ZeroWidth zero_width;
struct PackedZeroWidth packed_zero_width;

_Static_assert(sizeof(struct Scalars) == 16, "");
_Static_assert(_Alignof(struct Scalars) == 8, "");
_Static_assert(__builtin_offsetof(struct Scalars, l) == 8, "");
_Static_assert(sizeof(struct Packed) == 9, "");
_Static_assert(_Alignof(struct Packed) == 1, "");
_Static_assert(__builtin_offsetof(struct Packed, l) == 1, "");
_Static_assert(sizeof(struct Nested) == 24, "");
_Static_assert(__builtin_offsetof(struct Nested, value) == 8, "");
_Static_assert(sizeof(union Choice) == 8, "");
_Static_assert(_Alignof(union Choice) == 8, "");
_Static_assert(sizeof(struct Array) == 6, "");
_Static_assert(__builtin_offsetof(struct Array, values) == 2, "");
_Static_assert(sizeof(struct Flexible) == 4, "");
_Static_assert(__builtin_offsetof(struct Flexible, values) == 4, "");
_Static_assert(sizeof(struct Empty) == 0, "");
_Static_assert(sizeof(struct EmptyMember) == 2, "");
_Static_assert(sizeof(struct Bits) == 5, "");
_Static_assert(_Alignof(struct Bits) == 1, "");
_Static_assert(sizeof(struct ZeroWidth) == 16, "");
_Static_assert(_Alignof(struct ZeroWidth) == 8, "");
_Static_assert(sizeof(struct PackedZeroWidth) == 16, "");
_Static_assert(_Alignof(struct PackedZeroWidth) == 8, "");

// CHECK-LABEL: Type: struct Bits
// CHECK: Size:40
// CHECK: DataSize:40
// CHECK: Alignment:8
// CHECK: FieldOffsets: [0, 3, 33]

// CHECK-LABEL: Type: struct ZeroWidth
// CHECK: Size:128
// CHECK: DataSize:128
// CHECK: Alignment:64
// CHECK: FieldOffsets: [0, 64, 64]

// CHECK-LABEL: Type: struct PackedZeroWidth
// CHECK: Size:128
// CHECK: DataSize:128
// CHECK: Alignment:64
// CHECK: FieldOffsets: [0, 64, 64]
