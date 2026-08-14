# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld -T %t/layout.lds %t/input.o -o %t/layout
# RUN: llvm-readobj --file-headers --symbols %t/layout \
# RUN:   | FileCheck %s --check-prefix=ENTRY
# RUN: llvm-readobj --elf-output-style=GNU --sections --program-headers \
# RUN:   %t/layout | FileCheck %s --check-prefix=LAYOUT
# RUN: not ld.lld -T %t/overflow.lds %t/input.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OVERFLOW
# RUN: not ld.lld -T %t/no-region.lds %t/input.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-REGION
# RUN: not ld.lld -T %t/overlap.lds %t/input.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OVERLAP
# RUN: not ld.lld -T %t/invalid-entry.lds %t/input.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INVALID-ENTRY

## ENTRY selects a symbol in the explicitly executable text segment.
# ENTRY:      Entry: [[ENTRY_ADDR:0x[0-9A-F]+]]
# ENTRY:      Name: script_entry
# ENTRY-NEXT: Value: [[ENTRY_ADDR]]
# ENTRY:      Section: .text

## VMA, LMA (PhysAddr), and file offset are independent. The script places
## code and read-only data in ROM, while writable and zero storage use RAM.
# LAYOUT:      Name              Type            Address          Off    Size
# LAYOUT:      .text             PROGBITS        0000000000100100 000100 000020 {{.*}} AX {{.*}} 32
# LAYOUT-NEXT: .rodata           PROGBITS        0000000000100140 000140 000008 {{.*}} A  {{.*}} 64
# LAYOUT-NEXT: .data             PROGBITS        0000000000200080 001080 000008 {{.*}} WA {{.*}} 16
# LAYOUT-NEXT: .bss              NOBITS          0000000000200100 001088 000030 {{.*}} WA {{.*}} 128

# LAYOUT:      Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
# LAYOUT-NEXT: LOAD           0x000100 0x0000000000100100 0x0000000000300000 0x000048 0x000048 R E 0x1000
# LAYOUT-NEXT: LOAD           0x001080 0x0000000000200080 0x0000000000300100 0x000008 0x0000b0 RW  0x1000
# LAYOUT:      Section to Segment mapping:
# LAYOUT-NEXT: Segment Sections...
# LAYOUT-NEXT: 00     .text .rodata
# LAYOUT-NEXT: 01     .data .bss

# OVERFLOW: error: section '.text' will not fit in region 'tiny': overflowed by 7 bytes
# NO-REGION: error: no memory region specified for section '.text'
# OVERLAP: error: section .text virtual address range overlaps with .data
# INVALID-ENTRY: error: {{.*}}invalid-entry.lds:1: ) expected, but got +

#--- input.yaml
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
    Content:      FD000000FD000000
  - Name:         .rodata
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC ]
    AddressAlign: 8
    Content:      0123456789ABCDEF
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content:      FEDCBA9876543210
  - Name:         .bss
    Type:         SHT_NOBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 16
    Size:         32
  - Name:         .scratch
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 16
    Content:      00000000000000000000000000000000
Symbols:
  - Name:    script_entry
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL

#--- layout.lds
ENTRY(script_entry)

MEMORY {
  rom (rx) : ORIGIN = 0x100000, LENGTH = 0x1000
  ram (rw) : ORIGIN = 0x200000, LENGTH = 0x1000
}

PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS {
  .text ORIGIN(rom) + 0x100 : AT(0x300000) ALIGN(0x20) {
    *(.text)
    . = ALIGN(0x20);
  } > rom :text
  .rodata : ALIGN(0x40) { *(.rodata) } > rom :text

  .data ORIGIN(ram) + 0x80 : AT(0x300100) ALIGN(0x10) {
    *(.data)
  } > ram :data
  .bss (NOLOAD) : ALIGN(0x80) { *(.bss) *(.scratch) } > ram :data
}

#--- overflow.lds
MEMORY { tiny (rx) : ORIGIN = 0x1000, LENGTH = 1 }
SECTIONS { .text : { *(.text) } > tiny }

#--- overlap.lds
SECTIONS {
  .text 0x1000 : { *(.text) }
  .data 0x1000 : { *(.data) }
}

#--- no-region.lds
MEMORY { data (!rx) : ORIGIN = 0x1000, LENGTH = 0x1000 }
SECTIONS { .text : { *(.text) } }

#--- invalid-entry.lds
ENTRY(script_entry + 4)
SECTIONS { .text : { *(.text) } }
