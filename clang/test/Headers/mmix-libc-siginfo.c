// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 -fsyntax-only -pedantic-errors %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -x c++ -std=c++17 -fsyntax-only -pedantic-errors %s
// Linux target admission is not yet available for MMIX. Defining __linux__
// selects only the header branch; this is not a Linux compile/link test.
// RUN: %clang_cc1 -triple mmix-unknown-unknown -D__linux__ -std=c17 -fsyntax-only -pedantic-errors %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -D__linux__ -x c++ -std=c++17 -fsyntax-only -pedantic-errors %s

#include "../../../libc/test/include/siginfo_layout_test.h"
