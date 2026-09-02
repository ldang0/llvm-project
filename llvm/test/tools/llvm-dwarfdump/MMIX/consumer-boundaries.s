# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t.o
# RUN: llvm-dwarfdump --verify %t.o
# RUN: llvm-dwarfdump --debug-info %t.o \
# RUN:   | FileCheck %s --check-prefix=UNKNOWN-REGISTER
# RUN: yaml2obj %S/Inputs/consumer-invalid.yaml -o %t-truncated.o
# RUN: not llvm-dwarfdump --verify %t-truncated.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=TRUNCATED
# RUN: yaml2obj --docnum=2 %S/Inputs/consumer-invalid.yaml -o %t-version.o
# RUN: not llvm-dwarfdump --verify %t-version.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=VERSION

# UNKNOWN-REGISTER: DW_AT_location (DW_OP_regx 0x3e7)
# UNKNOWN-REGISTER-NEXT: DW_AT_name ("unknown_register")
# TRUNCATED: warning: unexpected end of data
# VERSION: warning: {{.*}}unsupported version 1, supported are 2-6

.section .debug_abbrev,"",@progbits
  .uleb128 1
  .uleb128 0x11                       # DW_TAG_compile_unit
  .byte 1                             # DW_CHILDREN_yes
  .byte 0
  .byte 0
  .uleb128 2
  .uleb128 0x34                       # DW_TAG_variable
  .byte 0                             # DW_CHILDREN_no
  .uleb128 0x02                       # DW_AT_location
  .uleb128 0x18                       # DW_FORM_exprloc
  .uleb128 0x03                       # DW_AT_name
  .uleb128 0x08                       # DW_FORM_string
  .byte 0
  .byte 0
  .byte 0

.section .debug_info,"",@progbits
.Lcu_begin:
  .4byte .Lcu_end-.Lcu_version
.Lcu_version:
  .2byte 5
  .byte 1                             # DW_UT_compile
  .byte 8
  .4byte 0
  .uleb128 1
  .uleb128 2
  .uleb128 3
  .byte 0x90                          # DW_OP_regx
  .uleb128 999
  .asciz "unknown_register"
  .byte 0
.Lcu_end:
