// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -target-cpu generic \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o - %s \
// RUN:   | FileCheck %s

#if !defined(__mmix__) || !defined(__MMIX__) || !defined(__MMIX_ABI_GNU__)
#error MMIX GNU ABI target macros are required
#endif

enum MMIXEnum { MMIXZero, MMIXOne };

struct MMIXRecord {
  char first;
  long second;
};

_Bool mmix_bool;
char mmix_char;
short mmix_short;
int mmix_int;
long mmix_long;
long long mmix_long_long;
float mmix_float;
double mmix_double;
long double mmix_long_double;
void *mmix_pointer;
void (*mmix_function_pointer)(void);
enum MMIXEnum mmix_enum;
__builtin_va_list mmix_va_list;
struct MMIXRecord mmix_record;

_Static_assert(sizeof(struct MMIXRecord) == 16, "record size");
_Static_assert(__builtin_offsetof(struct MMIXRecord, second) == 8,
               "record layout");
_Static_assert(__builtin_types_compatible_p(__builtin_va_list, void *),
               "va_list representation");

void mmix_target_info_probe(void) {
  long output;
  __asm__ volatile("" : "=r"(output) : "I"(17) : "$0", "sp", "rJ");
}

// CHECK: target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
// CHECK: target triple = "mmix-unknown-unknown"

// CHECK-DAG: @mmix_bool ={{.*}} global i8 0, align 4
// CHECK-DAG: @mmix_char ={{.*}} global i8 0, align 4
// CHECK-DAG: @mmix_short ={{.*}} global i16 0, align 4
// CHECK-DAG: @mmix_int ={{.*}} global i32 0, align 4
// CHECK-DAG: @mmix_long ={{.*}} global i64 0, align 8
// CHECK-DAG: @mmix_long_long ={{.*}} global i64 0, align 8
// CHECK-DAG: @mmix_float ={{.*}} global float 0.000000e+00, align 4
// CHECK-DAG: @mmix_double ={{.*}} global double 0.000000e+00, align 8
// CHECK-DAG: @mmix_long_double ={{.*}} global double 0.000000e+00, align 8
// CHECK-DAG: @mmix_pointer ={{.*}} global ptr null, align 8
// CHECK-DAG: @mmix_function_pointer ={{.*}} global ptr null, align 8
// CHECK-DAG: @mmix_enum ={{.*}} global i32 0, align 4
// CHECK-DAG: @mmix_va_list ={{.*}} global ptr null, align 8
// CHECK-DAG: @mmix_record ={{.*}} global %struct.MMIXRecord zeroinitializer, align 8

// CHECK-LABEL: define{{.*}} void @mmix_target_info_probe()
// CHECK: call i64 asm sideeffect "", "=r,I,~{r0},~{r254},~{rJ}"(i32 17)
// CHECK: attributes #[[ATTR:[0-9]+]] = {{.*}}"target-cpu"="generic"
// CHECK-SAME: "target-features"="+base,+cache,+system,+virtual-memory"
