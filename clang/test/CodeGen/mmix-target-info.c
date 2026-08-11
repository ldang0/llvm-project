// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -o - %s | FileCheck %s

// CHECK: target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
// CHECK: target triple = "mmix-unknown-unknown"
