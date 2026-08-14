# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/unreachable.s -o %t/unreachable.o
# RUN: ld.lld -T %t/layout.lds %t/input.o -o %t/out
# RUN: llvm-readobj --file-headers --sections --program-headers --symbols \
# RUN:   --relocations %t/out | FileCheck %s --check-prefix=LAYOUT
# RUN: not ld.lld -T %t/assert.lds %t/input.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ASSERT
# RUN: not ld.lld -T %t/stub-range.lds %t/unreachable.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=STUB-RANGE

# LAYOUT:      Entry: [[ENTRY:0x[0-9A-F]+]]
# LAYOUT:      Name: .text
# LAYOUT:      Name: .MMIX.reg_contents
# LAYOUT:      Name: .data
# LAYOUT:      Name: .bss
# LAYOUT:      Type: PT_LOAD
# LAYOUT:      Type: PT_LOAD
# LAYOUT:      Relocations [
# LAYOUT-NEXT: ]
# LAYOUT:      Name: __MMIX_call_stub_0
# LAYOUT:      Name: provided_text_end
# LAYOUT-NEXT: Value: [[TEXT_END:0x[0-9A-F]+]]
# LAYOUT:      Name: image_end
# LAYOUT-NEXT: Value: 0x200160
# LAYOUT:      Name: text_end
# LAYOUT-NEXT: Value: [[TEXT_END]]
# LAYOUT:      Name: image_start
# LAYOUT-NEXT: Value: [[ENTRY]]
# ASSERT:      error: static MMIX image layout assertion failed
# STUB-RANGE:  error: {{.*}}relocation R_MMIX_PUSHJ_STUBBABLE against symbol cannot reach its section-end stub

#--- input.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.entry
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F4000000FD000000FD000000FD000000F200000023AA0000
  - Name: .text.keep
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: FD010203
  - Name: .rela.text.entry
    Type: SHT_RELA
    Link: .symtab
    Info: .text.entry
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA, Symbol: data_object }
      - { Offset: 16, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: far_target }
      - { Offset: 22, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: data_object }
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
      - { Offset: 0, Type: R_MMIX_64, Symbol: provided_text_end }
  - Name: .bss
    Type: SHT_NOBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 16
    Size: 32
Symbols:
  - { Name: _start, Section: .text.entry, Binding: STB_GLOBAL, Type: STT_FUNC }
  - { Name: kept_code, Section: .text.keep, Binding: STB_GLOBAL }
  - { Name: data_object, Section: .data, Binding: STB_GLOBAL }
  - { Name: far_target, Index: SHN_ABS, Value: 1234605616436508552, Binding: STB_GLOBAL }
  - { Name: provided_text_end, Index: SHN_UNDEF, Binding: STB_GLOBAL }

#--- layout.lds
ENTRY(_start)
MEMORY {
  rom (rx) : ORIGIN = 0x100000, LENGTH = 0x10000
  ram (rw) : ORIGIN = 0x200000, LENGTH = 0x10000
}
PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}
SECTIONS {
  . = ORIGIN(rom) + 0x100;
  .text : AT(0x300000) ALIGN(0x20) {
    image_start = .;
    *(.text.entry)
    KEEP(*(.text.keep))
    . = ALIGN(0x20);
    text_end = .;
  } > rom :text
  PROVIDE(provided_text_end = text_end);
  .MMIX.reg_contents 0x200000 : AT(0x310000) {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
  .data ORIGIN(ram) + 0x100 : AT(0x310100) { *(.data*) } > ram :data
  .bss (NOLOAD) : ALIGN(0x40) { *(.bss*) } > ram :data
  image_end = .;
  ASSERT(image_end <= ORIGIN(ram) + LENGTH(ram),
         "static MMIX image layout assertion failed")
}

#--- assert.lds
SECTIONS {
  .text : { *(.text*) }
  PROVIDE(provided_text_end = .);
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
  .data : { *(.data*) }
  ASSERT(SIZEOF(.text) == 0, "static MMIX image layout assertion failed")
}

#--- stub-range.lds
SECTIONS { .text 0 : { *(.text) } }

#--- unreachable.s
.text
.global unreachable
unreachable:
  PUSHJ r1, unreachable_target
  .space 0x40000
.set unreachable_target, 0x1122334455667788
