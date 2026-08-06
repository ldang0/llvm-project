; RUN: not --crash llc -mtriple=mmix -relocation-model=pic %s \
; RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=RELOC
; RUN: not --crash llc -mtriple=mmix -code-model=large %s \
; RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=CODE

; RELOC: LLVM ERROR: MMIX supports only the static relocation model
; CODE: LLVM ERROR: MMIX supports only the small code model

define void @target_options() {
  ret void
}
