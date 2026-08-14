# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/input.s -o %t/input.o
# RUN: ld.lld -T %t/layout.lds %t/input.o -o %t/executable
# RUN: llvm-readobj --sections --symbols %t/executable | FileCheck %s --check-prefix=STRUCTURE
# RUN: llvm-objdump --no-print-imm-hex -d --section=.text %t/executable | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -s --section=.text %t/executable | FileCheck %s --check-prefix=BYTES

# STRUCTURE:      Name: .text
# STRUCTURE:      Size: 96
# STRUCTURE:      Name: hidden_target
# STRUCTURE:      Other [
# STRUCTURE-NEXT: STV_HIDDEN
# STRUCTURE-COUNT-4: Name: __MMIX_call_stub_
# STRUCTURE:      Name: global_target
# STRUCTURE:      Binding: Global
# STRUCTURE:      Name: weak_target
# STRUCTURE:      Binding: Weak
# STRUCTURE:      Section: Undefined

# DIS-LABEL: <_start>:
# DIS-NEXT: {{.*}} PUSHJ r1, 4
# DIS-NEXT: {{.*}} PUSHJ r2, 8
# DIS-NEXT: {{.*}} PUSHJ r3, 12
# DIS-NEXT: {{.*}} PUSHJ r4, 16
# DIS-COUNT-4: GO r255, r255, 0

## The weak undefined symbol contributes the static value zero, leaving only
## the relocation addend in its full-address stub.
# BYTES:      1040 e5ff3344 e4ff1122 9fffff00 e3ff7794
# BYTES-NEXT: 1050 e6ff5566 e5ff3344 e4ff1122 9fffff00

#--- input.s
.text
.global _start
.type _start,@function
_start:
PUSHJ r1, local_target
PUSHJ r2, hidden_target
PUSHJ r3, global_target
PUSHJ r4, weak_target + 0x1122334455667794
.size _start, .-_start

.local local_target
.set local_target, 0x1122334455667788
.global hidden_target
.hidden hidden_target
.set hidden_target, 0x112233445566778c
.global global_target
.set global_target, 0x1122334455667790
.weak weak_target

#--- layout.lds
ENTRY(_start)
SECTIONS {
  .text 0x1000 : { *(.text) }
}
