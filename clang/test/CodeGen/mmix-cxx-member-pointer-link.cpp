// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %t/provider.cpp \
// RUN:   | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %t/consumer.cpp \
// RUN:   | FileCheck %s --check-prefix=CONSUMER-IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer.o %t/consumer.cpp
// RUN: llvm-readobj --symbols --relocations %t/provider.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked \
// RUN:   %t/provider.o %t/consumer.o
// RUN: llvm-readobj --symbols --relocations %t/linked \
// RUN:   | FileCheck %s --check-prefix=LINKED
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider-opt.o \
// RUN:   %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer-opt.o \
// RUN:   %t/consumer.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt \
// RUN:   %t/provider-opt.o %t/consumer-opt.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- member-pointers.h
struct Left {
  long left;
  long from_left(long);
};

struct Right {
  long right;
  long from_right(long);
};

struct Derived : Left, Right {
  long own;
};

struct Single : Left {
  long own;
};

using LeftData = long Left::*;
using RightData = long Right::*;
using DerivedData = long Derived::*;
using LeftFunction = long (Left::*)(long);
using RightFunction = long (Right::*)(long);
using DerivedFunction = long (Derived::*)(long);
using SingleData = long Single::*;
using SingleFunction = long (Single::*)(long);

struct MemberBundle {
  DerivedData data;
  DerivedFunction function;
};

static_assert(sizeof(DerivedData) == 8);
static_assert(sizeof(DerivedFunction) == 16);
static_assert(sizeof(MemberBundle) == 24);
static_assert(alignof(MemberBundle) == 8);

extern DerivedData null_data;
extern DerivedData converted_null_data;
extern SingleData single_data;
extern DerivedData right_data;
extern DerivedFunction null_function;
extern DerivedFunction converted_null_function;
extern SingleFunction single_function;
extern DerivedFunction right_function;
extern MemberBundle right_bundle;

extern "C" long invoke(Derived *, DerivedData, DerivedFunction, long);
extern "C" long c_entry(Derived *);

//--- provider.cpp
#include "member-pointers.h"

long Left::from_left(long value) { return left + value; }
long Right::from_right(long value) { return right + value; }

DerivedData null_data = nullptr;
DerivedData converted_null_data = static_cast<RightData>(nullptr);
SingleData single_data = &Left::left;
DerivedData right_data = &Right::right;
DerivedFunction null_function = nullptr;
DerivedFunction converted_null_function = static_cast<RightFunction>(nullptr);
SingleFunction single_function = &Left::from_left;
DerivedFunction right_function = &Right::from_right;
MemberBundle right_bundle = {&Right::right, &Right::from_right};

extern "C" long invoke(Derived *object, DerivedData data,
                       DerivedFunction function, long value) {
  return object->*data + (object->*function)(value);
}

//--- consumer.cpp
#include "member-pointers.h"

DerivedData convert_data(RightData data) { return data; }

RightData restore_data(DerivedData data) {
  return static_cast<RightData>(data);
}

DerivedFunction convert_function(RightFunction function) { return function; }

RightFunction restore_function(DerivedFunction function) {
  return static_cast<RightFunction>(function);
}

extern "C" long c_entry(Derived *object) {
  DerivedFunction converted = convert_function(&Right::from_right);
  RightFunction restored = restore_function(converted);
  if (null_data != nullptr || converted_null_data != nullptr ||
      null_function != nullptr || converted_null_function != nullptr ||
      restored != &Right::from_right)
    return -1;
  return invoke(object, right_bundle.data, right_bundle.function, 3) +
         invoke(object, right_data, right_function, 4);
}

// IR: @null_data ={{.*}} global i64 -1, align 8
// IR: @converted_null_data ={{.*}} global i64 -1, align 8
// IR: @single_data ={{.*}} global i64 0, align 8
// IR: @right_data ={{.*}} global i64 8, align 8
// IR: @null_function ={{.*}} global { i64, i64 } zeroinitializer, align 8
// IR: @converted_null_function ={{.*}} global { i64, i64 } { i64 0, i64 8 }, align 8
// IR: @single_function ={{.*}} global { i64, i64 } { i64 ptrtoint (ptr @_ZN4Left9from_leftEl to i64), i64 0 }, align 8
// IR: @right_function ={{.*}} global { i64, i64 } { i64 ptrtoint (ptr @_ZN5Right10from_rightEl to i64), i64 8 }, align 8
// IR: @right_bundle ={{.*}} global %struct.MemberBundle { i64 8, { i64, i64 } { i64 ptrtoint (ptr @_ZN5Right10from_rightEl to i64), i64 8 } }, align 8
// IR-LABEL: define dso_local i64 @invoke(ptr {{.*}}%object, i64 {{.*}}%data, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}}, i64 {{.*}}%value)

// CONSUMER-IR-LABEL: define dso_local i64 @_Z12convert_dataM5Rightl(i64 {{.*}}%data)
// CONSUMER-IR: add nsw i64 %{{.*}}, 8
// CONSUMER-IR: icmp eq i64 %{{.*}}, -1
// CONSUMER-IR: select i1 %{{.*}}, i64 %{{.*}}, i64 %{{.*}}
// CONSUMER-IR-LABEL: define dso_local i64 @_Z12restore_dataM7Derivedl(i64 {{.*}}%data)
// CONSUMER-IR: sub nsw i64 %{{.*}}, 8
// CONSUMER-IR: icmp eq i64 %{{.*}}, -1
// CONSUMER-IR-LABEL: define dso_local void @_Z16convert_functionM5RightFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// CONSUMER-IR: add nsw i64 %{{.*}}, 8
// CONSUMER-IR-LABEL: define dso_local void @_Z16restore_functionM7DerivedFllE(ptr {{.*}}sret({ i64, i64 }) align 8 {{.*}}, ptr {{.*}}byval({ i64, i64 }) align 8 {{.*}})
// CONSUMER-IR: sub nsw i64 %{{.*}}, 8

// OBJECT: R_MMIX_64 _ZN4Left9from_leftEl 0x0
// OBJECT-COUNT-2: R_MMIX_64 _ZN5Right10from_rightEl 0x0
// OBJECT-DAG: Name: _ZN4Left9from_leftEl
// OBJECT-DAG: Name: _ZN5Right10from_rightEl
// OBJECT-DAG: Name: invoke
// OBJECT-DAG: Name: null_data
// OBJECT-DAG: Name: converted_null_data
// OBJECT-DAG: Name: single_data
// OBJECT-DAG: Name: right_data
// OBJECT-DAG: Name: null_function
// OBJECT-DAG: Name: converted_null_function
// OBJECT-DAG: Name: single_function
// OBJECT-DAG: Name: right_function
// OBJECT-DAG: Name: right_bundle

// LINKED: Relocations [
// LINKED-NEXT: ]
// LINKED-DAG: Name: _ZN4Left9from_leftEl
// LINKED-DAG: Name: _ZN5Right10from_rightEl
// LINKED-DAG: Name: invoke
// LINKED-DAG: Name: c_entry
