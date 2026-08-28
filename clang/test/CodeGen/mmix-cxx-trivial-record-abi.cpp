// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -x c++ -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o %t.cxx.ll %s
// RUN: FileCheck %s --check-prefix=ABI < %t.cxx.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj \
// RUN:   %t.cxx.ll -o /dev/null
// RUN: %clang_cc1 -x c++ -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o %t.opt.ll %s
// RUN: FileCheck %s --check-prefix=OPT < %t.opt.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj \
// RUN:   %t.opt.ll -o /dev/null
// RUN: %clang_cc1 -x c -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o %t.c.ll %s
// RUN: FileCheck %s --check-prefix=ABI < %t.c.ll

struct Empty {};
struct Small {
  int Value;
};
struct Boundary {
  long Value;
};
struct Large {
  long Values[2];
};

#ifdef __cplusplus
extern "C" {
#endif

struct Small external_small(struct Small);
struct Boundary external_boundary(struct Boundary);
struct Large external_large(struct Large);

struct Empty forward_empty(struct Empty Value) { return Value; }

struct Small forward_small(struct Small Value) {
  return external_small(Value);
}

struct Boundary forward_boundary(struct Boundary Value) {
  return external_boundary(Value);
}

struct Large forward_large(struct Large Value) {
  return external_large(Value);
}

#ifdef __cplusplus
}
#endif

// ABI-LABEL: define dso_local void @forward_empty()
// ABI: ret void
// ABI-LABEL: define dso_local i32 @forward_small(i32 noext %Value.coerce)
// ABI: call i32 @external_small(i32 noext
// ABI: ret i32
// ABI-LABEL: define dso_local i64 @forward_boundary(i64 %Value.coerce)
// ABI: call i64 @external_boundary(i64
// ABI: ret i64
// ABI-LABEL: define dso_local void @forward_large(
// ABI-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// ABI-SAME: ptr noundef byval(%struct.Large) align 8 %Value)
// ABI: call void @external_large(
// ABI-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8 %agg.result,
// ABI-SAME: ptr noundef byval(%struct.Large) align 8 %{{(agg.tmp|Value)}})

// OPT-LABEL: define dso_local void @forward_empty()
// OPT-LABEL: define dso_local i32 @forward_small(i32 noext %Value.coerce)
// OPT: tail call i32 @external_small(i32 noext %Value.coerce)
// OPT-LABEL: define dso_local i64 @forward_boundary(i64 %Value.coerce)
// OPT: tail call i64 @external_boundary(i64 %Value.coerce)
// OPT-LABEL: define dso_local void @forward_large(
// OPT-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// OPT-SAME: ptr {{.*}}byval(%struct.Large) align 8 {{.*}}%Value)
// OPT: tail call void @external_large(
// OPT-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8 %agg.result,
// OPT-SAME: ptr {{.*}}byval(%struct.Large) align 8 %Value)
