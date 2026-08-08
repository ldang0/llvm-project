; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/external-function.ll \
; RUN:   -o %t/external-function.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=EXTERNAL-FUNCTION
; RUN: test ! -s %t/external-function.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/external-data.ll \
; RUN:   -o %t/external-data.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=EXTERNAL-DATA
; RUN: test ! -s %t/external-data.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/runtime-helper.ll \
; RUN:   -o %t/runtime-helper.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=RUNTIME-HELPER
; RUN: test ! -s %t/runtime-helper.mms
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/resolved-helper.ll -o - \
; RUN:   | FileCheck %s --check-prefix=RESOLVED-HELPER
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/weak-linkage.ll \
; RUN:   -o %t/weak-linkage.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=WEAK-LINKAGE
; RUN: test ! -s %t/weak-linkage.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/comdat.ll -o %t/comdat.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=COMDAT
; RUN: test ! -s %t/comdat.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/ifunc.ll -o %t/ifunc.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=IFUNC
; RUN: test ! -s %t/ifunc.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/hidden.ll -o %t/hidden.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=HIDDEN
; RUN: test ! -s %t/hidden.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/partition.ll -o %t/partition.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=PARTITION
; RUN: test ! -s %t/partition.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/global-ctors.ll \
; RUN:   -o %t/global-ctors.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=GLOBAL-CTORS
; RUN: test ! -s %t/global-ctors.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/init-array.ll -o %t/init-array.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=INIT-ARRAY
; RUN: test ! -s %t/init-array.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/symvers.ll -o %t/symvers.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=SYMVERS
; RUN: test ! -s %t/symvers.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/nonzero-global.ll \
; RUN:   -o %t/nonzero-global.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NONZERO-GLOBAL
; RUN: test ! -s %t/nonzero-global.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/nonzero-instruction.ll \
; RUN:   -o %t/nonzero-instruction.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NONZERO-INSTRUCTION
; RUN: test ! -s %t/nonzero-instruction.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/tls-global.ll \
; RUN:   -o %t/tls-global.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=TLS-GLOBAL
; RUN: test ! -s %t/tls-global.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/thread-pointer.ll \
; RUN:   -o %t/thread-pointer.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=THREAD-POINTER
; RUN: test ! -s %t/thread-pointer.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/personality.ll \
; RUN:   -o %t/personality.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=PERSONALITY
; RUN: test ! -s %t/personality.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/uwtable.ll -o %t/uwtable.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=UWTABLE
; RUN: test ! -s %t/uwtable.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/stackmap.ll -o %t/stackmap.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=STACKMAP
; RUN: test ! -s %t/stackmap.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/sanitizer.ll \
; RUN:   -o %t/sanitizer.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=SANITIZER
; RUN: test ! -s %t/sanitizer.mms
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/debug-info.ll -o - \
; RUN:   | FileCheck %s --check-prefix=DEBUG-INFO
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   %t/canonical-linkage.ll -o - \
; RUN:   | FileCheck %s --check-prefix=CANONICAL

; EXTERNAL-FUNCTION: LLVM ERROR: MMIXAL output variant 1 cannot resolve referenced symbol 'external_function'
; EXTERNAL-FUNCTION-NOT: __LLVM_
; EXTERNAL-DATA: LLVM ERROR: MMIXAL output variant 1 cannot resolve referenced symbol 'external_data'
; EXTERNAL-DATA-NOT: __LLVM_
; RUNTIME-HELPER: LLVM ERROR: MMIXAL output variant 1 cannot resolve referenced symbol 'memcpy'
; RUNTIME-HELPER-NOT: __LLVM_
; RESOLVED-HELPER: memcpy IS @
; RESOLVED-HELPER: SETH ${{[0-9]+}}, memcpy>>48&65535
; RESOLVED-HELPER: PUSHGO $31, ${{[0-9]+}}, 0
; WEAK-LINKAGE: LLVM ERROR: MMIXAL output variant 1 does not support linkage 'weak' for symbol 'selected'
; COMDAT: LLVM ERROR: MMIXAL output variant 1 does not support COMDAT membership for symbol 'selected'
; IFUNC: LLVM ERROR: MMIXAL output variant 1 does not support GlobalIFunc 'selected'
; HIDDEN: LLVM ERROR: MMIXAL output variant 1 does not support hidden visibility for symbol 'selected'
; PARTITION: LLVM ERROR: MMIXAL output variant 1 does not support partition 'partition_name' for symbol 'selected'
; GLOBAL-CTORS: LLVM ERROR: MMIXAL output variant 1 does not support runtime registration symbol 'llvm.global_ctors'
; INIT-ARRAY: LLVM ERROR: MMIXAL output variant 1 does not support runtime registration section '.init_array.100' for symbol 'registration'
; SYMVERS: LLVM ERROR: MMIXAL output variant 1 does not support ELF symbol version for 'versioned'
; NONZERO-GLOBAL: LLVM ERROR: MMIXAL output variant 1 does not support nonzero address space in symbol 'nonzero_global'
; NONZERO-INSTRUCTION: LLVM ERROR: MMIXAL output variant 1 does not support nonzero address space in instruction 'inttoptr' in function 'owner'
; TLS-GLOBAL: LLVM ERROR: MMIXAL output variant 1 does not support thread-local symbol 'tls'
; THREAD-POINTER: LLVM ERROR: MMIXAL output variant 1 does not support TLS intrinsic 'llvm.thread.pointer' in function 'Main'
; PERSONALITY: LLVM ERROR: MMIXAL output variant 1 does not support an exception personality in function 'exceptional'
; UWTABLE: LLVM ERROR: MMIXAL output variant 1 does not support unwind-table generation in function 'Main'
; STACKMAP: LLVM ERROR: MMIXAL output variant 1 does not support runtime metadata intrinsic 'llvm.experimental.stackmap' in function 'Main'
; SANITIZER: LLVM ERROR: MMIXAL output variant 1 does not support runtime instrumentation attribute 'sanitize_address' in function 'Main'
; DEBUG-INFO: Main IS @
; DEBUG-INFO-NOT: .file
; DEBUG-INFO-NOT: .loc
; DEBUG-INFO-NOT: compiler identification
; CANONICAL: .weak selected
; CANONICAL: selected:

