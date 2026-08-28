// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s

namespace producer {

enum class Operation : unsigned { Add = 1, Scale = 2 };

constexpr unsigned operator+(Operation LHS, Operation RHS) {
  return static_cast<unsigned>(LHS) + static_cast<unsigned>(RHS);
}

constexpr unsigned adjust(unsigned Value) { return Value + 1; }
constexpr long adjust(long Value) { return Value + 2; }

template <typename T> constexpr T twice(T Value) { return Value + Value; }

template <> constexpr unsigned twice<unsigned>(unsigned Value) {
  return Value << 1;
}

template <typename... Ts> constexpr unsigned sum(Ts... Values) {
  return (0U + ... + Values);
}

template <bool Add> constexpr unsigned combine(unsigned LHS, unsigned RHS) {
  if constexpr (Add)
    return LHS + RHS;
  return LHS * RHS;
}

inline constexpr unsigned Bias = 3;

struct Pair {
  unsigned First;
  unsigned Second;
};

union Word {
  unsigned Value;
  unsigned char Bytes[4];
};

} // namespace producer

extern "C" unsigned cxx_language(unsigned LHS, unsigned RHS) {
  producer::Pair Values{producer::adjust(LHS),
                        producer::twice<unsigned>(RHS)};
  producer::Word Word{Values.First};
  unsigned Array[2] = {Word.Value, Values.Second};
  constexpr unsigned EnumSum =
      producer::Operation::Add + producer::Operation::Scale;
  return producer::combine<true>(Array[0], Array[1]) +
         producer::sum(producer::Bias, EnumSum);
}

// CHECK-LABEL: define dso_local {{(noundef )?}}i32 @cxx_language(
// CHECK-SAME: i32 noundef zeroext %LHS,
// CHECK-SAME: i32 noundef zeroext %RHS)
