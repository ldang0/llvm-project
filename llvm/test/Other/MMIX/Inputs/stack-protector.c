volatile unsigned char *escaped_buffer;

#ifdef PROVIDE_GUARD
unsigned long long __stack_chk_guard = 0x1122334455667788ULL;
#endif

int protected_entry(unsigned char value) {
  volatile unsigned char buffer[16];
  buffer[0] = value;
  escaped_buffer = buffer;
  return buffer[0];
}

int unprotected_entry(unsigned char value) { return value + 1; }
