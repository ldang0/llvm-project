# RUN: split-file %s %t

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/subtractive.s \
# RUN:   -o %t/subtractive.o 2>&1 | FileCheck %s --check-prefix=SUBTRACTIVE
# RUN: not test -e %t/subtractive.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/multiple-symbols.s \
# RUN:   -o %t/multiple-symbols.o 2>&1 | FileCheck %s --check-prefix=MULTIPLE
# RUN: not test -e %t/multiple-symbols.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/symbolic-operations.s \
# RUN:   -o %t/symbolic-operations.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OPERATIONS
# RUN: not test -e %t/symbolic-operations.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/symbolic-width.s \
# RUN:   -o %t/symbolic-width.o 2>&1 | FileCheck %s --check-prefix=WIDTH
# RUN: not test -e %t/symbolic-width.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/modifier.s \
# RUN:   -o %t/modifier.o 2>&1 | FileCheck %s --check-prefix=MODIFIER
# RUN: not test -e %t/modifier.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/constants.s \
# RUN:   -o %t/constants.o 2>&1 | FileCheck %s --check-prefix=CONSTANTS
# RUN: not test -e %t/constants.o

# SUBTRACTIVE: subtractive.s:2:16: error: symbol 'subtractive' can not be undefined in a subtraction expression
# SUBTRACTIVE: subtractive.s:6:16: error: Cannot represent a difference across sections

# MULTIPLE: multiple-symbols.s:1:13: error: expected relocatable expression
# MULTIPLE: multiple-symbols.s:2:13: error: expected relocatable expression

# OPERATIONS: symbolic-operations.s:1:15: error: expected relocatable expression
# OPERATIONS: symbolic-operations.s:2:14: error: expected relocatable expression

# WIDTH: symbolic-width.s:1:7: error: unknown token in expression

# MODIFIER: modifier.s:1:15: error: unexpected token

# CONSTANTS: constants.s:1:7: error: out of range literal value
# CONSTANTS: constants.s:2:7: error: out of range literal value
# CONSTANTS: constants.s:3:8: error: out of range literal value
# CONSTANTS: constants.s:4:8: error: out of range literal value
# CONSTANTS: constants.s:5:7: error: out of range literal value
# CONSTANTS: constants.s:6:7: error: out of range literal value
# CONSTANTS: constants.s:7:7: error: literal value out of range for directive

# Link-time GNU bitfield overflow depends on the final S + A value and remains
# a linker diagnostic. The assembly-known values below are diagnosed before
# relocation selection and must not be truncated.

#--- subtractive.s
.data
.quad additive - subtractive
.section .other,"aw",@progbits
other:
.data
.quad external - other

#--- multiple-symbols.s
.quad first + second
.quad first - second - third

#--- symbolic-operations.s
.quad shifted << 1
.quad masked & 255

#--- symbolic-width.s
.octa wide_symbol

#--- modifier.s
.quad modified@GOT

#--- constants.s
.byte 256
.byte -129
.short 65536
.short -32769
.long 4294967296
.long -2147483649
.quad 18446744073709551616
