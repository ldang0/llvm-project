// REQUIRES: mmix-registered-target

// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/sysroot/include/mmix-unknown-unknown \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown \
// RUN:   %t.dir/resource/include \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fno-stack-protector -DMMIX_CRT -c %s \
// RUN:   -o %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fno-stack-protector -c %s -o %t.dir/user.o
// RUN: echo 'ENTRY(_start)' > %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo 'SECTIONS {' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo '  . = 0x1000;' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo '  .text : { *(.text .text.*) }' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo '  .rodata : { *(.rodata .rodata.*) }' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo '  .data : { *(.data .data.*) }' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo '  .bss : { *(.bss .bss.*) *(COMMON) }' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: echo '}' >> %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: llvm-ar rc %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a
// RUN: llvm-ar rc %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a
// RUN: llvm-ar rc %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a
// RUN: llvm-ar rc %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a
// RUN: llvm-ar rc %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a
// RUN: env PATH=/usr/bin:/bin %clang --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource %t.dir/user.o -o %t.dir/a.out
// RUN: llvm-readobj --file-headers --sections --symbols %t.dir/a.out \
// RUN:   | FileCheck %s

// CHECK:      Format: elf64-mmix
// CHECK:      Type: Executable
// CHECK:      Machine: EM_MMIX
// CHECK:      Name: .text
// CHECK:      Name: _start
// CHECK-NEXT: Value: 0x1000

#ifdef MMIX_CRT
__attribute__((noreturn)) void _start(void) {
  for (;;) {
  }
}
#else
volatile int value = 1;
int main(void) { return value; }
#endif
