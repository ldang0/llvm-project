// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/sysroot/usr/include %t.dir/sysroot/usr/lib/mmix \
// RUN:   %t.dir/resource/include \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown %t.dir/explicit \
// RUN:   %t.dir/host/bin %t.dir/host/lib
// RUN: touch %t.dir/sysroot/usr/lib/mmix/crt0.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/trip-vectors.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/crti.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/crtn.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/libc.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/libm.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/libgloss.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/mmix-qemu.ld \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a \
// RUN:   %t.dir/explicit/custom-start.o %t.dir/explicit/custom.ld \
// RUN:   %t.dir/explicit/libcustom.a
// RUN: touch %t.dir/host/bin/gcc %t.dir/host/bin/ld \
// RUN:   %t.dir/host/lib/libm.a

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/canonical 2>&1 \
// RUN:   | FileCheck %s --check-prefix=HOSTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not='{{[/\\](gcc|libgcc|ld.bfd)[^/\\"]*}}' \
// RUN:       --implicit-check-not=libm.a
// RUN: %clang -### --target=mmix-unknown-elf --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/explicit-elf 2>&1 \
// RUN:   | FileCheck %s --check-prefix=HOSTED \
// RUN:       --implicit-check-not='{{[/\\](gcc|libgcc|ld.bfd)[^/\\"]*}}'
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/freestanding-hosted 2>&1 \
// RUN:   | FileCheck %s --check-prefix=HOSTED
// RUN: %clang -### --target=mmix-unknown-unknown -O2 -flto \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/full-lto 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FULL-LTO \
// RUN:       --implicit-check-not='"-plugin"'

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -lm %s -o %t.dir/math 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MATH
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -T %t.dir/explicit/custom.ld %s -o %t.dir/script 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SCRIPT \
// RUN:       --implicit-check-not=mmix-qemu.ld

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostartfiles %t.dir/explicit/custom-start.o -o %t.dir/no-start 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-START \
// RUN:       --implicit-check-not='{{[/\\](crt0|crti|crtn|trip-vectors)\.o}}'
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nodefaultlibs %s -o %t.dir/no-default-libs 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-LIBS \
// RUN:       --implicit-check-not='{{[/\\](libc|libgloss|libclang_rt\.[^/\\]+)\.a}}'
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlib %s -o %t.dir/no-stdlib 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-STDLIB \
// RUN:       --implicit-check-not='{{[/\\](crt0|crti|crtn|trip-vectors)\.o}}' \
// RUN:       --implicit-check-not='{{[/\\](libc|libgloss|libclang_rt\.[^/\\]+)\.a}}'
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlib -T %t.dir/explicit/custom.ld \
// RUN:   -L %t.dir/explicit %t.dir/explicit/custom-start.o -lcustom \
// RUN:   -o %t.dir/explicit-inputs 2>&1 \
// RUN:   | FileCheck %s --check-prefix=EXPLICIT

// RUN: rm %t.dir/sysroot/usr/lib/mmix/crti.o \
// RUN:   %t.dir/sysroot/usr/lib/mmix/crtn.o
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/no-fragments 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-FRAGMENTS \
// RUN:       --implicit-check-not='{{[/\\](crti|crtn)\.o}}'

// RUN: rm %t.dir/sysroot/usr/lib/mmix/crt0.o
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-crt0 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-CRT0 --implicit-check-not=ld.lld
// RUN: touch %t.dir/sysroot/usr/lib/mmix/crt0.o
// RUN: rm %t.dir/sysroot/usr/lib/mmix/trip-vectors.o
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-vectors 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-VECTORS --implicit-check-not=ld.lld
// RUN: touch %t.dir/sysroot/usr/lib/mmix/trip-vectors.o
// RUN: rm %t.dir/sysroot/usr/lib/mmix/mmix-qemu.ld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-script 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-SCRIPT --implicit-check-not=ld.lld
// RUN: touch %t.dir/sysroot/usr/lib/mmix/mmix-qemu.ld
// RUN: rm %t.dir/sysroot/usr/lib/mmix/libc.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-libc 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBC --implicit-check-not=ld.lld
// RUN: touch %t.dir/sysroot/usr/lib/mmix/libc.a
// RUN: rm %t.dir/sysroot/usr/lib/mmix/libgloss.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-libgloss 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBGLOSS --implicit-check-not=ld.lld
// RUN: touch %t.dir/sysroot/usr/lib/mmix/libgloss.a
// RUN: rm %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-runtime 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-BUILTINS \
// RUN:       --implicit-check-not=ld.lld
// RUN: touch %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a
// RUN: rm %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-runtime 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-ATOMIC \
// RUN:       --implicit-check-not=ld.lld
// RUN: touch %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a
// RUN: rm %t.dir/sysroot/usr/lib/mmix/libm.a
// RUN: env LIBRARY_PATH=%t.dir/host/lib COMPILER_PATH=%t.dir/host/bin \
// RUN:   not %clang --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -lm %s -o %t.dir/missing-libm 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBM \
// RUN:       --implicit-check-not='%t.dir/host' --implicit-check-not=ld.lld
// RUN: test ! -e %t.dir/missing-libm
// RUN: touch %t.dir/sysroot/usr/lib/mmix/libm.a
// RUN: rm %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/missing-runtime 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-STACK-PROTECTOR \
// RUN:       --implicit-check-not=ld.lld

