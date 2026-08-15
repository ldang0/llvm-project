// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -c \
// RUN:   %S/Inputs/freestanding.c -o %t-input.o
// RUN: touch %t-library.a %t-script.ld
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs -fuse-ld=lld \
// RUN:   %t-input.o %t-library.a -e mmix_driver_fixture \
// RUN:   -Wl,--gc-sections -T %t-script.ld -o %t 2>&1 \
// RUN:   | FileCheck --check-prefix=STATIC %s \
// RUN:       --implicit-check-not=crt --implicit-check-not=libclang_rt \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs \
// RUN:   %t-input.o -e mmix_driver_fixture -o %t-executable
// RUN: llvm-readobj --file-headers %t-executable \
// RUN:   | FileCheck --check-prefix=ELF %s
// RUN: not %clang -### --target=mmix-unknown-unknown %t-input.o \
// RUN:   -o %t 2>&1 | FileCheck --check-prefix=LINK %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding -static \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 \
// RUN:   | FileCheck --check-prefix=RUNTIME %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -r \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 \
// RUN:   | FileCheck --check-prefix=RELOCATABLE %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -shared \
// RUN:   %S/Inputs/freestanding.c -o %t.so 2>&1 \
// RUN:   | FileCheck --check-prefix=SHARED %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -pie \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 \
// RUN:   | FileCheck --check-prefix=PIE %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs -fPIC \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 \
// RUN:   | FileCheck --check-prefix=PIC %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs -rdynamic \
// RUN:   %t-input.o -o %t 2>&1 | FileCheck --check-prefix=DYNAMIC %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t-sysroot -nostdlib -nostartfiles -nodefaultlibs \
// RUN:   -rtlib=compiler-rt -fuse-ld=lld %S/Inputs/freestanding.c \
// RUN:   -ffreestanding -o %t 2>&1 | FileCheck --check-prefix=SYSROOT %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs -rtlib=compiler-rt \
// RUN:   %t-input.o -o %t 2>&1 | FileCheck --check-prefix=RUNTIME-LIB %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -fno-integrated-as -c \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 \
// RUN:   | FileCheck --check-prefix=AS %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -flto \
// RUN:   -ffreestanding -nostdlib -nostartfiles -nodefaultlibs \
// RUN:   %S/Inputs/freestanding.c -o %t-lto 2>&1 \
// RUN:   | FileCheck --check-prefix=LTO %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -flto=thin \
// RUN:   -ffreestanding -nostdlib -nostartfiles -nodefaultlibs \
// RUN:   %S/Inputs/freestanding.c -o %t-thinlto 2>&1 \
// RUN:   | FileCheck --check-prefix=LTO %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs -fuse-ld=bfd \
// RUN:   %t-input.o -o %t 2>&1 | FileCheck --check-prefix=LINKER %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'

// STATIC: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// STATIC-SAME: "-T" "{{.*}}script.ld"
// STATIC-SAME: "{{.*}}input.o" "{{.*}}library.a"
// STATIC-SAME: "-e" "mmix_driver_fixture"
// STATIC-SAME: "--gc-sections"
// STATIC-SAME: "-o" "{{.*}}"

// ELF: Format: elf64-mmix
// ELF: Type: Executable
// ELF: Machine: EM_MMIX

// LINK: error: the clang compiler does not support 'implicit hosted linking for MMIX'
// RUNTIME: error: the clang compiler does not support 'implicit runtime files for MMIX freestanding linking'
// RELOCATABLE: error: the clang compiler does not support 'relocatable linking for MMIX'
// SHARED: error: the clang compiler does not support 'shared linking for MMIX'
// DYNAMIC: error: the clang compiler does not support 'dynamic linking for MMIX'
// PIE: error: the clang compiler does not support 'PIE linking for MMIX'
// PIC: error: the clang compiler does not support 'position-independent linking for MMIX'
// SYSROOT: error: the clang compiler does not support 'sysroot selection for MMIX freestanding linking'
// RUNTIME-LIB: error: the clang compiler does not support 'runtime library selection for MMIX freestanding linking'
// LTO: error: the clang compiler does not support 'LTO linking for MMIX'
// LINKER: error: the clang compiler does not support 'non-lld linker selection for MMIX'

// AS: error: the clang compiler does not support 'external assembly for MMIX'
