// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -fexperimental-new-constant-interpreter -emit-llvm -o - %s | FileCheck %s

static_assert(__builtin_eh_return_data_regno(0) == 7);
static_assert(__builtin_eh_return_data_regno(1) == 8);
static_assert(__builtin_eh_return_data_regno(2) == -1);

extern "C" void _Unwind_SetGR(void *, int, unsigned long);

// CHECK-LABEL: define{{.*}} @_Z12install_dataPvmi
// CHECK: call void @_Unwind_SetGR(ptr {{.*}}, i32 noundef signext 7,
// CHECK: call void @_Unwind_SetGR(ptr {{.*}}, i32 noundef signext 8,
void install_data(void *context, unsigned long exception, int selector) {
  _Unwind_SetGR(context, __builtin_eh_return_data_regno(0), exception);
  _Unwind_SetGR(context, __builtin_eh_return_data_regno(1), selector);
}
