# REQUIRES: mmix

# RUN: yaml2obj --docnum=1 %s -o %t.o
# RUN: not ld.lld -shared %t.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=SHARED
# RUN: not ld.lld -pie %t.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=PIE
# RUN: not ld.lld --dynamic-linker=/lib/ld.so %t.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INTERP
# RUN: not ld.lld --oformat=binary %t.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BINARY
# RUN: yaml2obj --docnum=2 %s -o %t-osabi.o
# RUN: not ld.lld %t-osabi.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OSABI-ERR
# RUN: yaml2obj --docnum=3 %s -o %t-shared.so
# RUN: not ld.lld %t.o %t-shared.so -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=SHARED-INPUT
# RUN: yaml2obj --docnum=4 %s -o %t-abi-version.o
# RUN: not ld.lld %t-abi-version.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ABI-VERSION

# SHARED: error: MMIX does not support shared object output
# PIE: error: MMIX does not support PIE output
# INTERP: error: MMIX does not support a dynamic linker
# BINARY: error: MMIX lld supports only ELF output
# OSABI-ERR: error: MMIX supports only the System V ELF OSABI
# SHARED-INPUT: error: MMIX does not support dynamic shared object inputs
# ABI-VERSION: error: {{.*}}: unsupported MMIX ELF ABI version 1

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .text
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000
Symbols:
  - Name:    _start
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  OSABI:   ELFOSABI_FREEBSD
  Type:    ET_REL
  Machine: EM_MMIX

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_DYN
  Machine: EM_MMIX

--- !ELF
FileHeader:
  Class:      ELFCLASS64
  Data:       ELFDATA2MSB
  ABIVersion: 1
  Type:       ET_REL
  Machine:    EM_MMIX
