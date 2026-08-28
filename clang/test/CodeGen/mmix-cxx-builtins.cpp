// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -O0 \
// RUN:   -S -emit-llvm %s -o %t.debug.ll
// RUN: FileCheck %s --check-prefixes=IR,DEBUG-IR \
// RUN:   --implicit-check-not=personality --implicit-check-not=invoke \
// RUN:   --implicit-check-not=uwtable --implicit-check-not=__cxa \
// RUN:   --input-file=%t.debug.ll
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -O3 \
// RUN:   -S -emit-llvm %s -o %t.release.ll
// RUN: FileCheck %s --check-prefixes=IR,RELEASE-IR \
// RUN:   --implicit-check-not=personality --implicit-check-not=invoke \
// RUN:   --implicit-check-not=uwtable --implicit-check-not=__cxa \
// RUN:   --input-file=%t.release.ll
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -O0 \
// RUN:   -c %s -o %t.debug.o
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -O3 \
// RUN:   -c %s -o %t.release.o
// RUN: llvm-nm --undefined-only %t.debug.o | count 0
// RUN: llvm-nm --undefined-only %t.release.o | count 0
// RUN: llvm-objdump --no-print-imm-hex -d %t.debug.o \
// RUN:   | FileCheck %s --check-prefix=ASM
// RUN: llvm-objdump --no-print-imm-hex -d %t.release.o \
// RUN:   | FileCheck %s --check-prefix=ASM

struct PatternState {
  unsigned long Word;
  unsigned char Bytes[8];
};

extern "C" unsigned long pattern_state(bool Replace) {
  PatternState State;
  if (Replace) {
    State.Word = 7;
    State.Bytes[0] = 3;
  }
  return State.Word ^ State.Bytes[0];
}

extern "C" unsigned long builtin_bits(unsigned long Value) {
  unsigned Leading = __builtin_clzl(Value);
  unsigned Trailing = __builtin_ctzl(Value);
  unsigned Population = __builtin_popcountl(Value);
  return __builtin_bswap64(Value) ^ Leading ^ Trailing ^ Population;
}

extern "C" unsigned long builtin_memory(const unsigned long *Source) {
  unsigned long Copy;
  unsigned long Clear;
  __builtin_memcpy_inline(&Copy, Source, sizeof(Copy));
  __builtin_memset_inline(&Clear, 0, sizeof(Clear));
  return Copy ^ Clear;
}

extern "C" unsigned long builtin_bit_cast(double Value) {
  return __builtin_bit_cast(unsigned long, Value);
}

extern "C" [[gnu::flatten]] unsigned long
builtin_attributes(const unsigned long *Pointer) {
  const auto *Aligned = static_cast<const unsigned long *>(
      __builtin_assume_aligned(Pointer, alignof(unsigned long)));
  return __builtin_expect(*Aligned != 0, 1) ? *Aligned : 1;
}

// IR-LABEL: define{{.*}} @pattern_state(
// DEBUG-IR: call void @llvm.memcpy
// IR-LABEL: define{{.*}} @builtin_bits(
// IR: {{(tail )?call.*}}i64 @llvm.ctlz.i64
// IR: {{(tail )?call.*}}i64 @llvm.cttz.i64
// IR: {{(tail )?call.*}}i64 @llvm.ctpop.i64
// IR: {{(tail )?call.*}}i64 @llvm.bswap.i64
// IR-LABEL: define{{.*}} @builtin_memory(
// DEBUG-IR: call void @llvm.memcpy.inline
// DEBUG-IR: call void @llvm.memset.inline
// IR-LABEL: define{{.*}} @builtin_bit_cast(
// DEBUG-IR: load i64
// RELEASE-IR: bitcast double
// IR-LABEL: define{{.*}} @builtin_attributes(
// DEBUG-IR: call void @llvm.assume

// ASM-LABEL: <pattern_state>:
// ASM:       POP 0, 0
// ASM-LABEL: <builtin_bits>:
// ASM:       SADD
// ASM:       POP 0, 0
// ASM-LABEL: <builtin_memory>:
// ASM:       LDO
// ASM:       POP 0, 0
// ASM-LABEL: <builtin_bit_cast>:
// ASM:       POP 0, 0
// ASM-LABEL: <builtin_attributes>:
// ASM:       POP 0, 0
