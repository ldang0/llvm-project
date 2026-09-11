// REQUIRES: mmix-registered-target

// RUN: %clang_cc1 -triple mmix -fsyntax-only %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -fsyntax-only %s
// RUN: %clang_cc1 -triple mmix-none-none -fsyntax-only %s
// RUN: %clang_cc1 -triple mmix-unknown-none-none -fsyntax-only %s

// RUN: not %clang_cc1 -triple mmix-pc-unknown -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD-VENDOR
// RUN: not %clang_cc1 -triple mmix-unknown-freebsd -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD-OS
// RUN: not %clang_cc1 -triple mmix-unknown-unknown-elf -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD-ENV

// BAD-VENDOR: error: unknown target triple 'mmix-pc-unknown'
// BAD-OS: error: unknown target triple 'mmix-unknown-freebsd'
// BAD-ENV: error: unknown target triple 'mmix-unknown-unknown-elf'
