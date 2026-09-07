// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=SUPPORTED
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_RTTI %s 2>&1 | FileCheck %s --check-prefix=RTTI
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -fexceptions -fcxx-exceptions \
// RUN:   -emit-llvm -o /dev/null \
// RUN:   -DTEST_EXCEPTIONS %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=EXCEPTIONS
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_DEALLOCATION %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DEALLOCATION
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++20 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_COROUTINE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=COROUTINE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_TLS %s 2>&1 | FileCheck %s --check-prefix=TLS

#if defined(TEST_RTTI)
struct Base {
  virtual ~Base();
};

void *runtime_type(Base *object) { return dynamic_cast<void *>(object); }
// RTTI: error: MMIX C++ producer profile does not support RTTI
#elif defined(TEST_EXCEPTIONS)
void raise_error() { throw 1; }
// EXCEPTIONS: error: MMIX C++ producer profile does not support exceptions
#elif defined(TEST_DEALLOCATION)
void release(long *Object) { delete Object; }
// DEALLOCATION: error: MMIX C++ producer profile does not support general deallocation
#elif defined(TEST_COROUTINE)
namespace std {
template <typename Ret, typename... Args> struct coroutine_traits {
  using promise_type = typename Ret::promise_type;
};

template <typename Promise = void> struct coroutine_handle {
  static coroutine_handle from_address(void *) noexcept;
  operator coroutine_handle<>() const noexcept;
};
} // namespace std

struct SuspendNever {
  bool await_ready() noexcept;
  void await_suspend(std::coroutine_handle<>) noexcept;
  void await_resume() noexcept;
};

struct Task {
  struct promise_type {
    Task get_return_object();
    SuspendNever initial_suspend() noexcept;
    SuspendNever final_suspend() noexcept;
    void return_void();
    void unhandled_exception();
  };
};

Task coroutine() { co_return; }
// COROUTINE: error: MMIX C++ producer profile does not support coroutines
#elif defined(TEST_TLS)
thread_local long Value;
long read_tls() { return Value; }
// TLS: error: thread-local storage is not supported for the current target
#else
void empty() {}

long increment(long value) { return value + 1; }

extern "C" int add(int lhs, int rhs) { return lhs + rhs; }

// SUPPORTED-LABEL: define dso_local void @_Z5emptyv()
// SUPPORTED-LABEL: define dso_local noundef i64 @_Z9incrementl(i64 noundef %value)
// SUPPORTED-LABEL: define dso_local i32 @add(i32 noundef signext %lhs, i32 noundef signext %rhs)
#endif
