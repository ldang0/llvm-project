# REQUIRES: mmix

# RUN: yaml2obj %s -o %t.o
# RUN: ld.lld -e _start %t.o -o %t
# RUN: llvm-readobj --file-headers --sections --program-headers %t \
# RUN:   | FileCheck %s

# CHECK:      Format: elf64-mmix
# CHECK-NEXT: Arch: mmix
# CHECK-NEXT: AddressSize: 64bit
# CHECK:      ElfHeader {
# CHECK:        Class: 64-bit (0x2)
# CHECK:        DataEncoding: BigEndian (0x2)
# CHECK:        FileVersion: 1
# CHECK:        OS/ABI: SystemV (0x0)
# CHECK:        ABIVersion: 0
# CHECK:        Type: Executable (0x2)
# CHECK:        Machine: EM_MMIX (0x50)
# CHECK:        Version: 1
# CHECK:        Flags [ (0x0)
# CHECK:      }
# CHECK-NOT:  Name: .dynamic
# CHECK-NOT:  Name: .dynstr
# CHECK-NOT:  Name: .dynsym
# CHECK-NOT:  Type: PT_DYNAMIC
# CHECK-NOT:  Type: PT_INTERP

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
