# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld --gc-sections -e _start -Map=%t/out.map %t/input.o -o %t/out
# RUN: FileCheck %s --check-prefix=MAP < %t/out.map
# RUN: ld.lld --gc-sections --print-gc-sections -e _start %t/input.o \
# RUN:   -o /dev/null | FileCheck %s --check-prefix=DISCARDED

# MAP: {{[0-9a-f]+}} {{[0-9a-f]+}} 1c 4 .text
## The 0x1c output size includes the 8-byte input plus its 20-byte call stub.
# MAP: input.o:(.text.start)
# MAP: _start
# MAP: .data
# MAP: input.o:(.data)
# MAP: data_object
# MAP: .MMIX.reg_contents
# MAP: <internal>:(.MMIX.reg_contents)
# DISCARDED: removing unused section {{.*}}input.o:(.text.discarded)

#--- input.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.start
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F200000023AA0000
  - Name: .text.discarded
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F2000000
  - Name: .rela.text.start
    Type: SHT_RELA
    Link: .symtab
    Info: .text.start
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: far_target }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: data_object }
  - Name: .rela.text.discarded
    Type: SHT_RELA
    Link: .symtab
    Info: .text.discarded
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: discarded_far }
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: _start, Section: .text.start, Binding: STB_GLOBAL }
  - { Name: discarded, Section: .text.discarded, Binding: STB_GLOBAL }
  - { Name: data_object, Section: .data, Binding: STB_GLOBAL }
  - { Name: far_target, Index: SHN_ABS, Value: 4294967296, Binding: STB_GLOBAL }
  - { Name: discarded_far, Index: SHN_ABS, Value: 8589934592, Binding: STB_GLOBAL }
