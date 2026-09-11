#if defined(TEST_INVALID_NAME)
void invalid_name(void) { __asm__ volatile("" ::: "$256"); }
#elif defined(TEST_RESERVED_VALUES)
long unstable_global(long value) {
  register long fixed __asm__("$32") = value;
  __asm__ volatile("" : "+r"(fixed));
  return fixed;
}

long hidden_result_role(long value) {
  register long fixed __asm__("$251") = value;
  __asm__ volatile("" : "+r"(fixed));
  return fixed;
}

long static_chain_role(long value) {
  register long fixed __asm__("$252") = value;
  __asm__ volatile("" : "+r"(fixed));
  return fixed;
}

long frame_pointer_role(long value) {
  register long fixed __asm__("$253") = value;
  __asm__ volatile("" : "+r"(fixed));
  return fixed;
}

long stack_pointer_role(long value) {
  register long fixed __asm__("$254") = value;
  __asm__ volatile("" : "+r"(fixed));
  return fixed;
}

long scratch_role(long value) {
  register long fixed __asm__("$255") = value;
  __asm__ volatile("" : "+r"(fixed));
  return fixed;
}
#elif defined(TEST_UNSAFE_STATE)
void stack_pointer_clobber(void) {
  __asm__ volatile("SWYM 0, 0, 0" ::: "sp");
}

#elif defined(TEST_STATE_WRITE)
void procedure_state_write(void) { __asm__ volatile("PUT rJ, r0"); }

#elif defined(TEST_PROCEDURE_CALL)
void procedure_call(void) { __asm__ volatile("PUSHJ r31, target"); }
#else
struct Large {
  long words[3];
};

extern long external_many(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15);
extern struct Large external_large(long value);

long scalar_roles(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15) {
  volatile long local = a0;
  return external_many(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
                       a12, a13, a14, a15) + local;
}

struct Large indirect_result_role(long value) {
  return external_large(value);
}
#endif
