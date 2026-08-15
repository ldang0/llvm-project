// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -DUNSAFE_STATE -S %s -o %t.unsafe.s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=UNSAFE
// RUN: not test -s %t.unsafe.s
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -DNON_OFFSETTABLE -S %s -o %t.memory.s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MEMORY
// RUN: not test -s %t.memory.s
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -DINVALID_CONSTRAINT -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CONSTRAINT

// UNSAFE-DAG: error: MMIX instruction 'TRAP' is only permitted in module-level inline assembly
// UNSAFE-DAG: error: MMIX instruction 'SAVE' is only permitted in module-level inline assembly
// UNSAFE-DAG: error: MMIX instruction 'PUSHJ' is only permitted in module-level inline assembly
// UNSAFE-DAG: error: MMIX instruction 'PUT rG' is only permitted in module-level inline assembly
// MEMORY: error: MMIX has no non-offsettable inline assembly memory operand
// CONSTRAINT: error: invalid input constraint 'x' in asm

#if defined(UNSAFE_STATE)
void trap_transition(void) { __asm__ volatile("TRAP 0, 0, 0" ::: "memory"); }
void context_save(void) { __asm__ volatile("SAVE r0" ::: "memory"); }
void procedure_call(void) { __asm__ volatile("PUSHJ r31, target"); }
void abi_state_write(void) { __asm__ volatile("PUT rG, 231"); }
#elif defined(NON_OFFSETTABLE)
unsigned long non_offsettable(volatile unsigned long *ptr) {
  unsigned long value;
  __asm__ volatile("LDOU %0, %1" : "=r"(value) : "V"(*ptr) : "memory");
  return value;
}
#elif defined(INVALID_CONSTRAINT)
void invalid_constraint(unsigned long value) {
  __asm__ volatile("SWYM 0, 0, 0" : : "x"(value));
}
#endif
