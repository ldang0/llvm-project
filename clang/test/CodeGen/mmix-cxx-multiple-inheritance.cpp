// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/provider.ll %t/provider.cpp
// RUN: FileCheck %s --check-prefix=PROVIDER < %t/provider.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/consumer.ll %t/consumer.cpp
// RUN: FileCheck %s --check-prefix=CONSUMER < %t/consumer.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer.o %t/consumer.cpp
// RUN: llvm-readobj --symbols --relocations %t/provider.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked \
// RUN:   %t/provider.o %t/consumer.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider-opt.o \
// RUN:   %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer-opt.o \
// RUN:   %t/consumer.cpp
// RUN: llvm-readobj --symbols --relocations %t/provider-opt.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT-OPT
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt \
// RUN:   %t/provider-opt.o %t/consumer-opt.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- multiple-inheritance.h
struct Empty {
  Empty();
  ~Empty();
};

struct Left {
  long value;
  explicit Left(long);
  ~Left();
  long read() const;
};

struct LeftPath : Left {
  explicit LeftPath(long);
  ~LeftPath();
};

struct OtherLeftPath : Left {
  explicit OtherLeftPath(long);
  ~OtherLeftPath();
};

struct Right {
  long value;
  explicit Right(long);
  ~Right();
  long read() const;
};

struct Composite : Empty, LeftPath, OtherLeftPath, Right {
  long own;
  Composite(long, long, long, long);
  ~Composite();
};

static_assert(sizeof(Empty) == 1);
static_assert(sizeof(LeftPath) == 8);
static_assert(sizeof(OtherLeftPath) == 8);
static_assert(sizeof(Right) == 8);
static_assert(sizeof(Composite) == 32);
static_assert(alignof(Composite) == 8);

long inspect_left(const Left *);
long inspect_right(const Right *);
Composite transfer(Composite);
extern "C" long c_entry(long);

//--- provider.cpp
#include "multiple-inheritance.h"

Empty::Empty() = default;
Empty::~Empty() = default;
Left::Left(long input) : value(input) {}
Left::~Left() { value = -value; }
long Left::read() const { return value; }
LeftPath::LeftPath(long input) : Left(input) {}
LeftPath::~LeftPath() = default;
OtherLeftPath::OtherLeftPath(long input) : Left(input) {}
OtherLeftPath::~OtherLeftPath() = default;
Right::Right(long input) : value(input) {}
Right::~Right() { value = -value; }
long Right::read() const { return value; }

Composite::Composite(long first, long second, long third, long fourth)
    : Empty(), LeftPath(first), OtherLeftPath(second), Right(third),
      own(fourth) {}

Composite::~Composite() { own = -own; }

long inspect_left(const Left *object) { return object->read(); }
long inspect_right(const Right *object) { return object->read(); }

__attribute__((noinline)) Composite transfer(Composite input) {
  ++input.own;
  return input;
}

//--- consumer.cpp
#include "multiple-inheritance.h"

__attribute__((noinline)) long inspect_views(Composite *object) {
  return inspect_left(static_cast<LeftPath *>(object)) +
         inspect_left(static_cast<OtherLeftPath *>(object)) +
         inspect_right(object) + object->own;
}

extern "C" long c_entry(long value) {
  Composite object(value, value + 1, value + 2, value + 3);
  Composite result = transfer(object);
  return inspect_views(&result);
}

// PROVIDER: %struct.Composite = type { %struct.LeftPath, %struct.OtherLeftPath, %struct.Right, i64 }
// PROVIDER-LABEL: define dso_local void @_ZN9CompositeC2Ellll(ptr {{.*}}%this, i64 {{.*}}%first, i64 {{.*}}%second, i64 {{.*}}%third, i64 {{.*}}%fourth)
// PROVIDER: call void @_ZN8LeftPathC2El(ptr {{.*}}, i64 {{.*}})
// PROVIDER: getelementptr inbounds i8, ptr %{{.*}}, i64 8
// PROVIDER: call void @_ZN13OtherLeftPathC2El(ptr {{.*}}, i64 {{.*}})
// PROVIDER: getelementptr inbounds i8, ptr %{{.*}}, i64 16
// PROVIDER: call void @_ZN5RightC2El(ptr {{.*}}, i64 {{.*}})
// PROVIDER-LABEL: define dso_local void @_ZN9CompositeD2Ev(ptr {{.*}}%this)
// PROVIDER: getelementptr inbounds i8, ptr %{{.*}}, i64 16
// PROVIDER: call void @_ZN5RightD2Ev(ptr {{.*}})
// PROVIDER: getelementptr inbounds i8, ptr %{{.*}}, i64 8
// PROVIDER: call void @_ZN13OtherLeftPathD2Ev(ptr {{.*}})
// PROVIDER: call void @_ZN8LeftPathD2Ev(ptr {{.*}})
// PROVIDER-LABEL: define dso_local void @_Z8transfer9Composite(
// PROVIDER-SAME: ptr {{.*}}sret(%struct.Composite) align 8 {{.*}}%agg.result,
// PROVIDER-SAME: ptr {{.*}}noundef align 8 dereferenceable(32) %input)

// CONSUMER-LABEL: define dso_local noundef i64 @_Z13inspect_viewsP9Composite(ptr {{.*}}%object)
// CONSUMER: call noundef i64 @_Z12inspect_leftPK4Left(ptr {{.*}}%{{.*}})
// CONSUMER: getelementptr inbounds i8, ptr %{{.*}}, i64 8
// CONSUMER: call noundef i64 @_Z12inspect_leftPK4Left(ptr {{.*}}%{{.*}})
// CONSUMER: getelementptr inbounds i8, ptr %{{.*}}, i64 16
// CONSUMER: call noundef i64 @_Z13inspect_rightPK5Right(ptr {{.*}}%{{.*}})
// CONSUMER-LABEL: define dso_local i64 @c_entry(i64 {{.*}}%value)
// CONSUMER: call void @_Z8transfer9Composite(
// CONSUMER-SAME: ptr {{.*}}sret(%struct.Composite) align 8 %result,
// CONSUMER-SAME: ptr {{.*}}noundef align 8 dereferenceable(32) %agg.tmp)

// OBJECT: R_MMIX_PUSHJ_STUBBABLE _ZN8LeftPathC2El
// OBJECT: R_MMIX_PUSHJ_STUBBABLE _ZN13OtherLeftPathC2El
// OBJECT: R_MMIX_PUSHJ_STUBBABLE _ZN5RightC2El
// OBJECT-DAG: Name: _ZN9CompositeC1Ellll
// OBJECT-DAG: Name: _ZN9CompositeC2Ellll
// OBJECT-DAG: Name: _ZN9CompositeD1Ev
// OBJECT-DAG: Name: _ZN9CompositeD2Ev
// OBJECT-DAG: Name: _Z8transfer9Composite

// OBJECT-OPT: Relocations [
// OBJECT-OPT-NEXT: ]
// OBJECT-OPT-DAG: Name: _ZN9CompositeC1Ellll
// OBJECT-OPT-DAG: Name: _ZN9CompositeC2Ellll
// OBJECT-OPT-DAG: Name: _ZN9CompositeD1Ev
// OBJECT-OPT-DAG: Name: _ZN9CompositeD2Ev
// OBJECT-OPT-DAG: Name: _Z8transfer9Composite
