// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -o - %S/../CodeGenCXX/dynamic-cast-hint.cpp | FileCheck %S/../CodeGenCXX/dynamic-cast-hint.cpp --check-prefixes=CHECK,64BIT
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -Wno-unused-value -emit-llvm -o - %S/../CodeGenCXX/typeid-most-derived.cpp | FileCheck %S/../CodeGenCXX/typeid-most-derived.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-obj -o %t.cast.o %S/../CodeGenCXX/dynamic-cast-hint.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -Wno-unused-value -emit-obj -o %t.typeid.o %S/../CodeGenCXX/typeid-most-derived.cpp

// Reuse the generic pointer-width and evaluation checks on the MMIX ABI.
