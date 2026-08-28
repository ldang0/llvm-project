// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer.o %t/consumer.cpp
// RUN: llvm-nm %t/provider.o | FileCheck %s --check-prefix=PROVIDER-SYMBOLS
// RUN: llvm-nm %t/consumer.o | FileCheck %s --check-prefix=CONSUMER-SYMBOLS
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked %t/provider.o %t/consumer.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: llvm-nm --defined-only %t/linked \
// RUN:   | FileCheck %s --check-prefix=LINKED-SYMBOLS
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider-opt.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer-opt.o %t/consumer.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt \
// RUN:   %t/provider-opt.o %t/consumer-opt.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0
// RUN: not ld.lld --no-demangle -m elf64mmix -e c_entry \
// RUN:   -o /dev/null %t/consumer.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=UNDEFINED
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/duplicate.o %t/duplicate.cpp
// RUN: not ld.lld --no-demangle -m elf64mmix -e _ZN7library3runEl \
// RUN:   -o /dev/null %t/provider.o %t/duplicate.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DUPLICATE

//--- linkage.h
namespace library {

struct Counter {
  long Value;

  explicit Counter(long Value);
  long add(long Delta) const;
};

template <typename T> T twice(T Value);
extern template long twice<long>(long Value);

long run(long Value);

} // namespace library

extern "C" long c_entry(long Value);

//--- provider.cpp
#include "linkage.h"

namespace library {
namespace {

long local_adjust(long Value) { return Value + 1; }

} // namespace

Counter::Counter(long Value) : Value(Value) {}

long Counter::add(long Delta) const { return Value + Delta; }

template <typename T> T twice(T Value) { return Value + Value; }
template long twice<long>(long Value);

long run(long Value) { return local_adjust(Value); }

} // namespace library

//--- consumer.cpp
#include "linkage.h"

extern "C" long c_entry(long Value) {
  library::Counter Counter(Value);
  return Counter.add(2) + library::twice<long>(Value) + library::run(Value);
}

//--- duplicate.cpp
namespace library {

long run(long Value) { return Value - 1; }

} // namespace library

// PROVIDER-SYMBOLS: t _ZN7library12_GLOBAL__N_112local_adjustEl
// PROVIDER-SYMBOLS: T _ZN7library3runEl
// PROVIDER-SYMBOLS: W _ZN7library5twiceIlEET_S1_
// PROVIDER-SYMBOLS: T _ZN7library7CounterC1El
// PROVIDER-SYMBOLS: T _ZN7library7CounterC2El
// PROVIDER-SYMBOLS: T _ZNK7library7Counter3addEl

// CONSUMER-SYMBOLS: U _ZN7library3runEl
// CONSUMER-SYMBOLS: U _ZN7library5twiceIlEET_S1_
// CONSUMER-SYMBOLS: U _ZN7library7CounterC1El
// CONSUMER-SYMBOLS: U _ZNK7library7Counter3addEl
// CONSUMER-SYMBOLS: T c_entry

// LINKED-SYMBOLS: T _ZN7library3runEl
// LINKED-SYMBOLS: W _ZN7library5twiceIlEET_S1_
// LINKED-SYMBOLS: T _ZN7library7CounterC1El
// LINKED-SYMBOLS: T _ZNK7library7Counter3addEl
// LINKED-SYMBOLS: T c_entry

// UNDEFINED-DAG: undefined symbol: _ZN7library3runEl
// UNDEFINED-DAG: undefined symbol: _ZN7library5twiceIlEET_S1_
// UNDEFINED-DAG: undefined symbol: _ZN7library7CounterC1El
// UNDEFINED-DAG: undefined symbol: _ZNK7library7Counter3addEl

// DUPLICATE: duplicate symbol: _ZN7library3runEl
