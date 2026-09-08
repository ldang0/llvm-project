// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/sysroot/include/mmix-unknown-unknown \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown \
// RUN:   %t.dir/sysroot/usr/include %t.dir/sysroot/usr/lib/mmix \
// RUN:   %t.dir/resource/include \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/crt0.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/trip-vectors.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/libc.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/libgloss.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/mmix-qemu.ld \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libc++abi.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/clang_rt.crtdso.o \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a
// RUN: echo 'int value;' > %t.dir/input.c

// RUN: %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti %s -o %t.dir/llvm-libc 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LLVM-LIBC
// RUN: %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti %s -o %t.dir/llvm-libc 2>&1 \
// RUN:   | grep -o libc++abi.a | count 1
// RUN: %clangxx -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti %s -o %t.dir/newlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NEWLIB

// RUN: %clang -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %t.dir/input.c -o %t.dir/c-link 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-CXX-RUNTIME
// RUN: %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti -c %s -o %t.dir/compile-only.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-CXX-RUNTIME
// RUN: %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti -nostdlib %s -o %t.dir/nostdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-CXX-RUNTIME
// RUN: %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti -nodefaultlibs %s \
// RUN:   -o %t.dir/nodefaultlibs 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-CXX-RUNTIME
// RUN: %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti -nostdlib++ %s \
// RUN:   -o %t.dir/nostdlibxx 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-CXX-RUNTIME

// RUN: %clangxx -### --target=mmix --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource -nostartfiles %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-START
// NO-START: "{{.*}}ld.lld"
// NO-START-NOT: clang_rt.crtdso.o
// NO-START: libc++abi.a
// NO-START-NOT: clang_rt.crtdso.o
// RUN: %clangxx -### --target=mmix -resource-dir=%t.dir/resource -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=HEADERS
// HEADERS: "-internal-isystem" "{{.*}}resource{{/|\\}}include{{/|\\}}mmix-unknown-unknown{{/|\\}}c++{{/|\\}}v1"
// RUN: %clangxx -### --target=mmix -resource-dir=%t.dir/resource -nostdinc++ -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-HEADERS
// NO-HEADERS-NOT: c++{{/|\\}}v1
// RUN: rm %t.dir/resource/lib/mmix-unknown-unknown/clang_rt.crtdso.o
// RUN: %clangxx -### --target=mmix -resource-dir=%t.dir/resource -nostdinc -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-HEADERS
// RUN: %clangxx -### --target=mmix -resource-dir=%t.dir/resource -nostdlibinc -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-HEADERS
// RUN: %clangxx -### --target=mmix --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource -nostartfiles -nostdlib++ %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-CXX-RUNTIME
// RUN: not %clangxx -### --target=mmix --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-DSO --implicit-check-not=ld.lld
// MISSING-DSO: error: no such file or directory: '{{.*}}clang_rt.crtdso.o'
// RUN: touch %t.dir/resource/lib/mmix-unknown-unknown/clang_rt.crtdso.o

// RUN: touch %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.cxx.a
// RUN: rm %t.dir/resource/lib/mmix-unknown-unknown/libc++abi.a
// RUN: not %clangxx -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fno-exceptions -fno-rtti %s -o %t.dir/missing 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING \
// RUN:       --implicit-check-not=ld.lld

// LLVM-LIBC:      "{{.*}}ld.lld"
// LLVM-LIBC-SAME: "{{[^\"]+}}clang_rt.crtdso.o"
// LLVM-LIBC-SAME: "{{[^\"]+}}{{/|\\}}libc++abi.a"
// LLVM-LIBC-SAME: "[[LIBC:[^\"]+]]{{/|\\}}libc.a"
// LLVM-LIBC-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.builtins.a"

// NEWLIB:      "{{.*}}ld.lld"
// NEWLIB-SAME: "{{[^\"]+}}clang_rt.crtdso.o"
// NEWLIB-SAME: "{{[^\"]+}}{{/|\\}}libc++abi.a"
// NEWLIB-SAME: "[[NEWLIB:[^\"]+]]{{/|\\}}libc.a"
// NEWLIB-SAME: "[[NEWLIB]]{{/|\\}}libgloss.a"
// NEWLIB-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.builtins.a"

// NO-CXX-RUNTIME-NOT: libc++abi.a
// NO-CXX-RUNTIME-NOT: clang_rt.crtdso.o

// MISSING: error: no such file or directory: '{{.*}}libc++abi.a'

int main() { return 0; }
