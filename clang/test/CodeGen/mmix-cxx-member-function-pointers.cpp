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

struct Owner {
  long value;
  long method(long);
};

struct Other {
  long method(long);
};

using Member = long (Owner::*)(long);
using OtherMember = long (Other::*)(long);
using Transfer = Member (*)(Member);

static_assert(sizeof(Member) == 16);
static_assert(alignof(Member) == 8);

Member null_member = nullptr;
Member direct_member = &Owner::method;

__attribute__((noinline)) Member pass(Member member) { return member; }

Member load_member(const Member *slot) { return *slot; }

void store_member(Member *slot, Member member) { *slot = member; }

bool equal(Member lhs, Member rhs) { return lhs == rhs; }

OtherMember reinterpret_member(Member member) {
  return reinterpret_cast<OtherMember>(member);
}

__attribute__((noinline)) long call(Owner *object, Member member, long value) {
  return (object->*member)(value);
}

Member direct_boundary(Member member) { return pass(member); }

Member indirect_boundary(Transfer transfer, Member member) {
  return transfer(member);
}

// IR: @null_member ={{.*}} global { i64, i64 } zeroinitializer, align 8
// IR: @direct_member ={{.*}} global { i64, i64 } { i64 ptrtoint (ptr @_ZN5Owner6methodEl to i64), i64 0 }, align 8

// IR-LABEL: define {{.*}}void @_Z4passM5OwnerFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}%agg.result, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// IR-O0: load { i64, i64 }, ptr
// IR-O2: load i64, ptr

// IR-LABEL: define {{.*}}void @_Z11load_memberPKM5OwnerFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}%agg.result, ptr {{.*}}%slot)
// IR-O0: load { i64, i64 }, ptr %{{.*}}, align 8
// IR-O2: load i64, ptr %slot, align 8

// IR-LABEL: define {{.*}}void @_Z12store_memberPM5OwnerFllES1_(ptr {{.*}}%slot, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// IR-O0: store { i64, i64 } %{{.*}}, ptr %{{.*}}, align 8
// IR-O2: store i64 %{{.*}}, ptr %slot, align 8

// IR-LABEL: define {{.*}}i1 @_Z5equalM5OwnerFllES1_(
// IR-O0: extractvalue { i64, i64 } %{{.*}}, 0
// IR-O0: extractvalue { i64, i64 } %{{.*}}, 1
// IR-O2: load i64, ptr
// IR: icmp eq i64

// IR-LABEL: define {{.*}}void @_Z18reinterpret_memberM5OwnerFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}%agg.result, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})

// IR-LABEL: define {{.*}}i64 @_Z4callP5OwnerMS_FllEl(ptr {{.*}}%object, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}}, i64 {{.*}}%value)
// IR-O0: extractvalue { i64, i64 } %{{.*}}, 1
// IR-O2: load i64, ptr
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 %{{.*}}
// IR-O0: extractvalue { i64, i64 } %{{.*}}, 0
// IR-O2: and i64 %{{.*}}, 1
// IR: inttoptr i64 %{{.*}} to ptr
// IR: call {{.*}}i64 %{{.*}}(ptr {{.*}}, i64

// IR-LABEL: define {{.*}}void @_Z15direct_boundaryM5OwnerFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}%agg.result, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// IR: call void @_Z4passM5OwnerFllE(

// IR-LABEL: define {{.*}}void @_Z17indirect_boundaryPFM5OwnerFllES1_ES1_(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}%agg.result, ptr {{.*}}%transfer, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// IR: call void %{{.*}}(ptr {{.*}}sret({ i64, i64 }) align 8

// OBJECT: R_MMIX_64 _ZN5Owner6methodEl
// OBJECT-DAG: Name: _Z4passM5OwnerFllE
// OBJECT-DAG: Name: _Z4callP5OwnerMS_FllEl
// OBJECT-DAG: Name: _Z15direct_boundaryM5OwnerFllE
// OBJECT-DAG: Name: _Z17indirect_boundaryPFM5OwnerFllES1_ES1_
// OBJECT-DAG: Name: null_member
// OBJECT-DAG: Name: direct_member

// OBJECT-OPT: R_MMIX_64 _ZN5Owner6methodEl
// OBJECT-OPT-DAG: Name: _Z4passM5OwnerFllE
// OBJECT-OPT-DAG: Name: _Z4callP5OwnerMS_FllEl
// OBJECT-OPT-DAG: Name: _Z15direct_boundaryM5OwnerFllE
// OBJECT-OPT-DAG: Name: _Z17indirect_boundaryPFM5OwnerFllES1_ES1_
// OBJECT-OPT-DAG: Name: null_member
// OBJECT-OPT-DAG: Name: direct_member
