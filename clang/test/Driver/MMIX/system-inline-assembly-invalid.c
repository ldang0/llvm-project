// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -DTRAP_STATE -S %s -o %t.unsafe.s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=TRAP
// RUN: not test -s %t.unsafe.s
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -DNON_OFFSETTABLE -S %s -o %t.memory.s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=MEMORY
// RUN: not test -s %t.memory.s
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -DINVALID_CONSTRAINT -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CONSTRAINT

// RUN: not %clang --target=mmix -ffreestanding -DSAVE_STATE -S %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=SAVE
// RUN: not %clang --target=mmix -ffreestanding -DCALL_STATE -S %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALL
// RUN: not %clang --target=mmix -ffreestanding -DPUT_STATE -S %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT

// TRAP: error: error in backend: MMIX instruction 'TRAP' is only permitted in module-level inline assembly
// SAVE: error: error in backend: MMIX instruction 'SAVE' is only permitted in module-level inline assembly
// CALL: error: error in backend: MMIX instruction 'PUSHJ' is only permitted in module-level inline assembly
// PUT: error: error in backend: MMIX instruction 'PUT rG' is only permitted in module-level inline assembly
// MEMORY: error: error in backend: MMIX has no non-offsettable inline assembly memory operand
// CONSTRAINT: error: invalid input constraint 'x' in asm

#if defined(TRAP_STATE)
void trap_transition(void) { __asm__ volatile("TRAP 0, 0, 0" ::: "memory"); }
#elif defined(SAVE_STATE)
void context_save(void) { __asm__ volatile("SAVE r0" ::: "memory"); }
#elif defined(CALL_STATE)
void procedure_call(void) { __asm__ volatile("PUSHJ r31, target"); }
#elif defined(PUT_STATE)
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
