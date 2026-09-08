// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-llvm -o %t.ll %s
// RUN: FileCheck %s --check-prefix=IR < %t.ll
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -emit-obj -o %t.o %s
// RUN: llvm-readobj --file-headers --relocations %t.o | FileCheck %s --check-prefix=OBJ
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -fno-rtti -DNO_RTTI -emit-llvm -o - %s | FileCheck %s --check-prefix=OFF

struct Base { virtual void f(); };
void Base::f() {}
struct Single : Base { void f() override; };
void Single::f() {}
struct Other { virtual void g(); };
void Other::g() {}
struct Multiple : Base, Other { void f() override; };
void Multiple::f() {}
struct Virtual : virtual Base { void f() override; };
void Virtual::f() {}

#ifndef NO_RTTI
namespace std { class type_info; }
// Constant references exercise descriptor emission without runtime type queries.
const std::type_info &integer_type = typeid(int);
const std::type_info &qualified_type = typeid(const int);
const std::type_info &pointer_type = typeid(const Base *);
const std::type_info &member_type = typeid(int Base::*);
#endif

// IR-DAG: @_ZTIi = external dso_local constant ptr
// IR-DAG: @integer_type = dso_local{{.*}} constant ptr @_ZTIi, align 8
// IR-DAG: @qualified_type = dso_local{{.*}} constant ptr @_ZTIi, align 8
// IR-DAG: @_ZTI4Base ={{.*}}constant { ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 2), ptr @_ZTS4Base }, align 8
// IR-DAG: @_ZTI6Single ={{.*}}constant { ptr, ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2), ptr @_ZTS6Single, ptr @_ZTI4Base }, align 8
// IR-DAG: @_ZTI8Multiple ={{.*}}constant { ptr, ptr, i32, i32, ptr, i64, ptr, i64 } {{.*}}i32 0, i32 2, ptr @_ZTI4Base, i64 2, ptr @_ZTI5Other, i64 2050 }, align 8
// IR-DAG: @_ZTI7Virtual ={{.*}}constant { ptr, ptr, i32, i32, ptr, i64 } {{.*}}i32 0, i32 1, ptr @_ZTI4Base, i64 -8189 }, align 8
// IR-DAG: @_ZTIPK4Base = linkonce_odr{{.*}}constant { ptr, ptr, i32, ptr } {{.*}}ptr @_ZTVN10__cxxabiv119__pointer_type_infoE, i64 2), ptr @_ZTSPK4Base, i32 1, ptr @_ZTI4Base }, comdat, align 8
// IR-DAG: @_ZTIM4Basei = linkonce_odr{{.*}}constant { ptr, ptr, i32, ptr, ptr } {{.*}}ptr @_ZTVN10__cxxabiv129__pointer_to_member_type_infoE, i64 2), ptr @_ZTSM4Basei, i32 0, ptr @_ZTIi, ptr @_ZTI4Base }, comdat, align 8
// IR-DAG: @_ZTV4Base ={{.*}}constant { [3 x ptr] } { [3 x ptr] [ptr null, ptr @_ZTI4Base, ptr @_ZN4Base1fEv] }, align 8
// OBJ: Format: elf64-mmix
// OBJ: DataEncoding: BigEndian
// OBJ: Machine: EM_MMIX
// OBJ-DAG: R_MMIX_64 _ZTVN10__cxxabiv117__class_type_infoE 0x10
// OBJ-DAG: R_MMIX_64 _ZTVN10__cxxabiv120__si_class_type_infoE 0x10
// OBJ-DAG: R_MMIX_64 _ZTVN10__cxxabiv121__vmi_class_type_infoE 0x10
// OBJ-DAG: R_MMIX_64 _ZTVN10__cxxabiv119__pointer_type_infoE 0x10
// OBJ-DAG: R_MMIX_64 _ZTVN10__cxxabiv129__pointer_to_member_type_infoE 0x10
// OFF-NOT: @_ZTI
// OFF: @_ZTV4Base ={{.*}}[ptr null, ptr null, ptr @_ZN4Base1fEv]
// OFF-NOT: @_ZTI
