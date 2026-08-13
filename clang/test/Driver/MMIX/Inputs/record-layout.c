struct Inner {
  unsigned char tag;
  int value;
};

struct Nested {
  unsigned short head;
  struct Inner item;
  long tail;
};

struct ArrayRecord {
  unsigned char prefix;
  unsigned short values[3];
};

union Choice {
  unsigned long whole;
  unsigned char bytes[8];
};

struct Packed {
  unsigned char tag;
  unsigned int value;
} __attribute__((packed));

struct Bits {
  unsigned a : 3;
  unsigned b : 5;
  unsigned c : 9;
};

struct ZeroWidth {
  unsigned a : 3;
  unsigned : 0;
  unsigned b : 3;
};

_Static_assert(sizeof(struct Inner) == 8, "inner size");
_Static_assert(_Alignof(struct Inner) == 4, "inner alignment");
_Static_assert(__builtin_offsetof(struct Inner, value) == 4,
               "inner value offset");
_Static_assert(sizeof(struct Nested) == 24, "nested size");
_Static_assert(_Alignof(struct Nested) == 8, "nested alignment");
_Static_assert(__builtin_offsetof(struct Nested, item) == 4,
               "nested item offset");
_Static_assert(__builtin_offsetof(struct Nested, tail) == 16,
               "nested tail offset");
_Static_assert(sizeof(struct ArrayRecord) == 8, "array record size");
_Static_assert(__builtin_offsetof(struct ArrayRecord, values) == 2,
               "array member offset");
_Static_assert(sizeof(union Choice) == 8, "union size");
_Static_assert(_Alignof(union Choice) == 8, "union alignment");
_Static_assert(sizeof(struct Packed) == 5, "packed size");
_Static_assert(_Alignof(struct Packed) == 1, "packed alignment");
_Static_assert(__builtin_offsetof(struct Packed, value) == 1,
               "packed value offset");
_Static_assert(sizeof(struct Bits) == 3, "bit-field size");
_Static_assert(_Alignof(struct Bits) == 1, "bit-field alignment");
_Static_assert(sizeof(struct ZeroWidth) == 16, "zero-width size");
_Static_assert(_Alignof(struct ZeroWidth) == 8, "zero-width alignment");

long read_nested(const struct Nested *value) {
  return value->item.value + value->tail;
}

unsigned short read_array_member(const struct ArrayRecord *value,
                                 unsigned long index) {
  return value->values[index];
}

unsigned char read_union_byte(const union Choice *value, unsigned long index) {
  return value->bytes[index];
}

void write_union_whole(union Choice *value, unsigned long whole) {
  value->whole = whole;
}

unsigned int read_packed_value(const struct Packed *value) {
  return value->value;
}

unsigned int read_first_bits(const struct Bits *value) { return value->a; }

unsigned int read_second_bits(const struct Bits *value) { return value->b; }

unsigned int read_wide_bits(const struct Bits *value) { return value->c; }

unsigned int read_after_zero_width(const struct ZeroWidth *value) {
  return value->b;
}
