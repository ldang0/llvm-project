// REQUIRES: mmix-registered-target

// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/sysroot/include/mmix-unknown-unknown \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown \
// RUN:   %t.dir/resource/include \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown %t.dir/explicit
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libm.a \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a \
// RUN:   %t.dir/explicit/custom.ld %t.dir/explicit/first.o \
// RUN:   %t.dir/explicit/second.o

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -T %t.dir/explicit/custom.ld -L %t.dir/explicit \
// RUN:   %t.dir/explicit/first.o -lcustom %t.dir/explicit/second.o \
// RUN:   -o %t.dir/output 2>&1 | FileCheck %s --check-prefix=EXPLICIT \
// RUN:     --implicit-check-not=mmix-qemu.ld
// RUN: %clang -### --target=mmix-unknown-unknown -O2 -flto \
// RUN:   --cstdlib=llvm-libc --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource %s -o %t.dir/full-lto 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FULL-LTO \
// RUN:       --implicit-check-not='"-plugin"'

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostartfiles %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-START --implicit-check-not=crt1.o
// RUN: touch %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/libc.a \
// RUN:   %t.dir/sysroot/lib/mmix-unknown-unknown/libmmixplatform.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.builtins.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.atomic.a \
// RUN:   %t.dir/resource/lib/mmix-unknown-unknown/libclang_rt.stack_protector.a
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nodefaultlibs %t.dir/explicit/first.o -lcustom -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-LIBS \
// RUN:       --implicit-check-not=libc.a \
// RUN:       --implicit-check-not=libmmixplatform.a \
// RUN:       --implicit-check-not=libclang_rt

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/crt1.o
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlib %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-STDLIB \
// RUN:       --implicit-check-not=crt1.o --implicit-check-not=libc.a \
// RUN:       --implicit-check-not=libmmixplatform.a \
// RUN:       --implicit-check-not=libclang_rt

// RUN: rm %t.dir/sysroot/lib/mmix-unknown-unknown/mmix-qemu.ld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlib %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-SCRIPT \
// RUN:       --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlib -T %t.dir/explicit/custom.ld %t.dir/explicit/first.o \
// RUN:   -o %t.dir/output 2>&1 | FileCheck %s --check-prefix=EXPLICIT-ONLY \
// RUN:     --implicit-check-not='no such file or directory'

// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -shared %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SHARED --implicit-check-not=ld.lld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -dynamic %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DYNAMIC --implicit-check-not=ld.lld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -pie %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PIE --implicit-check-not=ld.lld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -fuse-ld=bfd %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LINKER --implicit-check-not=ld.lld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -rtlib=libgcc %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=RUNTIME --implicit-check-not=ld.lld
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -unwindlib=libgcc %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=UNWIND --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlib -T %t.dir/explicit/custom.ld -rtlib=compiler-rt \
// RUN:   -unwindlib=none %t.dir/explicit/first.o -o %t.dir/output 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FIXED-RUNTIMES \
// RUN:       --implicit-check-not='argument unused'

// EXPLICIT: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// EXPLICIT-SAME: "-L[[EXPLICIT_DIR:[^"]+]]"
// EXPLICIT-SAME: "-L{{[^"]+}}{{/|\\}}lib{{/|\\}}mmix-unknown-unknown"
// EXPLICIT-SAME: "-T" "[[EXPLICIT_DIR]]{{/|\\}}custom.ld"
// EXPLICIT-SAME: "[[EXPLICIT_DIR]]{{/|\\}}first.o" "-lcustom"
// EXPLICIT-SAME: "[[EXPLICIT_DIR]]{{/|\\}}second.o" "--start-group"
// FULL-LTO: "{{.*}}ld.lld" "-m" "elf64mmix" "-static"
// FULL-LTO-SAME: "-T" "[[LTO_LIBDIR:[^\"]+]]{{/|\\}}mmix-qemu.ld"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}crt1.o"
// FULL-LTO-SAME: "-plugin-opt=O2" "{{[^\"]+}}.o" "--start-group"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}libc.a"
// FULL-LTO-SAME: "[[LTO_LIBDIR]]{{/|\\}}libmmixplatform.a"
// FULL-LTO-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.builtins.a"
// FULL-LTO-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.atomic.a"
// FULL-LTO-SAME: "{{[^\"]+}}{{/|\\}}libclang_rt.stack_protector.a"
// FULL-LTO-SAME: "--end-group" "-o"
// NO-START: "{{.*}}ld.lld"
// NO-START-SAME: "{{[^"]+}}{{/|\\}}first.o" "--start-group"
// NO-LIBS: "{{.*}}ld.lld"
// NO-LIBS-SAME: "{{[^"]+}}{{/|\\}}crt1.o"
// NO-LIBS-SAME: "{{[^"]+}}{{/|\\}}first.o" "-lcustom" "-o"
// NO-STDLIB: "{{.*}}ld.lld"
// NO-STDLIB-SAME: "-T" "{{[^"]+}}{{/|\\}}mmix-qemu.ld"
// NO-STDLIB-SAME: "{{[^"]+}}{{/|\\}}first.o" "-o"
// EXPLICIT-ONLY: "{{.*}}ld.lld"
// EXPLICIT-ONLY-SAME: "-T" "{{[^"]+}}{{/|\\}}custom.ld"
// EXPLICIT-ONLY-SAME: "{{[^"]+}}{{/|\\}}first.o" "-o"
// FIXED-RUNTIMES: "{{.*}}ld.lld"
// FIXED-RUNTIMES-SAME: "-T" "{{[^"]+}}{{/|\\}}custom.ld"
// SHARED: error: the clang compiler does not support 'shared linking for MMIX'
// DYNAMIC: error: the clang compiler does not support 'dynamic linking for MMIX'
// PIE: error: the clang compiler does not support 'PIE linking for MMIX'
// LINKER: error: the clang compiler does not support 'non-lld linker selection for MMIX'
// RUNTIME: error: the clang compiler does not support 'non-compiler-rt runtime selection for MMIX'
// UNWIND: error: the clang compiler does not support 'unwind library selection for MMIX'
// MISSING-SCRIPT: error: no such file or directory: '{{.*}}mmix-qemu.ld'

int main(void) { return 0; }
