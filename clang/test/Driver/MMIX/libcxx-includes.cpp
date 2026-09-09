// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: mkdir -p %t/resource/include/mmix-unknown-unknown/c++/v1
// RUN: cp %t/explicit/vector %t/explicit/__config_site %t/resource/include/mmix-unknown-unknown/c++/v1/
// RUN: %clangxx --target=mmix -resource-dir=%t/resource -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -stdlib=libc++ -resource-dir=%t/resource -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -stdlib=platform -resource-dir=%t/resource -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -stdlib=libstdc++ -stdlib=libc++ -resource-dir=%t/resource -fsyntax-only %t/input.cpp
// RUN: not %clangxx --target=mmix -stdlib=libstdc++ -fsyntax-only %t/empty.cpp 2>&1 | FileCheck %s --check-prefix=BAD-STDLIB
// RUN: not %clangxx --target=mmix -stdlib=invalid -fsyntax-only %t/empty.cpp 2>&1 | FileCheck %s --check-prefix=INVALID
// RUN: not %clangxx --target=mmix -stdlib=libstdc++ -nostdinc++ -fsyntax-only %t/empty.cpp 2>&1 | FileCheck %s --check-prefix=BAD-STDLIB
// RUN: not %clangxx --target=mmix --cstdlib=picolibc -fsyntax-only %t/empty.cpp 2>&1 | FileCheck %s --check-prefix=C-PROVIDER
// RUN: %clangxx --target=mmix --cstdlib=newlib -resource-dir=%t/resource -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -resource-dir=%t/resource -nostdinc++ -isystem %t/explicit -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -resource-dir=%t/resource -nostdinc -I %t/explicit -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -resource-dir=%t/resource -nostdlibinc -I %t/explicit -fsyntax-only %t/input.cpp
// RUN: %clangxx --target=mmix -resource-dir=%t/missing -stdlib++-isystem %t/explicit -fsyntax-only %t/input.cpp
// RUN: not %clangxx --target=mmix -resource-dir=%t/resource -nostdinc++ -fsyntax-only %t/input.cpp 2>&1 | FileCheck %s --check-prefix=MISSING
// RUN: not %clangxx --target=mmix -resource-dir=%t/resource -nostdinc -fsyntax-only %t/input.cpp 2>&1 | FileCheck %s --check-prefix=MISSING
// RUN: not %clangxx --target=mmix -resource-dir=%t/resource -nostdlibinc -fsyntax-only %t/input.cpp 2>&1 | FileCheck %s --check-prefix=MISSING
// RUN: not %clangxx --target=mmix -resource-dir=%t/missing -fsyntax-only %t/input.cpp 2>&1 | FileCheck %s --check-prefix=MISSING
// Headerless compilation does not require a C++ library installation.
// RUN: %clangxx --target=mmix -resource-dir=%t/missing -fsyntax-only %t/empty.cpp
// RUN: mv %t/resource %t/relocated
// RUN: %clangxx --target=mmix -resource-dir=%t/relocated -fsyntax-only %t/input.cpp
// RUN: rm %t/relocated/include/mmix-unknown-unknown/c++/v1/__config_site
// RUN: not %clangxx --target=mmix -resource-dir=%t/relocated -fsyntax-only %t/input.cpp 2>&1 | FileCheck %s --check-prefix=CONFIG

// BAD-STDLIB: error: unsupported option '-stdlib=libstdc++' for target 'mmix'
// INVALID: error: invalid library name in argument '-stdlib=invalid'
// C-PROVIDER: error: unsupported option '--cstdlib=picolibc' for target 'mmix'
// MISSING: fatal error: 'vector' file not found
// CONFIG: fatal error: '__config_site' file not found

//--- explicit/vector
#include <__config_site>
static_assert(MMIX_INSTALLED_CXX == 1, "wrong installed configuration");
//--- explicit/__config_site
#define MMIX_INSTALLED_CXX 1
//--- input.cpp
#include <vector>
//--- empty.cpp
int value;
