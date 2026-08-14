ENTRY(llvm_entry)
SECTIONS {
  .text 0x1000 : { *(.text*) }
  .MMIX.reg_contents 0x2000 : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
  .data 0x3000 : { *(.data*) }
  .bss 0x4000 (NOLOAD) : { *(.bss*) *(COMMON) }
}