// HOSTED: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// HOSTED-SAME: "-L[[LIBDIR:[^"]+]]"
// HOSTED-SAME: "-T" "[[LIBDIR]]{{/|\\}}mmix-qemu.ld"
// HOSTED-SAME: "[[LIBDIR]]{{/|\\}}crt0.o"
// HOSTED-SAME: "[[LIBDIR]]{{/|\\}}trip-vectors.o"
// HOSTED-SAME: "[[LIBDIR]]{{/|\\}}crti.o"
// HOSTED-SAME: "{{[^"]+}}.o"
// HOSTED-SAME: "--start-group"
// HOSTED-SAME: "[[LIBDIR]]{{/|\\}}libc.a"
// HOSTED-SAME: "[[LIBDIR]]{{/|\\}}libgloss.a"
// HOSTED-SAME: "{{[^"]+}}{{/|\\}}libclang_rt.builtins.a"
// HOSTED-SAME: "{{[^"]+}}{{/|\\}}libclang_rt.atomic.a"
// HOSTED-SAME: "{{[^"]+}}{{/|\\}}libclang_rt.stack_protector.a"
// HOSTED-SAME: "--end-group"
// HOSTED-SAME: "[[LIBDIR]]{{/|\\}}crtn.o"
// HOSTED-SAME: "-o"

// FULL-LTO: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// FULL-LTO-SAME: "-T" "[[LTO_LIBDIR:[^\"]+]]{{/|\\}}mmix-qemu.ld"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}crt0.o"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}trip-vectors.o"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}crti.o"
// FULL-LTO-SAME: "-plugin-opt=O2" "{{[^\"]+}}.o" "--start-group"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}libc.a"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}libgloss.a"
// FULL-LTO-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.builtins.a"
// FULL-LTO-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.atomic.a"
// FULL-LTO-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.stack_protector.a"
// FULL-LTO-SAME: "--end-group" "[[LTO_LIBDIR]]{{/|\\}}crtn.o" "-o"

// MATH: "{{.*}}ld.lld"
// MATH-SAME: "-lm" "{{[^"]+}}.o" "--start-group"
// SCRIPT: "{{.*}}ld.lld"
// SCRIPT-SAME: "-T" "{{[^"]+}}{{/|\\}}custom.ld"
// NO-START: "{{.*}}ld.lld"
// NO-START-SAME: "{{[^"]+}}{{/|\\}}custom-start.o"
// NO-START-SAME: "--start-group"
// NO-LIBS: "{{.*}}ld.lld"
// NO-LIBS-SAME: "{{[^"]+}}{{/|\\}}crt0.o"
// NO-STDLIB: "{{.*}}ld.lld"
// NO-STDLIB-SAME: "-T" "{{[^"]+}}{{/|\\}}mmix-qemu.ld"
// EXPLICIT: "{{.*}}ld.lld"
// EXPLICIT-SAME: "-L{{[^"]+}}{{/|\\}}explicit"
// EXPLICIT-SAME: "-L{{[^"]+}}{{/|\\}}sysroot{{/|\\}}usr{{/|\\}}lib{{/|\\}}mmix"
// EXPLICIT-SAME: "-T" "{{[^"]+}}{{/|\\}}custom.ld"
// EXPLICIT-SAME: "{{[^"]+}}{{/|\\}}custom-start.o" "-lcustom"
// NO-FRAGMENTS: "{{.*}}ld.lld"
// NO-FRAGMENTS-SAME: "{{[^"]+}}{{/|\\}}crt0.o"
// NO-FRAGMENTS-SAME: "{{[^"]+}}{{/|\\}}trip-vectors.o"

// MISSING-CRT0: error: no such file or directory: '{{.*}}crt0.o'
// MISSING-VECTORS: error: no such file or directory: '{{.*}}trip-vectors.o'
// MISSING-SCRIPT: error: no such file or directory: '{{.*}}mmix-qemu.ld'
// MISSING-LIBC: error: no such file or directory: '{{.*}}libc.a'
// MISSING-LIBGLOSS: error: no such file or directory: '{{.*}}libgloss.a'
// MISSING-BUILTINS: error: no such file or directory: '{{.*}}libclang_rt.builtins.a'
// MISSING-ATOMIC: error: no such file or directory: '{{.*}}libclang_rt.atomic.a'
// MISSING-LIBM: error: no such file or directory: '{{.*}}libm.a'
// MISSING-STACK-PROTECTOR: error: no such file or directory: '{{.*}}libclang_rt.stack_protector.a'

int main(void) { return 0; }
