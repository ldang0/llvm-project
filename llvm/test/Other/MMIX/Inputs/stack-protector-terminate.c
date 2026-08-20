__attribute__((noreturn)) void __mmix_stack_chk_terminate(void) {
  for (;;)
    __asm__ volatile("SWYM 1, 0, 0");
}
