// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -fdump-record-layouts-simple -emit-llvm \
// RUN:   -o /dev/null %s | FileCheck %s --check-prefix=LAYOUT
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=IR,IR-O0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=IR,IR-O2
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t.o %s
// RUN: llvm-readobj --sections --symbols --relocations %t.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e _Z9read_rootP7Diamond -o %t %t.o
// RUN: llvm-nm --undefined-only %t | count 0

struct Root {
  long root;
  virtual long value() const;
};

struct Left : virtual Root {
  long left;
  virtual long left_value() const;
};

struct Right : virtual Root {
  long right;
  virtual long right_value() const;
};

struct Diamond : Left, Right {
  long own;
  long value() const override;
};

struct DirectRoot : Root {};

struct Repeated : Left, DirectRoot {
  long repeated;
};

long Root::value() const { return root; }
long Left::left_value() const { return root + left; }
long Right::right_value() const { return root + right; }
long Diamond::value() const { return root + left + right + own; }

long read_root(Diamond *object) { return object->root; }
long call_root(Diamond *object) {
  return static_cast<Root *>(object)->value();
}
Root *as_root(Diamond *object) { return object; }

long read_repeated_virtual(Repeated *object) {
  return static_cast<Left *>(object)->root;
}

long read_repeated_direct(Repeated *object) {
  return static_cast<DirectRoot *>(object)->root;
}

// LAYOUT: Type: struct Diamond
// LAYOUT: Size:448
// LAYOUT: DataSize:448
// LAYOUT: Alignment:64
// LAYOUT: BaseOffsets: [0, 16]>
// LAYOUT: VBaseOffsets: [40]>
// LAYOUT: FieldOffsets: [256]>
// LAYOUT: Type: struct Repeated
// LAYOUT: Size:448
// LAYOUT: DataSize:448
// LAYOUT: Alignment:64
// LAYOUT: BaseOffsets: [0, 16]>
// LAYOUT: VBaseOffsets: [40]>
// LAYOUT: FieldOffsets: [256]>

// IR: @_ZTV7Diamond ={{.*}} constant { [5 x ptr], [4 x ptr], [4 x ptr] }
// IR-SAME: ptr inttoptr (i64 40 to ptr)
// IR-SAME: ptr inttoptr (i64 24 to ptr)
// IR-SAME: ptr @_ZTv0_n24_NK7Diamond5valueEv

// IR-LABEL: define dso_local noundef i64 @_Z9read_rootP7Diamond(
// IR: load ptr, ptr %{{.*}}, align 8
// IR: getelementptr i8, ptr %{{.*}}, i64 -24
// IR: load i64, ptr %{{.*}}, align 8
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 %{{.*}}
// IR: load i64, ptr %{{.*}}, align 8
// IR: ret i64 %{{.*}}

// IR-LABEL: define dso_local noundef i64 @_Z9call_rootP7Diamond(
// IR: load ptr, ptr %{{.*}}, align 8
// IR: getelementptr i8, ptr %{{.*}}, i64 -24
// IR: load i64, ptr %{{.*}}, align 8
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 %{{.*}}
// IR: load ptr, ptr %{{.*}}, align 8
// IR: load ptr, ptr %{{.*}}, align 8
// IR: call noundef i64 %{{.*}}(ptr {{.*}})

// IR-LABEL: define dso_local noundef ptr @_Z7as_rootP7Diamond(
// IR: icmp eq ptr %{{.*}}, null
// IR: load ptr, ptr %{{.*}}, align 8
// IR: getelementptr i8, ptr %{{.*}}, i64 -24
// IR: load i64, ptr %{{.*}}, align 8
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 %{{.*}}
// IR: phi ptr

// IR-LABEL: define dso_local noundef i64 @_Z21read_repeated_virtualP8Repeated(
// IR: getelementptr i8, ptr %{{.*}}, i64 -24
// IR: load i64, ptr %{{.*}}, align 8
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 %{{.*}}

// IR-LABEL: define dso_local noundef i64 @_Z20read_repeated_directP8Repeated(
// IR-O0: getelementptr inbounds i8, ptr %{{.*}}, i64 16
// IR-O0: getelementptr inbounds nuw %struct.Root, ptr %{{.*}}, i32 0, i32 1
// IR-O2: getelementptr inbounds nuw i8, ptr %{{.*}}, i64 24
// IR-NOT: getelementptr i8, ptr %{{.*}}, i64 -24
// IR: ret i64 %{{.*}}

// OBJECT: Name: .rodata
// OBJECT: AddressAlignment: 8
// OBJECT: R_MMIX_64 _ZNK7Diamond5valueEv 0x0
// OBJECT: R_MMIX_64 _ZTv0_n24_NK7Diamond5valueEv 0x0
// OBJECT: R_MMIX_64 _ZTV7Diamond 0x18
// OBJECT-DAG: Name: _ZTV7Diamond
// OBJECT-DAG: Name: _ZTv0_n24_NK7Diamond5valueEv
