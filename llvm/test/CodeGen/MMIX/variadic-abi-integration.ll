; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
; RUN:   -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=asm \
; RUN:   %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
; RUN:   %s -o /dev/null

target triple = "mmix-unknown-elf"

%packed3 = type <{ i8, i16 }>
%large = type { i64, i64 }
%empty = type {}

@large_value = internal global %large { i64 17, i64 23 }, align 8

declare void @llvm.va_start(ptr)
declare void @llvm.va_copy(ptr, ptr)
declare void @llvm.va_end(ptr)
declare void @clobber()

; With no named slots, the caller starts the unnamed sequence in $231 and the
; callee starts its save area with that register at S-128.
; ISEL-LABEL: name: first_no_fixed
; ISEL:       [[FIRST:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; ISEL:       STOUI [[FIRST]], %fixed-stack.0, 0
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL:       [[VALUE:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.0, 0
define i64 @first_no_fixed(...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  %value = load i64, ptr %cursor, align 8
  call void @llvm.va_end(ptr %ap)
  ret i64 %value
}

; ISEL-LABEL: name: call_no_fixed
; ISEL:       $r231 = COPY
; ISEL:       DIRECT_CALL_STATE @first_no_fixed{{.*}}implicit $r231
define i64 @call_no_fixed(i64 %value) {
  %result = call i64 (...) @first_no_fixed(i64 %value)
  ret i64 %result
}

; Fifteen named slots put the first unnamed value in $246 and the second at
; incoming SP. The callee saves $246 at S-8 before the nested call, then walks
; from that save directly into the caller's stack area.
; ISEL-LABEL: name: boundary15
; ISEL:       [[SAVED:%[0-9]+]]:{{[^ ]+}} = COPY $r246
; ISEL:       STOUI [[SAVED]], %fixed-stack.0, 0
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL:       STOUI {{(killed )?}}[[CURSOR]], %stack.0.ap, 0
; ISEL:       DIRECT_CALL_STATE @clobber
; ISEL:       [[RELOADED:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.0.ap, 0
; ISEL-DAG:   [[FIRST:%[0-9]+]]:{{[^ ]+}} = LDOUI [[RELOADED]], 0
; ISEL-DAG:   [[SECOND:%[0-9]+]]:{{[^ ]+}} = LDOUI [[RELOADED]], 8
; ASM-LABEL: boundary15:
; ASM:       STOU r246,
; ASM:       SETH {{.*}}(clobber>>48)
; ASM:       PUSHGO
; ASM:       LDOU
define i64 @boundary15(i64, i64, i64, i64, i64, i64, i64, i64,
                       i64, i64, i64, i64, i64, i64, i64, ...) #0 {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  call void @clobber()
  %cursor = load ptr, ptr %ap, align 8
  %first = load i64, ptr %cursor, align 8
  %stack = getelementptr i8, ptr %cursor, i64 8
  %second = load i64, ptr %stack, align 8
  %next = getelementptr i8, ptr %cursor, i64 16
  store volatile ptr %next, ptr %ap, align 8
  call void @llvm.va_end(ptr %ap)
  %result = xor i64 %first, %second
  ret i64 %result
}

; ISEL-LABEL: name: call_boundary15
; ISEL:       ADJCALLSTACKDOWN 8, 0
; ISEL:       STOUI {{.*}}, {{%[0-9]+}}, 0 :: (store (s64) into stack)
; ISEL:       $r246 = COPY
; ISEL:       DIRECT_CALL_STATE @boundary15{{.*}}implicit $r246
define i64 @call_boundary15() {
  %result = call i64 (i64, i64, i64, i64, i64, i64, i64, i64,
                      i64, i64, i64, i64, i64, i64, i64, ...) @boundary15(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14,
      i64 85, i64 170)
  ret i64 %result
}

; At K=16 the first unnamed value starts at incoming SP. No register-save
; stores are needed.
; ISEL-LABEL: name: first_stack16
; ISEL:       liveins:         []
; ISEL:       body:
; ISEL-NOT:   STOUI {{.*}}, %fixed-stack.0
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL:       [[VALUE:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.0, 0
define i64 @first_stack16(i64, i64, i64, i64, i64, i64, i64, i64,
                          i64, i64, i64, i64, i64, i64, i64, i64, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  %value = load i64, ptr %cursor, align 8
  call void @llvm.va_end(ptr %ap)
  ret i64 %value
}

; At K=17 the named stack value occupies S and the first unnamed value starts
; at S+8.
; ISEL-LABEL: name: first_stack17
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.1, 0
; ISEL:       [[VALUE:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.1, 0
define i64 @first_stack17(i64, i64, i64, i64, i64, i64, i64, i64,
                          i64, i64, i64, i64, i64, i64, i64, i64,
                          i64, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  %value = load i64, ptr %cursor, align 8
  call void @llvm.va_end(ptr %ap)
  ret i64 %value
}

; The callers place the K=16 unnamed value at outgoing SP and place the K=17
; final named value and first unnamed value at outgoing SP and SP+8.
; ISEL-LABEL: name: call_stack_starts
; ISEL:       ADJCALLSTACKDOWN 8, 0
; ISEL:       STOUI {{.*}}, {{%[0-9]+}}, 0 :: (store (s64) into stack)
; ISEL:       DIRECT_CALL_STATE @first_stack16
; ISEL:       ADJCALLSTACKDOWN 16, 0
; ISEL-DAG:   STOUI {{.*}}, {{%[0-9]+}}, 0 :: (store (s64) into stack)
; ISEL-DAG:   STOUI {{.*}}, {{%[0-9]+}}, 8 :: (store (s64) into stack + 8)
; ISEL:       DIRECT_CALL_STATE @first_stack17
define i64 @call_stack_starts() {
  %at16 = call i64 (i64, i64, i64, i64, i64, i64, i64, i64,
                    i64, i64, i64, i64, i64, i64, i64, i64, ...) @first_stack16(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15,
      i64 100)
  %at17 = call i64 (i64, i64, i64, i64, i64, i64, i64, i64,
                    i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) @first_stack17(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15,
      i64 16, i64 200)
  %result = xor i64 %at16, %at17
  ret i64 %result
}

; Passing va_list by value forwards its cursor value. It does not give the
; callee access to the caller's pointer object.
; ISEL-LABEL: name: forwarded_value
; ISEL:       [[VALUE:%[0-9]+]]:{{[^ ]+}} = LDOUI {{%[0-9]+}}, 0
define i64 @forwarded_value(ptr %cursor) {
  %value = load i64, ptr %cursor, align 8
  ret i64 %value
}

; Repeated va_start operations publish the same first unnamed address into
; independent pointer objects. va_copy creates a third independent object;
; advancing the first cursor does not move either of the others.
; ISEL-LABEL: name: cursor_ownership
; ISEL:       [[START:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL-DAG:   STOUI [[START]], %stack.0.first, 0
; ISEL-DAG:   STOUI [[START]], %stack.1.second, 0
; ISEL-DAG:   STOUI [[START]], %stack.2.copy, 0
; ISEL-DAG:   [[NEXT:%[0-9]+]]:{{[^ ]+}} = ADDUI [[START]], 8
; ISEL-DAG:   STOUI {{(killed )?}}[[NEXT]], %stack.0.first, 0
; ISEL:       DIRECT_CALL_STATE @forwarded_value
; ISEL:       DIRECT_CALL_STATE @forwarded_value
define i64 @cursor_ownership(i64 %named, ...) {
  %first = alloca ptr, align 8
  %second = alloca ptr, align 8
  %copy = alloca ptr, align 8
  call void @llvm.va_start(ptr %first)
  call void @llvm.va_start(ptr %second)
  call void @llvm.va_copy(ptr %copy, ptr %first)
  %first_cursor = load ptr, ptr %first, align 8
  %next = getelementptr i8, ptr %first_cursor, i64 8
  store volatile ptr %next, ptr %first, align 8
  %second_cursor = load ptr, ptr %second, align 8
  %copy_cursor = load ptr, ptr %copy, align 8
  %second_value = call i64 @forwarded_value(ptr %second_cursor)
  %copy_value = call i64 @forwarded_value(ptr %copy_cursor)
  %first_value = load i64, ptr %next, align 8
  call void @llvm.va_end(ptr %first)
  call void @llvm.va_end(ptr %second)
  call void @llvm.va_end(ptr %copy)
  %same_start = xor i64 %second_value, %copy_value
  %result = xor i64 %same_start, %first_value
  ret i64 %result
}

; The caller and callee agree on every reviewed unnamed class. Signed and
; unsigned promoted values occupy complete octas, double uses one ordinary
; slot, the packed aggregate is right-adjusted in its slot, the empty value
; consumes no slot, and the large aggregate slot contains a caller-copy
; pointer.
; ISEL-LABEL: name: read_classes
; ISEL-DAG:   [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDTI {{(killed )?}}{{%[0-9]+}}, 0
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDTUI %fixed-stack.0, 12
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDOUI %fixed-stack.0, 16
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDBUI %fixed-stack.0, 29
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDBUI %fixed-stack.0, 30
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDBUI %fixed-stack.0, 31
; ISEL-DAG:   [[OBJECT:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.0, 32
; ISEL-DAG:   {{%[0-9]+}}:{{[^ ]+}} = LDOUI {{(killed )?}}[[OBJECT]], 8
; ISEL-DAG:   [[FINAL:%[0-9]+]]:{{[^ ]+}} = ADDUI [[CURSOR]], 40
define i64 @read_classes(i64 %named, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor0 = load ptr, ptr %ap, align 8

  %signed_address = getelementptr i8, ptr %cursor0, i64 4
  %signed = load i32, ptr %signed_address, align 4
  %cursor1 = getelementptr i8, ptr %cursor0, i64 8

  %unsigned_address = getelementptr i8, ptr %cursor1, i64 4
  %unsigned = load i32, ptr %unsigned_address, align 4
  %cursor2 = getelementptr i8, ptr %cursor1, i64 8

  %fp = load double, ptr %cursor2, align 8
  %cursor3 = getelementptr i8, ptr %cursor2, i64 8

  %packed_address = getelementptr i8, ptr %cursor3, i64 5
  %packed = load %packed3, ptr %packed_address, align 1
  %packed_byte = extractvalue %packed3 %packed, 0
  %packed_half = extractvalue %packed3 %packed, 1
  %cursor4 = getelementptr i8, ptr %cursor3, i64 8

  ; The empty unnamed aggregate requires no read and no cursor update.
  %object = load ptr, ptr %cursor4, align 8
  %large_field_address = getelementptr %large, ptr %object, i32 0, i32 1
  %large_field = load i64, ptr %large_field_address, align 8
  %cursor5 = getelementptr i8, ptr %cursor4, i64 8
  store volatile ptr %cursor5, ptr %ap, align 8
  call void @llvm.va_end(ptr %ap)

  %signed64 = sext i32 %signed to i64
  %unsigned64 = zext i32 %unsigned to i64
  %fp64 = bitcast double %fp to i64
  %byte64 = zext i8 %packed_byte to i64
  %half64 = zext i16 %packed_half to i64
  %a = xor i64 %signed64, %unsigned64
  %b = xor i64 %fp64, %byte64
  %c = xor i64 %half64, %large_field
  %d = xor i64 %a, %b
  %result = xor i64 %d, %c
  ret i64 %result
}

; ISEL-LABEL: name: call_classes
; ISEL:       [[UNSIGNED:%[0-9]+]]:{{[^ ]+}} = AND
; ISEL:       [[SHIFTED:%[0-9]+]]:{{[^ ]+}} = SLUI {{%[0-9]+}}, 32
; ISEL-NEXT:  [[SIGNED:%[0-9]+]]:{{[^ ]+}} = SRI {{(killed )?}}[[SHIFTED]], 32
; ISEL:       $r232 = COPY [[SIGNED]]
; ISEL:       $r233 = COPY [[UNSIGNED]]
; ISEL:       $r234 = COPY
; ISEL:       $r235 = COPY
; ISEL:       $r236 = COPY
; ISEL-NOT:   $r237 = COPY
; ISEL:       DIRECT_CALL_STATE @read_classes{{.*}}implicit $r231,
; ISEL-SAME:  implicit $r232, implicit $r233, implicit $r234, implicit $r235,
; ISEL-SAME:  implicit $r236
; ASM-LABEL: call_classes:
; ASM:       SETH {{.*}}(read_classes>>48)
; ASM:       PUSHGO
define i64 @call_classes(i32 %signed, i32 %unsigned, double %fp,
                         %packed3 %packed) {
  %result = call i64 (i64, ...) @read_classes(
      i64 0, i32 signext %signed, i32 zeroext %unsigned, double %fp,
      %packed3 %packed, %empty zeroinitializer,
      ptr byval(%large) align 8 @large_value)
  ret i64 %result
}

attributes #0 = { "frame-pointer"="all" }
