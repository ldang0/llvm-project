// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix-unknown-unknown -std=c17 -ffreestanding \
// RUN:   -fno-stack-protector -O0 -S %s -o /dev/null
// RUN: %clang --target=mmix-unknown-unknown -std=c17 -ffreestanding \
// RUN:   -fno-stack-protector -O2 -S %s -o - | FileCheck %s

typedef unsigned char u8x8 __attribute__((ext_vector_type(8)));
typedef unsigned int u32x2 __attribute__((ext_vector_type(2)));

// CHECK-LABEL: build_words:
// CHECK:       SLU
// CHECK:       OR r231
u32x2 build_words(unsigned lane0, unsigned lane1) {
  return (u32x2){lane0, lane1};
}

// CHECK-LABEL: extract_word0:
// CHECK:       SRU r231, r231, 32
unsigned extract_word0(u32x2 value) { return value[0]; }

// CHECK-LABEL: extract_byte:
// CHECK-NOT:   PUSHJ
// CHECK:       LDBU r231
// CHECK:       POP 0, 0
unsigned char extract_byte(u8x8 value, unsigned long index) {
  return value[index];
}

// CHECK-LABEL: insert_byte:
// CHECK-NOT:   PUSHJ
// CHECK:       STBU
// CHECK:       LDOU r231
// CHECK:       POP 0, 0
u8x8 insert_byte(u8x8 value, unsigned char lane, unsigned long index) {
  value[index] = lane;
  return value;
}
