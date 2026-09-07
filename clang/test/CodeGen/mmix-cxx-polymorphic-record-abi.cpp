// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O0 -mrelocation-model static -emit-llvm \
// RUN:   -o %t/provider.ll %t/provider.cpp
// RUN: FileCheck %s --check-prefixes=IR,IR-O0 < %t/provider.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O2 -mrelocation-model static -emit-llvm \
// RUN:   -o %t/provider-opt.ll %t/provider.cpp
// RUN: FileCheck %s --check-prefix=IR < %t/provider-opt.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O0 -mrelocation-model static -emit-llvm \
// RUN:   -o %t/consumer.ll %t/consumer.cpp
// RUN: FileCheck %s --check-prefix=CALL < %t/consumer.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O2 -mrelocation-model static -emit-llvm \
// RUN:   -o %t/consumer-opt.ll %t/consumer.cpp
// RUN: FileCheck %s --check-prefix=CALL < %t/consumer-opt.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O0 -mrelocation-model static -emit-obj \
// RUN:   -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O0 -mrelocation-model static -emit-obj \
// RUN:   -o %t/consumer.o %t/consumer.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/runtime-stubs.o \
// RUN:   %t/runtime-stubs.cpp
// RUN: llvm-readobj --symbols --relocations %t/provider.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked %t/provider.o \
// RUN:   %t/consumer.o %t/runtime-stubs.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O2 -mrelocation-model static -emit-obj \
// RUN:   -o %t/provider-opt.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -Wno-return-type-c-linkage -O2 -mrelocation-model static -emit-obj \
// RUN:   -o %t/consumer-opt.o %t/consumer.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt %t/provider-opt.o \
// RUN:   %t/consumer-opt.o %t/runtime-stubs.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- polymorphic-record-abi.h
struct Poly {
  long value;
  explicit Poly(long);
  Poly(const Poly &);
  virtual ~Poly();
  virtual long read() const;
};

struct Left {
  virtual Poly apply(Poly) const = 0;
};

struct Right {
  virtual Poly apply(Poly) const = 0;
};

struct Impl : Left, Right {
  Poly apply(Poly) const override;
};

struct VirtualRecord : virtual Poly {
  long extra;
  explicit VirtualRecord(long);
  VirtualRecord(const VirtualRecord &);
  ~VirtualRecord() override;
};

Poly free_roundtrip(Poly);
extern "C" Poly c_roundtrip(Poly);
VirtualRecord virtual_roundtrip(VirtualRecord);
Poly call_right(Right *, Poly);
extern "C" long c_entry(long);

//--- provider.cpp
#include "polymorphic-record-abi.h"

Poly::Poly(long input) : value(input) {}
Poly::Poly(const Poly &other) : value(other.value) {}
Poly::~Poly() = default;
long Poly::read() const { return value; }
Poly Impl::apply(Poly input) const { return input; }
VirtualRecord::VirtualRecord(long input) : Poly(input), extra(input + 1) {}
VirtualRecord::VirtualRecord(const VirtualRecord &other)
    : Poly(other), extra(other.extra) {}
VirtualRecord::~VirtualRecord() = default;

Poly free_roundtrip(Poly input) { return input; }
extern "C" Poly c_roundtrip(Poly input) { return input; }
VirtualRecord virtual_roundtrip(VirtualRecord input) { return input; }
Poly call_right(Right *object, Poly input) { return object->apply(input); }

//--- consumer.cpp
#include "polymorphic-record-abi.h"

extern "C" long c_entry(long input) {
  Poly first(input);
  Poly second = free_roundtrip(first);
  Poly third = c_roundtrip(second);
  Impl implementation;
  Poly fourth = call_right(&implementation, third);
  VirtualRecord virtual_input(input);
  VirtualRecord virtual_result = virtual_roundtrip(virtual_input);
  return fourth.read() + virtual_result.read();
}

//--- runtime-stubs.cpp
using size_t = decltype(sizeof(0));
void operator delete(void *, size_t) noexcept {}
extern "C" void __cxa_pure_virtual() {}

// IR-LABEL: define dso_local void @_ZNK4Impl5applyE4Poly(
// IR-SAME: sret(%struct.Poly) align 8
// IR-SAME: %agg.result,
// IR-SAME: %this,
// IR-SAME: nofreeobj
// IR-SAME: dereferenceable(16) %input)

// IR-LABEL: define dso_local void @_ZThn8_NK4Impl5applyE4Poly(
// IR-SAME: sret(%struct.Poly) align 8
// IR-SAME: %agg.result,
// IR-SAME: %this,
// IR-SAME: nofreeobj
// IR-SAME: dereferenceable(16) %input)
// IR-O0: getelementptr inbounds i8, ptr %{{.*}}, i64 -8
// IR-O0: call void @_ZNK4Impl5applyE4Poly(
// IR-O0-SAME: sret(%struct.Poly) align 8 %agg.result,
// IR-O0-SAME: ptr {{.*}}, ptr nofreeobj {{.*}}%input)

// IR-LABEL: define dso_local void @_Z14free_roundtrip4Poly(
// IR-SAME: sret(%struct.Poly) align 8
// IR-SAME: %agg.result,
// IR-SAME: nofreeobj
// IR-SAME: dereferenceable(16) %input)
// IR-LABEL: define dso_local void @c_roundtrip(
// IR-SAME: sret(%struct.Poly) align 8
// IR-SAME: %agg.result,
// IR-SAME: nofreeobj
// IR-SAME: dereferenceable(16) %input)
// IR-LABEL: define dso_local void @_Z17virtual_roundtrip13VirtualRecord(
// IR-SAME: sret(%struct.VirtualRecord) align 8
// IR-SAME: %agg.result,
// IR-SAME: nofreeobj
// IR-SAME: dereferenceable(32) %input)

// IR-LABEL: define dso_local void @_Z10call_rightP5Right4Poly(
// IR-SAME: sret(%struct.Poly) align 8
// IR-SAME: %agg.result,
// IR-SAME: %object,
// IR-SAME: nofreeobj
// IR-SAME: dereferenceable(16) %input)
// IR: call void %{{[^ (]+}}(
// IR-SAME: sret(%struct.Poly) align 8 %agg.result,
// IR-SAME: ptr {{.*}}, ptr nofreeobj {{.*}})

// CALL: call void @_Z14free_roundtrip4Poly(
// CALL-SAME: sret(%struct.Poly) align 8 %second,
// CALL-SAME: nofreeobj {{.*}}dereferenceable(16) %agg.tmp)
// CALL: call void @c_roundtrip(
// CALL-SAME: sret(%struct.Poly) align 8 %third,
// CALL-SAME: nofreeobj {{.*}}dereferenceable(16) %agg.tmp1)
// CALL: call void @_Z10call_rightP5Right4Poly(
// CALL-SAME: sret(%struct.Poly) align 8 %fourth,
// CALL-SAME: ptr {{.*}}, ptr nofreeobj {{.*}}dereferenceable(16) %agg.tmp2)
// CALL: call void @_Z17virtual_roundtrip13VirtualRecord(
// CALL-SAME: sret(%struct.VirtualRecord) align 8 %virtual_result,
// CALL-SAME: nofreeobj {{.*}}dereferenceable(32) %agg.tmp3)

// OBJECT: R_MMIX_64 _ZThn8_NK4Impl5applyE4Poly 0x0
// OBJECT-DAG: Name: _ZThn8_NK4Impl5applyE4Poly
// OBJECT-DAG: Name: c_roundtrip
// OBJECT-DAG: Name: _Z17virtual_roundtrip13VirtualRecord
