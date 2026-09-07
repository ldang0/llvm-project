// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=IR,IR-O0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=IR,IR-O2
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t.o %s
// RUN: llvm-readobj --symbols --relocations %t.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t-opt.o %s
// RUN: llvm-readobj --symbols --relocations %t-opt.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT-OPT

struct Left {
  long value;
};

struct Base {
  long value;
};

struct Derived : Left, Base {
  long own;
};

using BaseMember = long Base::*;
using DerivedMember = long Derived::*;

static_assert(sizeof(BaseMember) == 8);
static_assert(alignof(BaseMember) == 8);

struct Holder {
  BaseMember member;
};

BaseMember null_member = nullptr;
BaseMember base_member = &Base::value;
DerivedMember adjusted_member = &Base::value;
DerivedMember direct_member = &Derived::own;

__attribute__((noinline)) BaseMember pass(BaseMember member) { return member; }

__attribute__((noinline)) Holder pass_holder(Holder holder) { return holder; }

BaseMember load_member(const BaseMember *slot) { return *slot; }

void store_member(BaseMember *slot, BaseMember member) { *slot = member; }

__attribute__((noinline)) DerivedMember to_derived(BaseMember member) {
  return member;
}

__attribute__((noinline)) BaseMember to_base(DerivedMember member) {
  return static_cast<BaseMember>(member);
}

__attribute__((noinline)) long read(Derived *object, BaseMember member) {
  return object->*member;
}

__attribute__((noinline)) bool equal(BaseMember lhs, BaseMember rhs) {
  return lhs == rhs;
}

long call_boundary(Derived *object) {
  return read(object, pass(&Base::value));
}

// IR: @null_member ={{.*}} global i64 -1, align 8
// IR: @base_member ={{.*}} global i64 0, align 8
// IR: @adjusted_member ={{.*}} global i64 8, align 8
// IR: @direct_member ={{.*}} global i64 16, align 8

// IR-LABEL: define dso_local i64 @_Z4passM4Basel(i64 {{.*}}%member)
// IR: ret i64

// IR-LABEL: define dso_local i64 @_Z11pass_holder6Holder(i64 {{.*}}%holder.coerce)
// IR: ret i64

// IR-LABEL: define {{.*}}i64 @_Z11load_memberPKM4Basel(ptr {{.*}}%slot)
// IR: load i64, ptr %{{.*}}, align 8

// IR-LABEL: define {{.*}}void @_Z12store_memberPM4BaselS0_(ptr {{.*}}%slot, i64 {{.*}}%member)
// IR: store i64 %{{.*}}, ptr %{{.*}}, align 8

// IR-LABEL: define {{.*}}i64 @_Z10to_derivedM4Basel(i64 {{.*}}%member)
// IR-O0: add nsw i64 %{{.*}}, 8
// IR-O0: icmp eq i64 %{{.*}}, -1
// IR-O0: select i1 %{{.*}}, i64 %{{.*}}, i64 %{{.*}}
// IR-O2: add nsw i64 %member, 8
// IR-O2: icmp eq i64 %member, -1
// IR-O2: select i1 %{{.*}}, i64 -1, i64 %{{.*}}

// IR-LABEL: define {{.*}}i64 @_Z7to_baseM7Derivedl(i64 {{.*}}%member)
// IR-O0: sub nsw i64 %{{.*}}, 8
// IR-O0: icmp eq i64 %{{.*}}, -1
// IR-O2: add nsw i64 %member, -8
// IR-O2: icmp eq i64 %member, -1

// IR-LABEL: define {{.*}}i64 @_Z4readP7DerivedM4Basel(
// IR: getelementptr inbounds{{.*}} i8, ptr %{{.*}}, i64 8
// IR: getelementptr inbounds{{.*}} i8, ptr %{{.*}}, i64 %{{.*}}
// IR: load i64, ptr %{{.*}}, align 8

// IR-LABEL: define {{.*}}i1 @_Z5equalM4BaselS0_(i64 {{.*}}%lhs, i64 {{.*}}%rhs)
// IR: icmp eq i64 %{{.*}}, %{{.*}}

// OBJECT: R_MMIX_PUSHJ_STUBBABLE _Z4passM4Basel
// OBJECT: R_MMIX_PUSHJ_STUBBABLE _Z4readP7DerivedM4Basel
// OBJECT-DAG: Name: _Z4passM4Basel
// OBJECT-DAG: Name: _Z11pass_holder6Holder
// OBJECT-DAG: Name: _Z10to_derivedM4Basel
// OBJECT-DAG: Name: _Z7to_baseM7Derivedl
// OBJECT-DAG: Name: _Z4readP7DerivedM4Basel
// OBJECT-DAG: Name: _Z5equalM4BaselS0_
// OBJECT-DAG: Name: null_member
// OBJECT-DAG: Name: adjusted_member

// OBJECT-OPT: R_MMIX_ADDR27 _Z4readP7DerivedM4Basel
// OBJECT-OPT-DAG: Name: _Z4passM4Basel
// OBJECT-OPT-DAG: Name: _Z11pass_holder6Holder
// OBJECT-OPT-DAG: Name: _Z11load_memberPKM4Basel
// OBJECT-OPT-DAG: Name: _Z12store_memberPM4BaselS0_
// OBJECT-OPT-DAG: Name: _Z10to_derivedM4Basel
// OBJECT-OPT-DAG: Name: _Z7to_baseM7Derivedl
// OBJECT-OPT-DAG: Name: _Z4readP7DerivedM4Basel
// OBJECT-OPT-DAG: Name: _Z5equalM4BaselS0_
// OBJECT-OPT-DAG: Name: null_member
// OBJECT-OPT-DAG: Name: adjusted_member
