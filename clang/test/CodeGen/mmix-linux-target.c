// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-linux -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-linux-unknown -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-linux -mrelocation-model static -emit-obj -o %t.o %s
// RUN: llvm-readobj --file-headers %t.o | FileCheck %s --check-prefix=ELF
// Direct frontend object generation does not qualify Linux Driver linking,
// resource discovery, process startup or kernel-facing register conventions.
// CHECK: target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
// CHECK: define dso_local i8 @narrow(i8 noundef signext
// CHECK: define{{.*}} i64 @word(i64
// ELF: Format: elf64-mmix
// ELF: DataEncoding: BigEndian
// ELF: Type: Relocatable
// ELF: Machine: EM_MMIX

signed char narrow(signed char value) { return value; }
long word(long value) { return value + 1; }
