// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/provider.ll %t/provider.cpp
// RUN: FileCheck %s --check-prefix=IR < %t/provider.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer.o %t/consumer.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/runtime-stubs.o \
// RUN:   %t/runtime-stubs.cpp
// RUN: llvm-readobj --sections --symbols --relocations %t/provider.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: llvm-nm %t/provider.o | FileCheck %s --check-prefix=NM
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked %t/provider.o \
// RUN:   %t/consumer.o %t/runtime-stubs.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider-opt.o \
// RUN:   %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer-opt.o \
// RUN:   %t/consumer.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt %t/provider-opt.o \
// RUN:   %t/consumer-opt.o %t/runtime-stubs.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- virtual-structors.h
struct Root {
  long root;
  explicit Root(long);
  virtual ~Root();
  virtual long value() const;
};

struct Left : virtual Root {
  long left;
  explicit Left(long);
  ~Left() override;
  long value() const override;
};

struct Right : virtual Root {
  long right;
  explicit Right(long);
  ~Right() override;
  long value() const override;
};

struct Diamond : Left, Right {
  long own;
  explicit Diamond(long);
  ~Diamond() override;
  long value() const override;
};

extern "C" long c_entry(long);

//--- provider.cpp
#include "virtual-structors.h"

Root::Root(long input) : root(input) {}
Root::~Root() = default;
long Root::value() const { return root; }
Left::Left(long input) : Root(input), left(input + 1) {}
Left::~Left() = default;
long Left::value() const { return root + left; }
Right::Right(long input) : Root(input), right(input + 2) {}
Right::~Right() = default;
long Right::value() const { return root + right; }
Diamond::Diamond(long input)
    : Root(input), Left(input), Right(input), own(input + 3) {}
Diamond::~Diamond() = default;
long Diamond::value() const { return root + left + right + own; }

//--- consumer.cpp
#include "virtual-structors.h"

extern "C" long c_entry(long input) {
  Diamond object(input);
  return object.value();
}

//--- runtime-stubs.cpp
using size_t = decltype(sizeof(0));
void operator delete(void *, size_t) noexcept {}

// IR: @_ZTT7Diamond ={{.*}} constant [7 x ptr]
// IR-SAME: ptr {{.*}}@_ZTC7Diamond0_4Left
// IR-SAME: ptr {{.*}}@_ZTC7Diamond16_5Right
// IR: @_ZTC7Diamond0_4Left ={{.*}} constant
// IR: @_ZTC7Diamond16_5Right ={{.*}} constant

// IR-LABEL: define dso_local void @_ZN4LeftC2El(
// IR-SAME: ptr {{.*}}%this, ptr noundef %vtt, i64 {{.*}}%input)
// IR: load ptr, ptr %{{.*}}, align 8

// IR-LABEL: define dso_local void @_ZN7DiamondC1El(
// IR: call void @_ZN4RootC2El(
// IR: call void @_ZN4LeftC2El(
// IR-SAME: ptr noundef getelementptr inbounds nuw (i8, ptr @_ZTT7Diamond, i64 8)
// IR: call void @_ZN5RightC2El(
// IR-SAME: ptr noundef getelementptr inbounds nuw (i8, ptr @_ZTT7Diamond, i64 24)

// IR-LABEL: define dso_local void @_ZN7DiamondD2Ev(
// IR-SAME: ptr {{.*}}%this, ptr noundef %vtt)
// IR-LABEL: define dso_local void @_ZN7DiamondD1Ev(
// IR: call void @_ZN7DiamondD2Ev(
// IR-SAME: ptr noundef @_ZTT7Diamond)
// IR-LABEL: define dso_local void @_ZN7DiamondD0Ev(
// IR: call void @_ZN7DiamondD1Ev(
// IR: call void @_ZdlPvm(

// OBJECT: Name: .rodata
// OBJECT: AddressAlignment: 8
// OBJECT: R_MMIX_GETA _ZTT7Diamond 0x8
// OBJECT: R_MMIX_PUSHJ_STUBBABLE _ZN4LeftC2El 0x0
// OBJECT: R_MMIX_GETA _ZTT7Diamond 0x18
// OBJECT: R_MMIX_PUSHJ_STUBBABLE _ZN5RightC2El 0x0
// OBJECT: R_MMIX_64 _ZTC7Diamond0_4Left 0x18
// OBJECT: R_MMIX_64 _ZTC7Diamond16_5Right 0x18
// OBJECT-DAG: Name: _ZTT7Diamond
// OBJECT-DAG: Name: _ZTC7Diamond0_4Left
// OBJECT-DAG: Name: _ZTC7Diamond16_5Right

// NM-DAG: T _ZN7DiamondC1El
// NM-DAG: T _ZN7DiamondC2El
// NM-DAG: T _ZN7DiamondD0Ev
// NM-DAG: T _ZN7DiamondD1Ev
// NM-DAG: T _ZN7DiamondD2Ev
// NM-DAG: R _ZTT7Diamond
// NM-DAG: R _ZTC7Diamond0_4Left
// NM-DAG: R _ZTC7Diamond16_5Right
// NM-DAG: U _ZdlPvm
