// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -O0 -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=COMMON,O0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -O2 -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=COMMON,O2
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -O0 -mrelocation-model static -emit-obj -o %t.o %s
// RUN: llvm-nm --undefined-only %t.o | FileCheck %s --check-prefix=UNDEFINED

using size_t = decltype(sizeof(0));
void *operator new(size_t, void *) noexcept;

extern "C" void observe(long);

struct Base {
  long Value;

  explicit Base(long Value) : Value(Value) {}
  ~Base() { observe(Value); }
};

struct Derived : Base {
  long Extra;

  Derived(long Value, long Extra) : Base(Value), Extra(Extra) {}
  Derived(const Derived &Other) : Base(Other.Value), Extra(Other.Extra) {}
  Derived(Derived &&Other) : Base(Other.Value), Extra(Other.Extra + 1) {}
  ~Derived() { observe(Extra); }

  long sum() const { return Value + Extra; }
};

struct Add {
  long Bias;

  long operator()(long Value) const { return Value + Bias; }
};

extern "C" long object_lifetime(void *Storage, long Value) {
  Derived First(Value, 2);
  Derived Copy(First);
  Derived Moved(static_cast<Derived &&>(Copy));
  Add FunctionObject{3};
  auto Capturing = [Value](long Operand) { return Value + Operand; };
  auto Noncapturing = [](long Operand) { return Operand + 4; };
  Derived *Placed = new (Storage) Derived(Moved);
  long Result = FunctionObject(First.sum()) + Capturing(Placed->sum()) +
                Noncapturing(Value);
  Placed->~Derived();
  return Result;
}

// COMMON-LABEL: define dso_local {{.*}}i64 @object_lifetime(
// COMMON-SAME: ptr {{[^,]*}}noundef{{[^%]*}}%Storage,
// COMMON-SAME: i64 noundef %Value)
// COMMON-NOT: @_Znwm
// COMMON-NOT: @_ZdlPv
// COMMON-NOT: @__gxx_personality_v0
// COMMON-NOT: @__cxa_
// COMMON-NOT: @_ZTV
// COMMON-NOT: @_ZTI
// O0: call void @_ZN7DerivedC1Ell
// O0: call void @_ZN7DerivedC1ERKS_
// O0: call void @_ZN7DerivedC1EOS_
// O0: call noundef i64 @_ZNK3AddclEl
// O0: call noundef i64 @"_ZZ15object_lifetimeENK3$_0clEl"
// O0: call noundef i64 @"_ZZ15object_lifetimeENK3$_1clEl"
// O0: call void @_ZN7DerivedD1Ev
// O2: ret i64

// UNDEFINED-NOT: _Znwm
// UNDEFINED-NOT: _ZdlPv
// UNDEFINED-NOT: __gxx_personality_v0
// UNDEFINED-NOT: __cxa_
// UNDEFINED-NOT: _ZTV
// UNDEFINED-NOT: _ZTI
// UNDEFINED: observe
