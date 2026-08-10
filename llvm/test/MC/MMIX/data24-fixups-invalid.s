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

# RUN: not llvm-mc -triple=mmix -filetype=asm -output-asm-variant=1 \
# RUN:   %t/mmixal.s -o %t/mmixal.s.out 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MMIXAL
# RUN: not test -e %t/mmixal.s.out

# HIGH-BYTE: high-byte.s:1:10: error: expected preserved high byte in range [0, 255]
# HIGH-BYTE: high-byte.s:2:13: error: expected preserved high byte in range [0, 255]

# RANGE-ABS: range-absolute.s:1:13: error: MMIX 24-bit data fixup is out of range
# RAW: raw-relocation.s:2:11: error: unknown relocation name

# MMIXAL: mmixal.s:1:1: error: MMIX 24-in-32 directives require canonical assembly

#--- high-byte.s
.mmix_24 256, value
.mmix_pc_24 -1, value

#--- range-absolute.s
.mmix_24 0, 0x1000000

#--- raw-relocation.s
.data
.reloc ., R_MMIX_24, value
.long 0

#--- mmixal.s
.mmix_24 0, value
