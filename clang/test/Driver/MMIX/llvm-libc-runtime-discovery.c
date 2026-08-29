// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/sysroot/include/mmix-unknown-unknown \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown \
// RUN:   %t.dir/resource/include \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown \
// RUN:   %t.dir/explicit %t.dir/sysroot/usr/lib/mmix
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libm.a \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a \
// RUN:   %t.dir/explicit/custom.ld %t.dir/explicit/libm.a \
// RUN:   %t.dir/sysroot/usr/lib/mmix/libc.a

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LINK \
// RUN:       --implicit-check-not='{{[/\\]usr[/\\]lib[/\\]mmix}}' \
// RUN:       --implicit-check-not=libgloss --implicit-check-not=libgcc \
// RUN:       --implicit-check-not=libm

// RUN: mkdir -p %t.dir/no-runtime/include/mmix-unknown-unknown
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/no-runtime -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBRARY-DIRECTORY \
// RUN:       --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/no-runtime -resource-dir=%t.dir/resource \
// RUN:   -nostdlib -T %t.dir/explicit/custom.ld %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-RUNTIME \
// RUN:       --implicit-check-not='no such file or directory'
// RUN: %clang -### --target=mmix-unknown-elf --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LINK

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-CRT --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostartfiles %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-START \
// RUN:       --implicit-check-not=crt1.o
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-SCRIPT --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -T %t.dir/explicit/custom.ld %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SCRIPT \
// RUN:       --implicit-check-not=mmix-qemu.ld
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBC \
// RUN:       --implicit-check-not='{{[/\\]usr[/\\]lib[/\\]mmix}}' \
// RUN:       --implicit-check-not=ld.lld
// RUN: mkdir %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBC --implicit-check-not=ld.lld
// RUN: rmdir %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nodefaultlibs %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-LIBS \
// RUN:       --implicit-check-not=libc.a
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-PLATFORM \
// RUN:       --implicit-check-not=ld.lld
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -lm %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MATH \
// RUN:       --implicit-check-not='{{[/\\]usr[/\\]lib[/\\]mmix}}'

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/libm.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -lm %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-LIBM --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -L %t.dir/explicit -lm %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=EXPLICIT-MATH \
// RUN:       --implicit-check-not='{{[/\\]libm\.a}}'
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/libm.a

// RUN: rm %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   %s -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-BUILTINS \
// RUN:       --implicit-check-not=ld.lld

// LINK: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// LINK-SAME: "-L[[LIBDIR:[^"]+]]"
// LINK-SAME: "-T" "[[LIBDIR]]{{/|\\}}mmix-qemu.ld"
// LINK-SAME: "[[LIBDIR]]{{/|\\}}crt1.o"
// LINK-SAME: "{{[^"]+}}.o" "--start-group"
// LINK-SAME: "[[LIBDIR]]{{/|\\}}libc.a"
// LINK-SAME: "[[LIBDIR]]{{/|\\}}libmmixplatform.a"
// LINK-SAME: "{{[^"]+}}{{/|\\}}libclang_rt.builtins.a"
// LINK-SAME: "{{[^"]+}}{{/|\\}}libclang_rt.atomic.a"
// LINK-SAME: "{{[^"]+}}{{/|\\}}libclang_rt.stack_protector.a"
// LINK-SAME: "--end-group" "-o"
// NO-RUNTIME: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// NO-RUNTIME-SAME: "-T" "{{[^"]+}}{{/|\\}}custom.ld"
// NO-START: "{{.*}}ld.lld"
// NO-START-SAME: "--start-group"
// SCRIPT: "{{.*}}ld.lld"
// SCRIPT-SAME: "-T" "{{[^"]+}}{{/|\\}}custom.ld"
// NO-LIBS: "{{.*}}ld.lld"
// NO-LIBS-SAME: "{{[^"]+}}{{/|\\}}crt1.o"
// MATH: "{{.*}}ld.lld"
// MATH-SAME: "-L[[MATHDIR:[^"]+]]"
// MATH-SAME: "-lm"
// EXPLICIT-MATH: "{{.*}}ld.lld"
// EXPLICIT-MATH-SAME: "-L{{[^"]+}}{{/|\\}}explicit"
// EXPLICIT-MATH-SAME: "-lm"
// MISSING-LIBRARY-DIRECTORY: error: no such file or directory: '{{.*}}no-runtime{{/|\\}}lib{{/|\\}}mmix-unknown-unknown'
// MISSING-CRT: error: no such file or directory: '{{.*}}lib{{/|\\}}mmix-unknown-unknown{{/|\\}}crt1.o'
// MISSING-SCRIPT: error: no such file or directory: '{{.*}}lib{{/|\\}}mmix-unknown-unknown{{/|\\}}mmix-qemu.ld'
// MISSING-LIBC: error: no such file or directory: '{{.*}}lib{{/|\\}}mmix-unknown-unknown{{/|\\}}libc.a'
// MISSING-PLATFORM: error: no such file or directory: '{{.*}}lib{{/|\\}}mmix-unknown-unknown{{/|\\}}libmmixplatform.a'
// MISSING-LIBM: error: no such file or directory: '{{.*}}lib{{/|\\}}mmix-unknown-unknown{{/|\\}}libm.a'
// MISSING-BUILTINS: error: no such file or directory: '{{.*}}lib{{/|\\}}mmix-unknown-unknown{{/|\\}}libclang_rt.builtins.a'

int main(void) { return 0; }
