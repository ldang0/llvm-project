long add_immediate(long value) {
  long result;
  __asm__ volatile("ADDU %0, %1, %2"
                   : "=r"(result)
                   : "r"(value), "I"(255));
  return result;
}

long fixed_registers(long value) {
  register long input __asm__("$1") = value;
  register long result __asm__("$0");
  __asm__ volatile("ADDU %0, %1, 1" : "=r"(result) : "r"(input));
  return result;
}

long load_memory(const long *address) {
  long result;
  __asm__ volatile("LDO %0, %1" : "=r"(result) : "m"(*address));
  return result;
}

void store_offsettable(long *address, long value) {
  __asm__ volatile("STOU %1, %0" : "=o"(*address) : "r"(value));
}

long load_address(const long *address) {
  long result;
  __asm__ volatile("LDO %0, %1" : "=r"(result) : "p"(address));
  return result;
}
