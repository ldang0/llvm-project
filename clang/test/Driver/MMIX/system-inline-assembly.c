// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 -O1 \
// RUN:   -S -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 -O1 \
// RUN:   -S %s -o - | FileCheck %s --check-prefix=ASM
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 -O1 \
// RUN:   -c %s -o %t.o
// RUN: llvm-readobj --file-headers --relocations %t.o \
// RUN:   | FileCheck %s --check-prefix=OBJ
// RUN: llvm-objdump --no-print-imm-hex -dr %t.o \
// RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

typedef unsigned long u64;

u64 read_translation_state(void) {
  u64 value;
  __asm__ volatile("GET %0, rV" : "=r"(value));
  return value;
}

void write_translation_state(u64 value) {
  __asm__ volatile("PUT rV, %0" : : "r"(value) : "memory");
}

u64 read_interrupt_mask(void) {
  u64 value;
  __asm__ volatile("GET %0, rK" : "=r"(value));
  return value;
}

void write_interrupt_mask(u64 value) {
  __asm__ volatile("PUT rK, %0" : : "r"(value) : "memory");
}

void system_barriers(void) {
  __asm__ volatile("SYNC 3" : : : "memory");
  __asm__ volatile("SYNC 6" : : : "memory");
}

u64 add_small_immediate(u64 value) {
  __asm__ volatile("ADDU %0, %0, %1" : "+r"(value) : "I"(255));
  return value;
}

void local_control_label(u64 condition) {
  __asm__ volatile("BZ %0, .Ldone%=\n\tSWYM 0, 0, 0\n.Ldone%=:\n"
                   :
                   : "r"(condition)
                   : "memory");
}

u64 load_with_memory_operand(const volatile u64 *address) {
  u64 value;
  __asm__ volatile("LDOU %0, %1" : "=r"(value) : "m"(*address) : "memory");
  return value;
}

// IR-LABEL: define {{.*}} i64 @read_translation_state(
// IR: call i64 asm sideeffect "GET $0, rV", "=r"
// IR-LABEL: define {{.*}} void @write_translation_state(
// IR: call void asm sideeffect "PUT rV, $0", "r,~{memory}"
// IR-LABEL: define {{.*}} i64 @read_interrupt_mask(
// IR: call i64 asm sideeffect "GET $0, rK", "=r"
// IR-LABEL: define {{.*}} void @write_interrupt_mask(
// IR: call void asm sideeffect "PUT rK, $0", "r,~{memory}"
// IR-LABEL: define {{.*}} void @system_barriers(
// IR: call void asm sideeffect "SYNC 3", "~{memory}"
// IR: call void asm sideeffect "SYNC 6", "~{memory}"
// IR-LABEL: define {{.*}} i64 @add_small_immediate(
// IR: call i64 asm sideeffect "ADDU $0, $0, $1", "=r,I,0"
// IR-LABEL: define {{.*}} void @local_control_label(
// IR: call void asm sideeffect "BZ $0, .Ldone${:uid}\0A\09SWYM 0, 0, 0\0A.Ldone${:uid}:\0A", "r,~{memory}"
// IR-LABEL: define {{.*}} i64 @load_with_memory_operand(
// IR: call i64 asm sideeffect "LDOU $0, $1", "=r,*m,~{memory}"

// ASM-LABEL: read_translation_state:
// ASM: GET {{r[0-9]+}}, rV
// ASM-LABEL: write_translation_state:
// ASM: PUT rV, {{r[0-9]+}}
// ASM-LABEL: read_interrupt_mask:
// ASM: GET {{r[0-9]+}}, rK
// ASM-LABEL: write_interrupt_mask:
// ASM: PUT rK, {{r[0-9]+}}
// ASM-LABEL: system_barriers:
// ASM: SYNC 3
// ASM: SYNC 6
// ASM-LABEL: add_small_immediate:
// ASM: ADDU {{r[0-9]+}}, {{r[0-9]+}}, 255
// ASM-LABEL: local_control_label:
// ASM: BZ {{r[0-9]+}}, .Ldone{{[0-9]+}}
// ASM: SWYM 0, 0, 0
// ASM: .Ldone{{[0-9]+}}:
// ASM-LABEL: load_with_memory_operand:
// ASM: LDOU {{r[0-9]+}}, {{r[0-9]+}}, 0

// OBJ: Format: elf64-mmix
// OBJ-NEXT: Arch: mmix
// OBJ: Type: Relocatable
// OBJ-NEXT: Machine: EM_MMIX
// OBJ: Relocations [
// OBJ-NEXT: ]

// DIS-LABEL: <read_translation_state>:
// DIS: GET {{r[0-9]+}}, rV
// DIS-LABEL: <write_translation_state>:
// DIS: PUT rV, {{r[0-9]+}}
// DIS-LABEL: <read_interrupt_mask>:
// DIS: GET {{r[0-9]+}}, rK
// DIS-LABEL: <write_interrupt_mask>:
// DIS: PUT rK, {{r[0-9]+}}
// DIS-LABEL: <system_barriers>:
// DIS: SYNC 3
// DIS-NEXT: {{.*}} SYNC 6
// DIS-LABEL: <add_small_immediate>:
// DIS: ADDU {{r[0-9]+}}, {{r[0-9]+}}, 255
// DIS-LABEL: <local_control_label>:
// DIS: BZ {{r[0-9]+}}, 2
// DIS-NEXT: {{.*}} SWYM 0, 0, 0
// DIS-LABEL: <load_with_memory_operand>:
// DIS: LDOU {{r[0-9]+}}, {{r[0-9]+}}, 0
