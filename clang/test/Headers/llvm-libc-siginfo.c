// Cross-target compile coverage for libc's Linux layout test, independent of
// which LLVM backends are built. The tests also run in libc's own test suite.
// RUN: %clang_cc1 -triple x86_64-unknown-linux -std=c17 -fsyntax-only -pedantic-errors %S/../../../libc/test/include/siginfo_layout_test.c
// RUN: %clang_cc1 -triple x86_64-unknown-linux -std=c++17 -fsyntax-only -pedantic-errors %S/../../../libc/test/include/siginfo_layout_test.cpp
// RUN: %clang_cc1 -triple i386-unknown-linux -std=c17 -fsyntax-only -pedantic-errors %S/../../../libc/test/include/siginfo_layout_test.c
// RUN: %clang_cc1 -triple i386-unknown-linux -std=c++17 -fsyntax-only -pedantic-errors %S/../../../libc/test/include/siginfo_layout_test.cpp
// RUN: %clang_cc1 -triple aarch64-unknown-linux -std=c17 -fsyntax-only -pedantic-errors %S/../../../libc/test/include/siginfo_layout_test.c
// RUN: %clang_cc1 -triple aarch64-unknown-linux -std=c++17 -fsyntax-only -pedantic-errors %S/../../../libc/test/include/siginfo_layout_test.cpp
