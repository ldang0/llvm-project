// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/resource/lib/mmix-unknown-unknown
// RUN: touch %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a
// RUN: %clang --target=mmix-unknown-unknown \
// RUN:   -resource-dir=%t.dir/resource -print-libgcc-file-name > %t.canonical
// RUN: %clang --target=mmix-unknown-elf \
// RUN:   -resource-dir=%t.dir/resource -print-libgcc-file-name > %t.elf
// RUN: diff %t.canonical %t.elf
// RUN: FileCheck %s --check-prefix=RESOURCE < %t.canonical

// RUN: not %clang --target=mmix-unknown-unknown \
// RUN:   -resource-dir=%t.dir/missing -print-libgcc-file-name 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING \
// RUN:       --implicit-check-not='{{[/\\](gcc|libgcc)[^/\\]*}}'
// RUN: not %clang --target=mmix-unknown-unknown \
// RUN:   -resource-dir=%t.dir/resource -rtlib=libgcc \
// RUN:   -print-libgcc-file-name 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LIBGCC

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -c %s -o %t.o
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -resource-dir=%t.dir/resource \
// RUN:   -nostdlib -nostartfiles -nodefaultlibs %t.o -o %t 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-RUNTIME \
// RUN:       --implicit-check-not=libclang_rt \
// RUN:       --implicit-check-not='{{[/\\](gcc|libgcc)[^/\\]*}}'

// RESOURCE: {{.*}}resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a
// MISSING: error: no such file or directory: '{{.*}}missing/lib/mmix-unknown-unknown/libclang_rt.builtins.a'
// LIBGCC: error: the clang compiler does not support 'non-compiler-rt runtime selection for MMIX'
// NO-RUNTIME: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"

int resource_fixture;
