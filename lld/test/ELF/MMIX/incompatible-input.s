# REQUIRES: mmix

# RUN: yaml2obj --docnum=1 %s -o %t-mmix.o
# RUN: yaml2obj --docnum=2 %s -o %t-32.o
# RUN: not ld.lld %t-32.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MMIX-KIND
# RUN: not ld.lld %t-mmix.o %t-32.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=CLASS
# RUN: yaml2obj --docnum=3 %s -o %t-le.o
# RUN: not ld.lld %t-le.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MMIX-KIND
# RUN: not ld.lld %t-mmix.o %t-le.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ENDIAN
# RUN: yaml2obj --docnum=4 %s -o %t-x86.o
# RUN: not ld.lld %t-mmix.o %t-x86.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MACHINE
# RUN: yaml2obj --docnum=5 %s -o %t-exec
# RUN: not ld.lld %t-exec -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=TYPE
# RUN: echo -e -n "\x7fELF\x00\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x50" > %t-malformed.o
# RUN: not ld.lld %t-malformed.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MALFORMED

# MMIX-KIND: error: MMIX supports only ELF64 big-endian input and output
# CLASS: error: {{.*}} is incompatible with {{.*}}
# ENDIAN: error: {{.*}} is incompatible with {{.*}}
# MACHINE: error: {{.*}} is incompatible with {{.*}}
# TYPE: error: {{.*}}: unknown file type
# MALFORMED: error: {{.*}}: corrupted ELF file: invalid file class

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

--- !ELF
FileHeader:
  Class:   ELFCLASS32
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2LSB
  Type:    ET_REL
  Machine: EM_MMIX

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_X86_64

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_EXEC
  Machine: EM_MMIX