;--- external-function.ll
target triple = "mmix-unknown-elf"

declare void @external_function()

define void @Main() {
entry:
  call void @external_function()
  br label %loop

loop:
  br label %loop
}

;--- external-data.ll
target triple = "mmix-unknown-elf"

@external_data = external global i64

define void @Main() {
entry:
  %value = load volatile i64, ptr @external_data
  br label %loop

loop:
  br label %loop
}

;--- runtime-helper.ll
target triple = "mmix-unknown-elf"

declare ptr @memcpy(ptr, ptr, i64)

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

define void @copy(ptr %destination, ptr %source, i64 %size) {
entry:
  %result = call ptr @memcpy(ptr %destination, ptr %source, i64 %size)
  ret void
}

;--- resolved-helper.ll
target triple = "mmix-unknown-elf"

declare void @unused()
declare i64 @llvm.ctpop.i64(i64)

define void @Main() {
entry:
  %destination = inttoptr i64 256 to ptr
  %source = inttoptr i64 512 to ptr
  %result = call ptr @memcpy(ptr %destination, ptr %source, i64 8)
  br label %loop

loop:
  br label %loop
}

define ptr @memcpy(ptr %destination, ptr %source, i64 %size) {
entry:
  ret ptr %destination
}

;--- weak-linkage.ll
target triple = "mmix-unknown-elf"

define weak void @selected() {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- comdat.ll
target triple = "mmix-unknown-elf"

$group = comdat any

define void @selected() comdat($group) {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- ifunc.ll
target triple = "mmix-unknown-elf"

@selected = ifunc void (), ptr @resolver

define ptr @resolver() {
  ret ptr @implementation
}

define void @implementation() {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- hidden.ll
target triple = "mmix-unknown-elf"

@selected = hidden global i64 0

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- partition.ll
target triple = "mmix-unknown-elf"

@selected = global i64 0, partition "partition_name"

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- global-ctors.ll
target triple = "mmix-unknown-elf"

@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }]
  [{ i32, ptr, ptr } { i32 65535, ptr @constructor, ptr null }]

define void @constructor() {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- init-array.ll
target triple = "mmix-unknown-elf"

@registration = global ptr @initializer, section ".init_array.100"

define void @initializer() {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- symvers.ll
target triple = "mmix-unknown-elf"

!symvers = !{!0}
!0 = !{!"versioned", !"versioned@VERSION_1"}

define void @versioned() {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- nonzero-global.ll
target triple = "mmix-unknown-elf"

@nonzero_global = addrspace(1) global i64 0

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- nonzero-instruction.ll
target triple = "mmix-unknown-elf"

define void @owner() {
entry:
  %pointer = inttoptr i64 1 to ptr addrspace(1)
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- tls-global.ll
target triple = "mmix-unknown-elf"

@tls = thread_local(localexec) global i64 0

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- thread-pointer.ll
target triple = "mmix-unknown-elf"

declare ptr @llvm.thread.pointer()

define void @Main() {
entry:
  %pointer = call ptr @llvm.thread.pointer()
  br label %loop

loop:
  br label %loop
}

;--- personality.ll
target triple = "mmix-unknown-elf"

declare i32 @personality(...)

define void @exceptional() personality ptr @personality {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- uwtable.ll
target triple = "mmix-unknown-elf"

define void @Main() uwtable {
entry:
  br label %loop

loop:
  br label %loop
}

;--- stackmap.ll
target triple = "mmix-unknown-elf"

declare void @llvm.experimental.stackmap(i64, i32, ...)

define void @Main() {
entry:
  call void (i64, i32, ...) @llvm.experimental.stackmap(i64 1, i32 0)
  br label %loop

loop:
  br label %loop
}

;--- sanitizer.ll
target triple = "mmix-unknown-elf"

define void @Main() sanitize_address {
entry:
  br label %loop

loop:
  br label %loop
}

;--- debug-info.ll
target triple = "mmix-unknown-elf"

define void @Main() !dbg !4 {
entry:
  br label %loop, !dbg !7

loop:
  br label %loop, !dbg !8
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}
!llvm.ident = !{!3}
!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1,
    producer: "compiler", isOptimized: false, runtimeVersion: 0,
    emissionKind: FullDebug)
!1 = !DIFile(filename: "input.c", directory: "/source")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{!"compiler identification"}
!4 = distinct !DISubprogram(name: "Main", scope: !1, file: !1, line: 1,
    type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = !DILocation(line: 2, column: 1, scope: !4)
!8 = !DILocation(line: 3, column: 1, scope: !4)

;--- canonical-linkage.ll
target triple = "mmix-unknown-elf"

define weak void @selected() {
  ret void
}

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}
