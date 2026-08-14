# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld -e _start -T %t/layout.lds %t/input.o -o %t/a
# RUN: ld.lld -e _start -T %t/layout.lds %t/input.o -o %t/b
# RUN: cmp %t/a %t/b
# RUN: llvm-readobj --sections %t/a | FileCheck %s

# CHECK:      Name: .note.orphan
# CHECK:      Name: .text
# CHECK:      Name: .data.orphan
# CHECK:      Name: .MMIX.reg_contents
# CHECK:      Name: .comment.orphan

#--- layout.lds
SECTIONS {
  .text 0x1000 : { *(.text.start) }
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
}

#--- input.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.start
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA0000
  - Name: .rela.text.start
    Type: SHT_RELA
    Link: .symtab
    Info: .text.start
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: orphan_data }
  - Name: .data.orphan
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content: 0102030405060708
  - Name: .note.orphan
    Type: SHT_NOTE
    Flags: [ SHF_ALLOC ]
    AddressAlign: 4
    Content: 000000000000000000000000
  - Name: .comment.orphan
    Type: SHT_PROGBITS
    AddressAlign: 1
    Content: 4D4D495800
Symbols:
  - { Name: _start, Section: .text.start, Binding: STB_GLOBAL }
  - { Name: orphan_data, Section: .data.orphan, Binding: STB_GLOBAL }
