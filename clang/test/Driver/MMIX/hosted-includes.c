// RUN: rm -rf %t.dir
// RUN: mkdir -p %t.dir/resource/include %t.dir/sysroot/usr/include \
// RUN:   %t.dir/explicit
// RUN: echo '#define MMIX_NEWLIB_HEADER 1' \
// RUN:   > %t.dir/sysroot/usr/include/mmix-hosted.h
// RUN: echo '#define MMIX_EXPLICIT_HEADER 1' \
// RUN:   > %t.dir/explicit/mmix-explicit.h
// RUN: %clang -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   --cstdlib=newlib -I %t.dir/explicit -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATHS \
// RUN:       --implicit-check-not='argument unused'
// RUN: %clang -### --target=mmix-unknown-elf \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -I %t.dir/explicit -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATHS
// RUN: %clang --target=mmix-unknown-unknown --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource -I %t.dir/explicit -fsyntax-only %s
// RUN: %clang --target=mmix-unknown-elf --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot \
// RUN:   -resource-dir=%t.dir/resource -I %t.dir/explicit -fsyntax-only %s

// RUN: %clang -### --target=mmix-unknown-unknown \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdinc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-INCLUDES \
// RUN:       --implicit-check-not=-internal-isystem
// RUN: %clang -### --target=mmix-unknown-unknown \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nobuiltininc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NEWLIB-ONLY \
// RUN:       --implicit-check-not=%t.dir/resource/include
// RUN: %clang -### --target=mmix-unknown-unknown \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/sysroot -resource-dir=%t.dir/resource \
// RUN:   -nostdlibinc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=RESOURCE-ONLY \
// RUN:       --implicit-check-not=%t.dir/sysroot/usr/include

// RUN: not %clang -### --target=mmix-unknown-unknown \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/missing -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-SYSROOT
// RUN: touch %t.dir/not-a-directory
// RUN: not %clang -### --target=mmix-unknown-unknown \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/not-a-directory -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MALFORMED-SYSROOT
// RUN: mkdir %t.dir/incomplete
// RUN: not %clang -### --target=mmix-unknown-unknown \
// RUN:   --cstdlib=newlib \
// RUN:   --sysroot=%t.dir/incomplete -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MISSING-HEADERS

// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -resource-dir=%t.dir/resource -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NO-INCLUDES \
// RUN:       --implicit-check-not=-isysroot \
// RUN:       --implicit-check-not=-internal-isystem

// PATHS: "-cc1" "-triple" "mmix-unknown-unknown"
// PATHS-SAME: "-I" "[[EXPLICIT:[^"]+]]"
// PATHS-SAME: "-isysroot" "[[SYSROOT:[^"]+]]"
// PATHS-SAME: "-internal-isystem" "[[RESOURCE:[^"]+]]"
// PATHS-SAME: "-internal-isystem" "[[SYSROOT]]{{/|\\}}usr{{/|\\}}include"
// PATHS-NOT: {{[/\\](gcc|include/c\+\+|usr/local/include)[/\\]}}

// NO-INCLUDES: "-cc1" "-triple" "mmix-unknown-unknown"
// NEWLIB-ONLY: "-internal-isystem" "{{[^"]+}}{{/|\\}}sysroot{{/|\\}}usr{{/|\\}}include"
// RESOURCE-ONLY: "-internal-isystem" "{{[^"]+}}{{/|\\}}resource{{/|\\}}include"

// MISSING-SYSROOT: error: no such sysroot directory: '{{.*}}missing'
// MALFORMED-SYSROOT: error: no such sysroot directory: '{{.*}}not-a-directory'
// MISSING-HEADERS: error: no such file or directory: '{{.*}}incomplete{{/|\\}}usr{{/|\\}}include'

#include <mmix-hosted.h>
#include <mmix-explicit.h>

#if !MMIX_NEWLIB_HEADER || !MMIX_EXPLICIT_HEADER
#error MMIX hosted include search order is incomplete
#endif
