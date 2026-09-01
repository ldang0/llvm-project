// REQUIRES: mmix-registered-target

// RUN: rm -rf %t && mkdir -p %t
// RUN: %clang --target=mmix-unknown-unknown -c %s -o %t/input.o
// RUN: llvm-ar rcs %t/library.a %t/input.o
// RUN: touch %t/script.ld
// RUN: %clang -### --target=mmix-unknown-unknown -r %t/input.o %t/library.a \
// RUN:   -L %t -u retained -Wl,--whole-archive,--no-whole-archive \
// RUN:   -T %t/script.ld -o %t/partial.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=COMMAND \
// RUN:       --implicit-check-not=-static --implicit-check-not=crt \
// RUN:       --implicit-check-not=libc --implicit-check-not=libm \
// RUN:       --implicit-check-not=libclang_rt --implicit-check-not=-e
// RUN: env PATH=/usr/bin:/bin %clang --target=mmix-unknown-unknown -r \
// RUN:   %t/input.o -o %t/object-partial.o
// RUN: llvm-readobj --file-headers --symbols %t/object-partial.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: env PATH=/usr/bin:/bin %clang --target=mmix-unknown-elf -r %s \
// RUN:   -o %t/source-partial.o
// RUN: llvm-readobj --file-headers --symbols %t/source-partial.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: not %clang -### --target=mmix-unknown-unknown -r -flto %s \
// RUN:   -o %t/lto.o 2>&1 | FileCheck %s --check-prefix=LTO
// RUN: not %clang -### --target=mmix-unknown-unknown -r -flto=thin %s \
// RUN:   -o %t/thin.o 2>&1 | FileCheck %s --check-prefix=THIN
// RUN: not %clang -### --target=mmix-unknown-unknown -r -fuse-ld=bfd \
// RUN:   %t/input.o -o %t/bfd.o 2>&1 | FileCheck %s --check-prefix=LINKER
// RUN: not %clang -### --target=mmix-unknown-unknown -r -rtlib=compiler-rt \
// RUN:   %t/input.o -o %t/runtime.o 2>&1 | FileCheck %s --check-prefix=RUNTIME
// RUN: not %clang -### --target=mmix-unknown-unknown -r -fPIC %s \
// RUN:   -o %t/pic.o 2>&1 | FileCheck %s --check-prefix=PIC

// COMMAND:      "{{.*}}ld.lld" "-m" "elf64mmix" "-r"
// COMMAND-SAME: "-L{{[^"]*}}"
// COMMAND-SAME: "-u" "retained"
// COMMAND-SAME: "-T" "{{.*}}script.ld"
// COMMAND-SAME: "{{.*}}input.o" "{{.*}}library.a"
// COMMAND-SAME: "--whole-archive" "--no-whole-archive"
// COMMAND-SAME: "-o" "{{.*}}partial.o"
// OBJECT:      Format: elf64-mmix
// OBJECT:      Type: Relocatable
// OBJECT:      Entry: 0x0
// OBJECT:      ProgramHeaderCount: 0
// OBJECT:      Name: retained
// LTO: error: the clang compiler does not support 'LTO relocatable linking for MMIX'
// THIN: error: the clang compiler does not support 'ThinLTO linking for MMIX'
// LINKER: error: the clang compiler does not support 'non-lld linker selection for MMIX'
// RUNTIME: error: the clang compiler does not support 'runtime library selection for MMIX relocatable linking'
// PIC: error: the clang compiler does not support 'position-independent linking for MMIX'

int retained(void) { return 42; }
