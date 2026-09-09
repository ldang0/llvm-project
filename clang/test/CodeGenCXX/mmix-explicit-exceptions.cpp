// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -std=c++17 -mrelocation-model static -fexceptions -fcxx-exceptions -exception-model=dwarf -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix -std=c++17 -mrelocation-model static -fexceptions -fcxx-exceptions -exception-model=dwarf -emit-obj -o %t.o %s
// RUN: not %clang_cc1 -triple mmix -std=c++17 -mrelocation-model static -emit-llvm -o /dev/null %s 2>&1 | FileCheck %s --check-prefix=DISABLED

struct Error {
  Error(const Error &);
  ~Error();
};
void may_throw();

void raise(Error &error) { throw error; }
// CHECK-LABEL: define {{.*}} @_Z5raiseR5Error(
// CHECK-SAME: personality ptr @__gxx_personality_v0
// CHECK: call ptr @__cxa_allocate_exception
// CHECK: invoke void @_ZN5ErrorC1ERKS_
// CHECK: call void @__cxa_throw
// CHECK: landingpad
// CHECK: call void @__cxa_free_exception
// CHECK: resume

int handle() {
  try { may_throw(); }
  catch (Error &) { return 1; }
  catch (...) { throw; }
  return 0;
}
// CHECK-LABEL: define {{.*}} @_Z6handlev(
// CHECK: invoke void @_Z9may_throwv
// CHECK: landingpad
// CHECK: catch ptr @_ZTI5Error
// CHECK: catch ptr null
// CHECK: call ptr @__cxa_begin_catch
// CHECK: call void @__cxa_end_catch
// CHECK: invoke void @__cxa_rethrow

void cleanup(Error &error) { Error copy(error); may_throw(); }
// CHECK-LABEL: define {{.*}} @_Z7cleanupR5Error(
// CHECK: invoke void @_Z9may_throwv
// CHECK: call void @_ZN5ErrorD1Ev
// CHECK: landingpad
// CHECK: cleanup
// CHECK: call void @_ZN5ErrorD1Ev
// CHECK: resume

void cannot_throw() noexcept { may_throw(); }
// CHECK-LABEL: define {{.*}} @_Z12cannot_throwv(
// CHECK: invoke void @_Z9may_throwv
// CHECK: landingpad
// CHECK: catch ptr null
// CHECK: call void @__clang_call_terminate
// DISABLED: error: cannot use 'throw' with exceptions disabled
