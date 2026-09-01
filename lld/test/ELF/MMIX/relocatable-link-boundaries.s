# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=mmix %t/undefined.s -o %t/undefined.o
# RUN: ld.lld -r -e ignored_entry --image-base=0x1000 %t/undefined.o \
# RUN:   -o %t/ignored-layout.o
# RUN: llvm-readobj --file-headers --sections --symbols %t/ignored-layout.o \
# RUN:   | FileCheck %s --check-prefix=IGNORED
# RUN: not ld.lld -r -pie %t/undefined.o -o %t/pie.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=PIE
# RUN: not test -e %t/pie.o
# RUN: not ld.lld -r --dynamic-linker=/lib/ld.so %t/undefined.o \
# RUN:   -o %t/dynamic.o 2>&1 | FileCheck %s --check-prefix=DYNAMIC
# RUN: not test -e %t/dynamic.o
# RUN: yaml2obj %t/unknown.yaml -o %t/unknown.o
# RUN: not ld.lld -r %t/unknown.o -o %t/unknown-linked.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=UNKNOWN
# RUN: not test -e %t/unknown-linked.o
# RUN: yaml2obj %t/rel.yaml -o %t/rel.o
# RUN: not ld.lld -r %t/rel.o -o %t/rel-linked.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=REL
# RUN: not test -e %t/rel-linked.o
# RUN: yaml2obj %t/malformed-register.yaml -o %t/malformed-register.o
# RUN: not ld.lld -r %t/malformed-register.o -o %t/malformed-linked.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MALFORMED
# RUN: not test -e %t/malformed-linked.o
# RUN: yaml2obj %t/tls.yaml -o %t/tls.o
# RUN: not ld.lld -r %t/tls.o -o %t/tls-linked.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=TLS
# RUN: not test -e %t/tls-linked.o

# IGNORED:      Type: Relocatable
# IGNORED:      Entry: 0x0
# IGNORED:      ProgramHeaderCount: 0
# IGNORED:      Address: 0x0
# IGNORED:      Name: missing
# IGNORED:      Section: Undefined
# PIE:          -r and -pie may not be used together
# DYNAMIC:      MMIX does not support a dynamic linker
# UNKNOWN:      unknown relocation (250) against symbol target
# REL:          MMIX supports only RELA relocations
# MALFORMED:    MMIX register contents size is not a multiple of 8
# TLS:          relocation R_MMIX_64 against TLS symbol tls is unsupported

#--- undefined.s
.section .data,"aw",@progbits
.global ignored_entry
ignored_entry:
  .quad missing
.global missing

#--- unknown.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: 250, Symbol: target }
Symbols:
  - { Name: target, Index: SHN_ABS }

#--- rel.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rel.data
    Type: SHT_REL
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: target }
Symbols:
  - { Name: target, Index: SHN_ABS }

#--- malformed-register.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 00000000000000

#--- tls.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: tls }
Symbols:
  - { Name: tls, Section: .data, Type: STT_TLS, Binding: STB_GLOBAL }
