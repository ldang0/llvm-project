# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/a.yaml -o %t/a.o
# RUN: yaml2obj %t/b.yaml -o %t/b.o
# RUN: ld.lld -e _start %t/a.o %t/b.o -o %t/executable
# RUN: llvm-readobj --sections --symbols %t/executable | FileCheck %s --check-prefix=STRUCTURE --implicit-check-not='Name: *REG*'
# RUN: llvm-objdump -s --section=.MMIX.reg_contents %t/executable | FileCheck %s --check-prefix=CONTENTS
# RUN: yaml2obj %t/invalid-symbols.yaml -o %t/invalid-symbols.o
# RUN: not ld.lld --error-limit=0 -e 0 %t/invalid-symbols.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID-SYMBOL
# RUN: yaml2obj %t/invalid-sections.yaml -o %t/invalid-sections.o
# RUN: not ld.lld --error-limit=0 -e 0 %t/invalid-sections.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID-SECTION
# RUN: yaml2obj %t/too-many.yaml -o %t/too-many.o
# RUN: not ld.lld -e 0 %t/too-many.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=TOO-MANY
# RUN: yaml2obj %t/duplicate-a.yaml -o %t/duplicate-a.o
# RUN: yaml2obj %t/duplicate-b.yaml -o %t/duplicate-b.o
# RUN: not ld.lld -e 0 %t/duplicate-a.o %t/duplicate-b.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=DUPLICATE

# STRUCTURE:      Name: .MMIX.reg_contents
# STRUCTURE:      Type: SHT_PROGBITS
# STRUCTURE:      Flags [ (0x0)
# STRUCTURE:      Address: 0x7E0
# STRUCTURE:      Size: 24
# STRUCTURE:      AddressAlignment: 8
# STRUCTURE:      Name: local_direct
# STRUCTURE-NEXT: Value: 0x20
# STRUCTURE:      Section: Processor Specific (0xFF00)
# STRUCTURE:      Name: local_content
# STRUCTURE-NEXT: Value: 0xFC
# STRUCTURE:      Section: Processor Specific (0xFF00)
# STRUCTURE:      Name: global_direct
# STRUCTURE-NEXT: Value: 0xFF
# STRUCTURE:      Section: Processor Specific (0xFF00)
# STRUCTURE:      Name: global_content
# STRUCTURE-NEXT: Value: 0xFD
# STRUCTURE:      Section: Processor Specific (0xFF00)
# STRUCTURE:      Name: other_content
# STRUCTURE-NEXT: Value: 0xFE
# STRUCTURE:      Section: Processor Specific (0xFF00)

# CONTENTS:      Contents of section .MMIX.reg_contents:
# CONTENTS-NEXT: 07e0 11223344 55667788 99aabbcc ddeeff00
# CONTENTS-NEXT: 07f0 01234567 89abcdef

# INVALID-SYMBOL-DAG: MMIX register symbol bad_low has invalid register number 31
# INVALID-SYMBOL-DAG: MMIX register symbol bad_high has invalid register number 256
# INVALID-SYMBOL-DAG: MMIX register symbol bad_type must use STT_NOTYPE
# INVALID-SYMBOL-DAG: MMIX register symbol bad_size must have size zero
# INVALID-SYMBOL-DAG: MMIX register-content symbol bad_content_align is not 8-byte aligned
# INVALID-SYMBOL-DAG: MMIX register-content symbol bad_content_range is outside its content section
# INVALID-SYMBOL-DAG: MMIX register-content symbol bad_content_type must use STT_NOTYPE
# INVALID-SYMBOL-DAG: MMIX register-content symbol bad_content_size must have size zero

# INVALID-SECTION-DAG: MMIX register contents must not be allocated
# INVALID-SECTION-DAG: MMIX register contents require 8-byte alignment
# INVALID-SECTION-DAG: MMIX register contents size is not a multiple of 8
# TOO-MANY: too many MMIX global register contents: 224, maximum is 223
# DUPLICATE: duplicate symbol: duplicate_content

#--- a.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: FD000000
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 112233445566778899AABBCCDDEEFF00
Symbols:
  - { Name: local_direct, Index: 0xFF00, Value: 32 }
  - { Name: local_content, Section: .MMIX.reg_contents, Value: 0 }
  - { Name: _start, Section: .text, Binding: STB_GLOBAL, Type: STT_FUNC }
  - { Name: global_direct, Index: 0xFF00, Value: 255, Binding: STB_GLOBAL }
  - { Name: global_content, Section: .MMIX.reg_contents, Value: 8, Binding: STB_GLOBAL }

#--- b.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0123456789ABCDEF
Symbols:
  - { Name: other_content, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- invalid-symbols.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
Symbols:
  - { Name: bad_low, Index: 0xFF00, Value: 31 }
  - { Name: bad_content_align, Section: .MMIX.reg_contents, Value: 1 }
  - { Name: bad_content_range, Section: .MMIX.reg_contents, Value: 8 }
  - { Name: bad_content_type, Section: .MMIX.reg_contents, Type: STT_OBJECT }
  - { Name: bad_content_size, Section: .MMIX.reg_contents, Size: 8 }
  - { Name: bad_high, Index: 0xFF00, Value: 256, Binding: STB_GLOBAL }
  - { Name: bad_type, Index: 0xFF00, Value: 32, Binding: STB_GLOBAL, Type: STT_OBJECT }
  - { Name: bad_size, Index: 0xFF00, Value: 32, Binding: STB_GLOBAL, Size: 8 }

#--- invalid-sections.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC ]
    AddressAlign: 4
    Content: 00000000000000

#--- too-many.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Size: 1792

#--- duplicate-a.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
Symbols:
  - { Name: duplicate_content, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- duplicate-b.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
Symbols:
  - { Name: duplicate_content, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }
