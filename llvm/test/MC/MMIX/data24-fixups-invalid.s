# RUN: split-file %s %t

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/high-byte.s \
# RUN:   -o %t/high-byte.o 2>&1 | FileCheck %s --check-prefix=HIGH-BYTE
# RUN: not test -e %t/high-byte.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/range-absolute.s \
# RUN:   -o %t/range-absolute.o 2>&1 | FileCheck %s --check-prefix=RANGE-ABS
# RUN: not test -e %t/range-absolute.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/raw-relocation.s \
# RUN:   -o %t/raw-relocation.o 2>&1 | FileCheck %s --check-prefix=RAW
# RUN: not test -e %t/raw-relocation.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/symbol-differences.s \
# RUN:   -o %t/symbol-differences.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=DIFFERENCES
# RUN: not test -e %t/symbol-differences.o

# RUN: not llvm-mc -triple=mmix -filetype=asm -output-asm-variant=1 \
# RUN:   %t/mmixal.s -o %t/mmixal.s.out 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MMIXAL
# RUN: not test -e %t/mmixal.s.out

# HIGH-BYTE: high-byte.s:1:10: error: expected preserved high byte in range [0, 255]
# HIGH-BYTE: high-byte.s:2:13: error: expected preserved high byte in range [0, 255]

# RANGE-ABS: range-absolute.s:1:13: error: MMIX 24-bit data fixup is out of range
# RAW: raw-relocation.s:3:13: error: unknown relocation name
# RAW: raw-relocation.s:4:13: error: unknown relocation name

# DIFFERENCES: symbol-differences.s:3:13: error: MMIX 24-in-32 data relocations do not support symbol differences
# DIFFERENCES: symbol-differences.s:4:16: error: MMIX 24-in-32 data relocations do not support symbol differences

# MMIXAL: mmixal.s:1:1: error: MMIX 24-in-32 directives require canonical assembly

#--- high-byte.s
.mmix_24 256, value
.mmix_pc_24 -1, value

#--- range-absolute.s
.mmix_24 0, 0x1000000

#--- raw-relocation.s
.data
.byte 0
.reloc .+1, R_MMIX_24, value
.reloc .+2, R_MMIX_PC_24, value

#--- symbol-differences.s
.data
base:
.mmix_24 0, external - base
.mmix_pc_24 0, external - base

#--- mmixal.s
.mmix_24 0, value
