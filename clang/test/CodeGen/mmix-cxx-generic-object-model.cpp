// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O0 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/member-data-pointers.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O2 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/member-data-pointers.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O0 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/member-function-pointer-calls.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O2 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/member-function-pointer-calls.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O0 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/cxx11-vtable-key-function.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O2 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/cxx11-vtable-key-function.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O0 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/constructors.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-everything -O2 -mrelocation-model static -emit-obj -o /dev/null \
// RUN:   %S/../CodeGenCXX/constructors.cpp

// This test intentionally contains only RUN lines. It selects unchanged
// target-independent Clang sources for the supported MMIX C++ profile.
