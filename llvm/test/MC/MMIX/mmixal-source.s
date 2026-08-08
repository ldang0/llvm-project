# RUN: llvm-mc -triple=mmix -filetype=asm --output-asm-variant=1 %s \
# RUN:   -o - | FileCheck %s

.text
entry:
  ADD r1, r2, r3
.Ltemporary:
  JMPB entry

.section .rodata
prefix:
  .byte 0xaa
  .p2align 3
constant:
  .byte 1, 2
  .short 0x0304
  .long 0x05060708
  .quad entry

.data
use:
  .quad target + 8
target:
  .quad entry
alias = target

.bss
zero:
  .zero 8

# CHECK:      LOC #0000000000000100
# CHECK-NEXT: entry	IS @
# CHECK-NEXT: ADD $1, $2, $3
# CHECK-NEXT: LOC #0000000000000104
# CHECK-NEXT: __LLVM_M_TMP_0	IS @
# CHECK-NEXT: JMP entry
# CHECK-NEXT: LOC #2000000000000000
# CHECK-NEXT: prefix	IS @
# CHECK-NEXT: BYTE #AA
# CHECK-NEXT: LOC #2000000000000008
# CHECK-NEXT: constant	IS @
# CHECK-NEXT: BYTE #01
# CHECK-NEXT: BYTE #02
# CHECK-NEXT: BYTE #03, #04
# CHECK-NEXT: BYTE #05, #06, #07, #08
# CHECK-NEXT: OCTA entry
# CHECK-NEXT: LOC #2000000000000020
# CHECK-NEXT: target	IS @
# CHECK-NEXT: OCTA entry
# CHECK-NEXT: alias	IS target
# CHECK-NEXT: LOC #2000000000000018
# CHECK-NEXT: use	IS @
# CHECK-NEXT: OCTA target+8
# CHECK-NEXT: LOC #2000000000000028
# CHECK-NEXT: zero	IS @
# CHECK-NEXT: OCTA 0
# CHECK-NOT:  :
# CHECK-NOT:  {{^[[:space:]]*\.}}
