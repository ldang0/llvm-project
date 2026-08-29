// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/resource/include \
// RUN:   %t.dir/sysroot/include/mmix-unknown-unknown %t.dir/explicit
// RUN: echo '#define MMIX_LLVM_LIBC_HEADER 1' \
// RUN:   > %t.dir/sysroot/include/mmix-unknown-unknown/mmix-hosted.h
// RUN: echo '#define MMIX_EXPLICIT_HEADER 1' \
// RUN:   > %t.dir/explicit/mmix-explicit.h

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -I %t.dir/explicit -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATHS
// RUN: %clang -### --target=mmix-unknown-elf --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -I %t.dir/explicit -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATHS
// RUN: %clang --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -I %t.dir/explicit -fsyntax-only %s
// RUN: %clang --target=mmix-unknown-elf --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -I %t.dir/explicit -fsyntax-only %s

// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdinc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-INCLUDES \
// RUN:       --implicit-check-not=-internal-isystem
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nobuiltininc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LLVM-LIBC-ONLY \
// RUN:       --implicit-check-not=%t.dir/resource/include
// RUN: %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlibinc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=RESOURCE-ONLY \
// RUN:       --implicit-check-not=%t.dir/sysroot/include/mmix-unknown-unknown

// RUN: mkdir -p %t.dir/missing/usr/include %t.dir/missing/include
// RUN: echo '#define MMIX_LLVM_LIBC_HEADER 1' \
// RUN:   > %t.dir/missing/usr/include/mmix-hosted.h
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/missing -resource-dir=%t.dir/resource \
// RUN:   -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING \
// RUN:       --implicit-check-not='{{[/\\]usr[/\\]include}}'
// RUN: touch %t.dir/missing/include/mmix-unknown-unknown
// RUN: not %clang -### --target=mmix-unknown-unknown --cstdlib=llvm-libc \
// RUN:   --sysroot=%t.dir/missing -resource-dir=%t.dir/resource \
// RUN:   -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MALFORMED

// PATHS: "-cc1" "-triple" "mmix-unknown-unknown"
// PATHS-SAME: "-I" "[[EXPLICIT:[^"]+]]"
// PATHS-SAME: "-isysroot" "[[SYSROOT:[^"]+]]"
// PATHS-SAME: "-internal-isystem" "[[RESOURCE:[^"]+]]"
// PATHS-SAME: "-internal-isystem" "[[SYSROOT]]{{/|\\}}include{{/|\\}}mmix-unknown-unknown"
// PATHS-NOT: {{[/\\](usr/include|gcc|include/c\+\+|usr/local/include)[/\\]}}

// NO-INCLUDES: "-cc1" "-triple" "mmix-unknown-unknown"
// LLVM-LIBC-ONLY: "-internal-isystem" "{{[^"]+}}{{/|\\}}include{{/|\\}}mmix-unknown-unknown"
// RESOURCE-ONLY: "-internal-isystem" "{{[^"]+}}{{/|\\}}resource{{/|\\}}include"
// MISSING: error: no such file or directory: '{{.*}}missing{{/|\\}}include{{/|\\}}mmix-unknown-unknown'
// MALFORMED: error: no such file or directory: '{{.*}}missing{{/|\\}}include{{/|\\}}mmix-unknown-unknown'

#include <mmix-hosted.h>
#include <mmix-explicit.h>

#if !MMIX_LLVM_LIBC_HEADER || !MMIX_EXPLICIT_HEADER
#error MMIX LLVM libc include search order is incomplete
#endif
