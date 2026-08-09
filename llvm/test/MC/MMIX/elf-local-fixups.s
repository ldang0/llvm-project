# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --relocations %t \
# RUN:   | FileCheck %s --check-prefix=OBJ --implicit-check-not='Name: .rel' \
# RUN:       --implicit-check-not='Name: .rela'
# RUN: llvm-objdump --triple=mmix --no-print-imm-hex -d %t \
# RUN:   | FileCheck %s --check-prefix=DIS

start:
BN r1, forward_target
PUSHJ r2, forward_target
GETA r3, forward_target
JMP forward_target

forward_target:
BNB r4, start
PUSHJB r5, start
GETAB r6, start
JMPB start

# OBJ:      Name: .text
# OBJ-NEXT: Type: SHT_PROGBITS (0x1)
# OBJ-NEXT: Flags [ (0x6)
# OBJ-NEXT:   SHF_ALLOC (0x2)
# OBJ-NEXT:   SHF_EXECINSTR (0x4)
# OBJ-NEXT: ]
# OBJ-NEXT: Address: 0x0
# OBJ-NEXT: Offset: 0x40
# OBJ-NEXT: Size: 32
# OBJ-NEXT: Link: 0
# OBJ-NEXT: Info: 0
# OBJ-NEXT: AddressAlignment: 4
# OBJ-NEXT: EntrySize: 0
# OBJ-NEXT: SectionData (
# OBJ-NEXT:   0000: 40010004 F2020003 F4030002 F0000001
# OBJ-NEXT:   0010: 4104FFFC F305FFFB F506FFFA F1FFFFF9
# OBJ-NEXT: )

# OBJ:      Relocations [
# OBJ-NEXT: ]

# DIS:      BN r1, 4
# DIS-NEXT: PUSHJ r2, 3
# DIS-NEXT: GETA r3, 2
# DIS-NEXT: JMP 1
# DIS:      BNB r4, -4
# DIS-NEXT: PUSHJB r5, -5
# DIS-NEXT: GETAB r6, -6
# DIS-NEXT: JMPB -7
