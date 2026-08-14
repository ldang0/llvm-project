# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/input.s -o %t/input.o
# RUN: ld.lld -m elf64mmix -e _start %t/input.o -o %t/emulation
# RUN: llvm-readobj --file-headers %t/emulation \
# RUN:   | FileCheck %s --check-prefix=HEADER
# RUN: ld.lld -T %t/output-format.lds -e _start %t/input.o -o %t/format
# RUN: llvm-readobj --file-headers %t/format \
# RUN:   | FileCheck %s --check-prefix=HEADER

# HEADER:      Format: elf64-mmix
# HEADER-NEXT: Arch: mmix
# HEADER-NEXT: AddressSize: 64bit
# HEADER:      DataEncoding: BigEndian
# HEADER:      Type: Executable (0x2)
# HEADER-NEXT: Machine: EM_MMIX (0x50)

#--- input.s
.global _start
_start:
  SWYM 0, 0, 0

#--- output-format.lds
OUTPUT_FORMAT(elf64-mmix)
