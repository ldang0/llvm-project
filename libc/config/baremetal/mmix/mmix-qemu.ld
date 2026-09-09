ENTRY(_start)

PHDRS {
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
  heap PT_LOAD FLAGS(6);
}

SECTIONS {
  . = 0x00100000;
  __llvm_libc_image_start = .;

  .text : ALIGN(0x2000) {
    KEEP(*(.text.init.enter))
    KEEP(*(.text.entry))
    *(.text .text.*)
  } :text

  . = ALIGN(0x2000);
  .rodata : ALIGN(0x2000) {
    *(.rodata .rodata.*)
  } :rodata

  /* Static bare-metal libunwind uses these bounds without frame registration. */
  .eh_frame : ALIGN(8) {
    __eh_frame_start = .;
    KEEP(*(.eh_frame .eh_frame.*))
    __eh_frame_end = .;
  } :rodata
  .eh_frame_hdr : ALIGN(8) {
    KEEP(*(.eh_frame_hdr))
  } :rodata
  __eh_frame_hdr_start = SIZEOF(.eh_frame_hdr) ? ADDR(.eh_frame_hdr) : 0;
  __eh_frame_hdr_end = SIZEOF(.eh_frame_hdr) ?
      ADDR(.eh_frame_hdr) + SIZEOF(.eh_frame_hdr) : 0;
  .gcc_except_table : ALIGN(8) {
    *(.gcc_except_table .gcc_except_table.*)
  } :rodata

  .preinit_array : ALIGN(8) {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } :data

  .init_array : ALIGN(8) {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN(__init_array_end = .);
  } :data

  .fini_array : ALIGN(8) {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } :data

  .data : ALIGN(0x2000) {
    __data_start = .;
    *(.data .data.*)
    __data_end = .;
  } :data
  __data_source = LOADADDR(.data);
  __data_size = SIZEOF(.data);

  .bss (NOLOAD) : ALIGN(0x2000) {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    __bss_end = .;
  } :data
  __bss_size = SIZEOF(.bss);

  . = ALIGN(0x2000);
  __llvm_libc_image_end = .;

  /* The heap is a fixed ELF reservation: QEMU reserves and clears the whole
     range at startup even when malloc never uses it. Override this link-time
     default when a smaller or larger bounded heap is appropriate. */
  PROVIDE(__llvm_libc_heap_size = 0x00800000);
  .heap (NOLOAD) : ALIGN(0x2000) {
    _end = .;
    . += __llvm_libc_heap_size;
    __llvm_libc_heap_limit = .;
  } :heap

  .MMIX.reg_contents : {
    KEEP(*(.MMIX.reg_contents.linker_allocated))
    KEEP(*(.MMIX.reg_contents))
  } :NONE

  .dynamic : { *(.dynamic) } :NONE
  .dynsym : { *(.dynsym) } :NONE
  .dynstr : { *(.dynstr) } :NONE
  .interp : { *(.interp) } :NONE
  .got : { *(.got .got.*) } :NONE
  .plt : { *(.plt .plt.*) } :NONE
  .tdata : { *(.tdata .tdata.*) } :NONE
  .tbss (NOLOAD) : { *(.tbss .tbss.*) } :NONE

  ASSERT(_start == 0x00100000,
         "MMIX LLVM libc process entry moved")
  ASSERT(__llvm_libc_heap_size >= 0x2000 &&
             (__llvm_libc_heap_size & 0x1fff) == 0,
         "MMIX LLVM libc heap size must be a positive multiple of 8 KiB")
  ASSERT(__llvm_libc_heap_limit <= 0x06000000,
         "MMIX LLVM libc process image exceeds the legacy QEMU ELF window")
  ASSERT(SIZEOF(.dynamic) + SIZEOF(.dynsym) + SIZEOF(.dynstr) +
             SIZEOF(.interp) + SIZEOF(.got) + SIZEOF(.plt) +
             SIZEOF(.tdata) + SIZEOF(.tbss) == 0,
         "MMIX LLVM libc process image contains unsupported sections")

  /DISCARD/ : { *(.comment) *(.note.GNU-stack) }
}
