# REQUIRES: mmix

# RUN: yaml2obj --docnum=1 %s -o %t.o
# RUN: ld.lld -e _start %t.o -o %t
# RUN: llvm-readobj --file-headers %t | FileCheck %s --check-prefix=HEADER

# RUN: yaml2obj --docnum=2 %s -o %t-unsupported.o
# RUN: not ld.lld -e _start %t-unsupported.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ERROR

# HEADER:      Format: elf64-mmix
# HEADER-NEXT: Arch: mmix
# HEADER-NEXT: AddressSize: 64bit
# HEADER:      Type: Executable (0x2)
# HEADER-NEXT: Machine: EM_MMIX (0x50)

# ERROR: error: {{.*}}unsupported relocation R_MMIX_PC_64 against symbol _start

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
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_NONE, Symbol: _start, Addend: 0 }
Symbols:
  - Name:    _start
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL

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
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Size:         8
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_PC_64, Symbol: _start, Addend: 0 }
Symbols:
  - Name:    _start
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL
