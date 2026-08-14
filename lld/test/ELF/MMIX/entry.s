# REQUIRES: mmix

# RUN: yaml2obj --docnum=1 %s -o %t.o
# RUN: ld.lld %t.o -o %t-default
# RUN: llvm-readobj --file-headers --symbols --program-headers %t-default \
# RUN:   | FileCheck %s --check-prefix=DEFAULT
# RUN: ld.lld -e explicit %t.o -o %t-explicit
# RUN: llvm-readobj --file-headers --symbols --program-headers %t-explicit \
# RUN:   | FileCheck %s --check-prefix=EXPLICIT
# RUN: echo 'ENTRY(script_entry)' > %t.script
# RUN: ld.lld -T %t.script %t.o -o %t-script
# RUN: llvm-readobj --file-headers --symbols %t-script \
# RUN:   | FileCheck %s --check-prefix=SCRIPT
# RUN: ld.lld -T %t.script -e explicit %t.o -o %t-override
# RUN: llvm-readobj --file-headers --symbols %t-override \
# RUN:   | FileCheck %s --check-prefix=EXPLICIT
# RUN: ld.lld -e 0x1234 %t.o -o %t-numeric
# RUN: llvm-readobj --file-headers %t-numeric \
# RUN:   | FileCheck %s --check-prefix=NUMERIC
# RUN: yaml2obj --docnum=2 %s -o %t-no-entry.o
# RUN: ld.lld %t-no-entry.o -o %t-no-entry 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MISSING
# RUN: llvm-readobj --file-headers %t-no-entry \
# RUN:   | FileCheck %s --check-prefix=FALLBACK

## The conventional _start symbol supplies the default entry. Its .text
## section is mapped by the executable PT_LOAD shown below.
# DEFAULT:      Entry: [[DEFAULT_ENTRY:0x[0-9A-F]+]]
# DEFAULT:      Type: PT_LOAD
# DEFAULT:      Flags [ (0x4)
# DEFAULT-NEXT: PF_R
# DEFAULT:      Type: PT_LOAD
# DEFAULT-NEXT: Offset:
# DEFAULT-NEXT: VirtualAddress: [[DEFAULT_ENTRY]]
# DEFAULT:      Flags [ (0x5)
# DEFAULT-NEXT: PF_R
# DEFAULT-NEXT: PF_X
# DEFAULT:      Name: _start
# DEFAULT-NEXT: Value: [[DEFAULT_ENTRY]]
# DEFAULT:      Section: .text

## -e selects a named symbol and takes precedence over script ENTRY.
# EXPLICIT:      Entry: [[EXPLICIT_ENTRY:0x[0-9A-F]+]]
# EXPLICIT:      Name: explicit
# EXPLICIT-NEXT: Value: [[EXPLICIT_ENTRY]]

# SCRIPT:      Entry: [[SCRIPT_ENTRY:0x[0-9A-F]+]]
# SCRIPT:      Name: script_entry
# SCRIPT-NEXT: Value: [[SCRIPT_ENTRY]]

# NUMERIC: Entry: 0x1234

## Without -e, ENTRY, or _start, lld warns and uses the generic zero fallback.
# MISSING: warning: cannot find entry symbol _start; not setting start address
# FALLBACK: Entry: 0x0

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
    Content:      FD000000FD000000FD000000
Symbols:
  - Name:    _start
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL
    Value:   0
  - Name:    explicit
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL
    Value:   4
  - Name:    script_entry
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL
    Value:   8

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
