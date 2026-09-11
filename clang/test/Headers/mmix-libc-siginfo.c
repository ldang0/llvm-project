// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 -fsyntax-only -pedantic-errors %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -x c++ -std=c++17 -fsyntax-only -pedantic-errors %s
// RUN: %clang_cc1 -triple mmix-unknown-linux -std=c17 -fsyntax-only -pedantic-errors %s
// RUN: %clang_cc1 -triple mmix-unknown-linux -x c++ -std=c++17 -fsyntax-only -pedantic-errors %s

#include "../../../libc/test/include/siginfo_layout_test.h"
