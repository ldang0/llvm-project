# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/use.yaml -o %t/use.o
# RUN: yaml2obj %t/contents.yaml -o %t/contents.o
# RUN: ld.lld -r %t/use.o %t/contents.o -o %t/partial.o
# RUN: llvm-readobj --sections --symbols --relocations --expand-relocs \
# RUN:   %t/partial.o | FileCheck %s --check-prefix=PARTIAL \
# RUN:   --implicit-check-not=.MMIX.reg_contents.linker_allocated
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents \
# RUN:   %t/partial.o | FileCheck %s --check-prefix=PARTIAL-CONTENTS
# RUN: ld.lld -e _start %t/use.o %t/contents.o -o %t/direct
# RUN: ld.lld -e _start %t/partial.o -o %t/staged
# RUN: llvm-readobj --symbols --relocations %t/staged \
# RUN:   | FileCheck %s --check-prefix=FINAL --implicit-check-not=R_MMIX_
# RUN: llvm-objcopy --dump-section=.text=%t/direct.text \
# RUN:   --dump-section=.MMIX.reg_contents=%t/direct.regs %t/direct
# RUN: llvm-objcopy --dump-section=.text=%t/staged.text \
# RUN:   --dump-section=.MMIX.reg_contents=%t/staged.regs %t/staged
# RUN: cmp %t/direct.text %t/staged.text
# RUN: cmp %t/direct.regs %t/staged.regs
# RUN: yaml2obj %t/invalid-fixed.yaml -o %t/invalid-fixed.o
# RUN: not ld.lld -r %t/invalid-fixed.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INVALID-FIXED
# RUN: yaml2obj %t/duplicate.yaml -o %t/duplicate.o
# RUN: not ld.lld -r %t/contents.o %t/duplicate.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=DUPLICATE

# PARTIAL:      Name: .MMIX.reg_contents
# PARTIAL:      Type: SHT_PROGBITS
# PARTIAL:      Flags [ (0x0)
# PARTIAL:      Address: 0x0
# PARTIAL:      Size: 8
# PARTIAL:      AddressAlignment: 8
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_REG
# PARTIAL:        Symbol: fixed_register
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_REG
# PARTIAL:        Symbol: content_register
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_BASE_PLUS_OFFSET
# PARTIAL:        Symbol: address
# PARTIAL:      Name: fixed_register
# PARTIAL-NEXT: Value: 0x20
# PARTIAL:      Section: Processor Specific (0xFF00)
# PARTIAL:      Name: content_register
# PARTIAL-NEXT: Value: 0x0
# PARTIAL:      Section: .MMIX.reg_contents
# PARTIAL-CONTENTS:      Contents of section .text:
# PARTIAL-CONTENTS-NEXT: 0000 22aa0000 22bb0000 23cc0000
# PARTIAL-CONTENTS:      Contents of section .MMIX.reg_contents:
# PARTIAL-CONTENTS-NEXT: 0000 11223344 55667788
# FINAL:      Relocations [
# FINAL-NEXT: ]
# FINAL:      Name: fixed_register
# FINAL-NEXT: Value: 0x20
# FINAL:      Name: content_register
# FINAL-NEXT: Value: 0xFE
# INVALID-FIXED: MMIX register symbol bad_register has invalid register number 256
# DUPLICATE: duplicate symbol: content_register

#--- use.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 22AA000022BB000023CC0000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 3, Type: R_MMIX_REG, Symbol: fixed_register }
      - { Offset: 7, Type: R_MMIX_REG, Symbol: content_register }
      - { Offset: 10, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: address }
Symbols:
  - { Name: _start, Section: .text, Binding: STB_GLOBAL, Type: STT_FUNC }
  - { Name: fixed_register, Index: 0xFF00, Value: 32, Binding: STB_GLOBAL }
  - { Name: content_register, Index: SHN_UNDEF, Binding: STB_GLOBAL }
  - { Name: address, Index: SHN_ABS, Value: 4096, Binding: STB_GLOBAL }

#--- contents.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: content_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- invalid-fixed.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Symbols:
  - { Name: bad_register, Index: 0xFF00, Value: 256, Binding: STB_GLOBAL }

#--- duplicate.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 99AABBCCDDEEFF00
Symbols:
  - { Name: content_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }
