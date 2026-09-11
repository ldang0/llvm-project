// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: mkdir -p %t/root/usr/lib %t/resource/include %t/resource/lib/mmix-unknown-linux
// RUN: touch %t/root/usr/lib/crt1.o %t/root/usr/lib/libc.a %t/root/usr/lib/libc++.a %t/root/usr/lib/libc++abi.a %t/root/usr/lib/libunwind.a
// RUN: touch %t/resource/lib/mmix-unknown-linux/libclang_rt.builtins.a
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=HEADERS
// RUN: %clang --target=mmix-unknown-linux-unknown --sysroot=%t/root -resource-dir %t/resource -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=HEADERS
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -fsyntax-only %t/probe.cpp
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -nobuiltininc -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=NO-BUILTIN --implicit-check-not=resource/include
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -nostdinc++ -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=NO-CXX --implicit-check-not=include/c++
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -nostdlibinc -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=BUILTIN --implicit-check-not=root/usr/include
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/missing -resource-dir %t/missing-resource -nostdinc -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=NONE --implicit-check-not=-internal-isystem --implicit-check-not=-internal-externc-isystem --implicit-check-not=error:
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/missing-resource -nobuiltininc -fsyntax-only %t/probe.cpp
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/missing -resource-dir %t/resource -nostdlibinc -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=BUILTIN --implicit-check-not=error:
// RUN: %clang --target=mmix-unknown-linux -resource-dir %t/resource -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=BUILTIN --implicit-check-not=root/usr/include --implicit-check-not=include/c++
// RUN: not %clang --target=mmix-unknown-linux --sysroot=%t/missing -resource-dir %t/resource -nostdinc++ -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=MISSING-INCLUDE
// RUN: not %clang --target=mmix-unknown-linux -resource-dir %t/missing-resource -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=MISSING-BUILTIN
// RUN: not %clang --target=mmix-unknown-linux --sysroot=%t/no-config -resource-dir %t/resource -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=MISSING-CONFIG
// RUN: mkdir -p %t/no-config/usr/include/c++/v1/__config_site
// RUN: not %clang --target=mmix-unknown-linux --sysroot=%t/no-config -resource-dir %t/resource -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=MISSING-CONFIG
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/no-config -resource-dir %t/resource -nostdinc++ -### -c %t/probe.cpp 2>&1 | FileCheck %s --check-prefix=BUILTIN --implicit-check-not=error:
// RUN: %clang --target=mmix-unknown-linux -resource-dir %t/resource -print-libgcc-file-name | FileCheck %s --check-prefix=RT
// RUN: %clang --target=mmix-unknown-linux-unknown -resource-dir %t/resource -print-libgcc-file-name | FileCheck %s --check-prefix=RT
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -print-file-name=crt1.o | FileCheck %s --check-prefix=CRT
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -print-file-name=libc.a | FileCheck %s --check-prefix=LIBC
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -print-file-name=libc++.a | FileCheck %s --check-prefix=LIBCXX
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -print-file-name=libc++abi.a | FileCheck %s --check-prefix=ABI
// RUN: %clang --target=mmix-unknown-linux --sysroot=%t/root -resource-dir %t/resource -print-file-name=libunwind.a | FileCheck %s --check-prefix=UNWIND
// RUN: not %clang --target=mmix-unknown-linux -resource-dir %t/decoy -print-libgcc-file-name 2>&1 | FileCheck %s --check-prefix=MISSING-RT
// RUN: not %clang --target=mmix-unknown-linux-unknown -resource-dir %t/decoy -print-libgcc-file-name 2>&1 | FileCheck %s --check-prefix=MISSING-RT
// RUN: mkdir -p %t/decoy/lib/mmix-unknown-linux/libclang_rt.builtins.a
// RUN: not %clang --target=mmix-unknown-linux -resource-dir %t/decoy -print-libgcc-file-name 2>&1 | FileCheck %s --check-prefix=MISSING-RT

// Archives and CRT objects are path fixtures, not linkable runtime inputs.
// General -print-file-name queries do not validate mandatory linker inputs.
// HEADERS: "-internal-isystem" "{{.*}}/root/usr/include/c++/v1" "-internal-isystem" "{{.*}}/resource/include" "-internal-externc-isystem" "{{.*}}/root/usr/include"
// NO-BUILTIN: "-internal-isystem" "{{.*}}/root/usr/include/c++/v1" "-internal-externc-isystem" "{{.*}}/root/usr/include"
// NO-CXX: "-internal-isystem" "{{.*}}/resource/include" "-internal-externc-isystem" "{{.*}}/root/usr/include"
// BUILTIN: "-internal-isystem" "{{.*}}/resource/include"
// NONE: "-cc1"
// MISSING-INCLUDE: error: no such file or directory: '{{.*}}/missing/usr/include'
// MISSING-BUILTIN: error: no such file or directory: '{{.*}}/missing-resource/include'
// MISSING-CONFIG: error: no such file or directory: '{{.*}}/no-config/usr/include/c++/v1/__config_site'
// RT: {{^.*}}/resource/lib/mmix-unknown-linux/libclang_rt.builtins.a{{$}}
// CRT: {{^.*}}/root/usr/lib/crt1.o{{$}}
// LIBC: {{^.*}}/root/usr/lib/libc.a{{$}}
// LIBCXX: {{^.*}}/root/usr/lib/libc++.a{{$}}
// ABI: {{^.*}}/root/usr/lib/libc++abi.a{{$}}
// UNWIND: {{^.*}}/root/usr/lib/libunwind.a{{$}}
// MISSING-RT: error: no such file or directory: '{{.*}}/decoy/lib/mmix-unknown-linux/libclang_rt.builtins.a'

//--- probe.cpp
#include <__config_site>
#include <linux_probe.h>
static_assert(MMIX_LINUX_CONFIG == 1 && MMIX_LINUX_HEADERS == 1, "Linux headers");
//--- root/usr/include/c++/v1/__config_site
#define MMIX_LINUX_CONFIG 1
//--- root/usr/include/linux_probe.h
#define MMIX_LINUX_HEADERS 1
//--- no-config/usr/include/linux_probe.h
#define MMIX_LINUX_HEADERS 1
//--- resource/include/c++/v1/__config_site
#error resource-directory libc++ headers must not be used
//--- resource/include/mmix-unknown-unknown/c++/v1/__config_site
#error bare-metal libc++ headers must not be used
//--- decoy/lib/mmix-unknown-unknown/libclang_rt.builtins.a
bare-metal
//--- decoy/lib/mmix-unknown-linux-unknown/libclang_rt.builtins.a
alias
//--- decoy/lib/linux/libclang_rt.builtins-mmix.a
legacy
