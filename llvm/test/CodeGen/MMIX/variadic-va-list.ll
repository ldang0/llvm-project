; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj %s -o /dev/null

target triple = "mmix"

%packed3 = type <{ i8, i16 }>
%large = type { i64, i64 }

@copied_cursor = external global ptr

declare void @llvm.va_start(ptr)
declare void @llvm.va_copy(ptr, ptr)
declare void @llvm.va_end(ptr)

; A four-byte direct value occupies the rightmost four bytes of its octa. The
; cursor itself advances by one complete octa.
; CHECK-LABEL: name: direct_i32
; CHECK:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; CHECK:       [[NEXT:%[0-9]+]]:{{[^ ]+}} = ADDUI [[CURSOR]], 8
; CHECK:       STOUI {{(killed )?}}[[NEXT]], %stack.0.ap, 0
; CHECK:       [[ADDRESS:%[0-9]+]]:{{[^ ]+}} = disjoint ORI [[CURSOR]], 4
; CHECK:       [[VALUE:%[0-9]+]]:{{[^ ]+}} = LDTUI {{(killed )?}}[[ADDRESS]], 0
define i32 @direct_i32(i64 %named, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  %address = getelementptr i8, ptr %cursor, i64 4
  %value = load i32, ptr %address, align 4
  %next = getelementptr i8, ptr %cursor, i64 8
  store volatile ptr %next, ptr %ap, align 8
  call void @llvm.va_end(ptr %ap)
  ret i32 %value
}

; The same forced right adjustment applies to a three-byte packed aggregate.
; Its fields begin at cursor+5 and cursor+6, while the next cursor is cursor+8.
; CHECK-LABEL: name: direct_packed3
; CHECK:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; CHECK:       [[NEXT:%[0-9]+]]:{{[^ ]+}} = ADDUI [[CURSOR]], 8
; CHECK:       STOUI {{(killed )?}}[[NEXT]], %stack.0.ap, 0
; CHECK-DAG:   [[ADDRESS5:%[0-9]+]]:{{[^ ]+}} = disjoint ORI [[CURSOR]], 5
; CHECK-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDBUI {{(killed )?}}[[ADDRESS5]], 0
; CHECK-DAG:   [[ADDRESS6:%[0-9]+]]:{{[^ ]+}} = disjoint ORI [[CURSOR]], 6
; CHECK-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDBUI {{(killed )?}}[[ADDRESS6]], 0
; CHECK-DAG:   [[ADDRESS7:%[0-9]+]]:{{[^ ]+}} = disjoint ORI [[CURSOR]], 7
; CHECK-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDBUI {{(killed )?}}[[ADDRESS7]], 0
define i64 @direct_packed3(i64 %named, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  %address = getelementptr i8, ptr %cursor, i64 5
  %value = load %packed3, ptr %address, align 1
  %byte = extractvalue %packed3 %value, 0
  %half = extractvalue %packed3 %value, 1
  %byte64 = zext i8 %byte to i64
  %half64 = zext i16 %half to i64
  %shifted = shl i64 %byte64, 16
  %result = or i64 %shifted, %half64
  %next = getelementptr i8, ptr %cursor, i64 8
  store volatile ptr %next, ptr %ap, align 8
  call void @llvm.va_end(ptr %ap)
  ret i64 %result
}

; An indirect aggregate slot contains a pointer to the caller-owned copy.
; CHECK-LABEL: name: indirect_large
; CHECK:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; CHECK:       [[OBJECT:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.0, 0
; CHECK:       [[VALUE:%[0-9]+]]:{{[^ ]+}} = LDOUI {{(killed )?}}[[OBJECT]], 8
; CHECK:       [[NEXT:%[0-9]+]]:{{[^ ]+}} = ADDUI [[CURSOR]], 8
; CHECK:       STOUI {{(killed )?}}[[NEXT]], %stack.0.ap, 0
define i64 @indirect_large(i64 %named, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  %object = load ptr, ptr %cursor, align 8
  %field = getelementptr %large, ptr %object, i32 0, i32 1
  %value = load i64, ptr %field, align 8
  %next = getelementptr i8, ptr %cursor, i64 8
  store volatile ptr %next, ptr %ap, align 8
  call void @llvm.va_end(ptr %ap)
  ret i64 %value
}

; va_copy copies the pointer value, not an alias to the source va_list object.
; Advancing the source object therefore leaves the destination value unchanged,
; and va_end contributes no operation after legalization.
; CHECK-LABEL: name: copy_cursor
; CHECK:       STOUI [[INITIAL:%[0-9]+]], %stack.0.source, 0
; CHECK:       [[COPIED:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.0.source, 0
; CHECK:       STOUI {{(killed )?}}[[COPIED]], %stack.1.destination, 0
; CHECK:       [[NEXT:%[0-9]+]]:{{[^ ]+}} = ADDUI [[INITIAL]], 8
; CHECK:       STOUI {{(killed )?}}[[NEXT]], %stack.0.source, 0
; CHECK:       [[RESULT:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.1.destination, 0
; CHECK-NOT:   VAEND
define void @copy_cursor(i64 %initial_bits) {
  %source = alloca ptr, align 8
  %destination = alloca ptr, align 8
  %initial = inttoptr i64 %initial_bits to ptr
  store volatile ptr %initial, ptr %source, align 8
  call void @llvm.va_copy(ptr %destination, ptr %source)
  %next = getelementptr i8, ptr %initial, i64 8
  store volatile ptr %next, ptr %source, align 8
  %result = load volatile ptr, ptr %destination, align 8
  call void @llvm.va_end(ptr %source)
  call void @llvm.va_end(ptr %destination)
  store volatile ptr %result, ptr @copied_cursor, align 8
  ret void
}
