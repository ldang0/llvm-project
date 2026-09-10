// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -o - %s | FileCheck %s

struct Empty {};
struct Nontrivial { ~Nontrivial(); };

// CHECK-LABEL: define{{.*}} i64 @_Z4read5Emptyz(...)
// CHECK: call void @llvm.va_start
// CHECK: getelementptr inbounds i8, ptr {{.*}}, i64 8
long read(Empty last, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, last);
  long result = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return result;
}

// CHECK-LABEL: define{{.*}} i64 @_Z4readl5EmptyS_z(i64{{.*}}, ...)
long read(long fixed, Empty, Empty last, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, last);
  long result = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return fixed + result;
}

// Nontrivial empty objects retain their invisible reference, not Ignore.
// CHECK-LABEL: define{{.*}} i64 @_Z4read10Nontrivialz(ptr{{.*}}, ...)
long read(Nontrivial last, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, last);
  long result = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return result;
}

// CHECK-LABEL: define{{.*}} i64 @_Z6callerPFll5EmptyS_zE
// CHECK: call{{.*}} i64 (...) @_Z4read5Emptyz(i64{{.*}} 41)
// CHECK: call{{.*}} i64 (i64, ...) {{.*}}(i64{{.*}} 1, i64{{.*}} 42)
long caller(long (*fp)(long, Empty, Empty, ...)) {
  return read(Empty{}, 41L) + fp(1, Empty{}, Empty{}, 42L);
}
