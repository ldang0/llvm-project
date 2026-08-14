# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/use.yaml -o %t/use.o
# RUN: yaml2obj %t/contents.yaml -o %t/contents.o
# RUN: ld.lld -e _start %t/use.o %t/contents.o -o %t/default
# RUN: llvm-readobj --sections --symbols %t/default \
# RUN:   | FileCheck %s --check-prefix=STRUCTURE \
# RUN:     --implicit-check-not=.MMIX.reg_contents.linker_allocated
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/default \
# RUN:   | FileCheck %s --check-prefix=CONTENTS
# RUN: ld.lld -T %t/valid.ld %t/use.o %t/contents.o -o %t/scripted
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/scripted \
# RUN:   | FileCheck %s --check-prefix=CONTENTS
# RUN: yaml2obj %t/register.yaml -o %t/register.o
# RUN: not ld.lld -e 0 %t/register.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=REGISTER
# RUN: yaml2obj %t/exhausted.yaml -o %t/exhausted.o
# RUN: not ld.lld -e 0 %t/exhausted.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=EXHAUSTED
# RUN: not ld.lld -T %t/invalid.ld %t/use.o %t/contents.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=LAYOUT

# STRUCTURE:      Name: .MMIX.reg_contents
# STRUCTURE:      Type: SHT_PROGBITS
# STRUCTURE:      Address: 0x7E0
# STRUCTURE:      Size: 24
# STRUCTURE:      AddressAlignment: 8
# STRUCTURE:      Name: input_register
# STRUCTURE-NEXT: Value: 0xFE
# STRUCTURE:      Section: Processor Specific (0xFF00)

# CONTENTS:      Contents of section .text:
# CONTENTS-NEXT: {{[0-9a-f]+}} 23aafc00 23bbfcff 23ccfd00 23ddfd01
# CONTENTS:      Contents of section .MMIX.reg_contents:
# CONTENTS-NEXT: 07e0 00000000 00001000 00000000 00001100
# CONTENTS-NEXT: 07f0 11223344 55667788

# REGISTER: relocation R_MMIX_BASE_PLUS_OFFSET cannot use register symbol reg as an address
# EXHAUSTED: too many MMIX global register contents: 224, maximum is 223
# LAYOUT: MMIX linker-allocated register contents must precede ordinary .MMIX.reg_contents input sections

#--- valid.ld
SECTIONS {
  .text : { *(.text) }
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
}

#--- invalid.ld
SECTIONS {
  .text : { *(.text) }
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents)
    *(.MMIX.reg_contents.linker_allocated)
  }
}

#--- use.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA000023BB000023CC000023DD0000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: first, Addend: 0 }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: first, Addend: 255 }
      - { Offset: 10, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: second, Addend: 0 }
      - { Offset: 14, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: second, Addend: 1 }
Symbols:
  - { Name: _start, Section: .text, Binding: STB_GLOBAL, Type: STT_FUNC }
  - { Name: first, Index: SHN_ABS, Value: 4096, Binding: STB_GLOBAL }
  - { Name: second, Index: SHN_ABS, Value: 4352, Binding: STB_GLOBAL }

#--- contents.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: input_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- register.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA0000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: reg }
Symbols:
  - { Name: reg, Index: 0xFF00, Value: 32 }

#--- exhausted.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA000023BB0000
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Size: 1776
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: low }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: high }
Symbols:
  - { Name: low, Index: SHN_ABS, Value: 0 }
  - { Name: high, Index: SHN_ABS, Value: 256 }
