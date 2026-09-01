// REQUIRES: mmix-registered-target

// RUN: rm -rf %t && split-file %s %t
// RUN: %clang --target=mmix-unknown-unknown -O2 -ffreestanding -c \
// RUN:   %t/entry.c -o %t/entry.o
// RUN: %clang --target=mmix-unknown-unknown -O2 -flto -ffreestanding -c \
// RUN:   %t/direct.c -o %t/direct.bc
// RUN: %clang --target=mmix-unknown-unknown -O2 -flto -ffreestanding -c \
// RUN:   %t/archive.c -o %t/archive.bc
// RUN: %clang --target=mmix-unknown-unknown -O2 -flto -ffreestanding -c \
// RUN:   %t/unused.c -o %t/unused.bc
// RUN: llvm-ar rcs %t/libbitcode.a %t/unused.bc %t/archive.bc
// RUN: env PATH=/usr/bin:/bin %clang --target=mmix-unknown-unknown -O2 -flto \
// RUN:   -ffreestanding -nostdlib -nostartfiles -nodefaultlibs %t/entry.o \
// RUN:   %t/direct.bc %t/libbitcode.a -Wl,-e,_start,--trace -o %t/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=TRACE --implicit-check-not=unused.bc
// RUN: llvm-readobj --file-headers --symbols --relocations %t/output \
// RUN:   | FileCheck %s --check-prefix=OUTPUT --implicit-check-not=unused_leaf
// RUN: env PATH=%t/empty not %clang --target=mmix-unknown-unknown -O2 -flto \
// RUN:   -ffreestanding -nostdlib -nostartfiles -nodefaultlibs %t/entry.o \
// RUN:   %t/direct.bc -Wl,-e,_start -o %t/missing 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld.bfd|libgcc)[^/\\"]*}}'

// OUTPUT:      Format: elf64-mmix
// OUTPUT:      Type: Executable
// OUTPUT:      Machine: EM_MMIX
// OUTPUT:      Relocations [
// OUTPUT-NEXT: ]
// OUTPUT-DAG:  Name: _start
// OUTPUT-DAG:  Name: bitcode_direct
// TRACE:      {{.*}}entry.o
// TRACE:      {{.*}}libbitcode.a({{.*}}archive.bc)
// MISSING: ld.lld: error: undefined symbol: archive_leaf

//--- entry.c
extern long bitcode_direct(void);

long _start(void) { return bitcode_direct(); }

//--- direct.c
extern long archive_leaf(void);

__attribute__((noinline)) long bitcode_direct(void) {
  return archive_leaf() + 1;
}

//--- archive.c
__attribute__((noinline)) long archive_leaf(void) { return 41; }

//--- unused.c
long unused_leaf(void) { return 99; }
