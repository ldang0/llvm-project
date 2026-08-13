// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x \
// RUN:   -S -emit-llvm %S/Inputs/inline-assembly.c -o %t.ll
// RUN: FileCheck %s --check-prefix=IR < %t.ll

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x \
// RUN:   -S %S/Inputs/inline-assembly.c -o %t.s
// RUN: FileCheck %s --check-prefix=ASM --implicit-check-not=MMIXAL < %t.s

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x \
// RUN:   -c %S/Inputs/inline-assembly.c -o %t.o
// RUN: llvm-readobj --file-headers --symbols --relocations %t.o \
// RUN:   | FileCheck %s --check-prefix=ELF --implicit-check-not=R_MMIX_
// RUN: llvm-objdump --no-print-imm-hex -dr %t.o \
// RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

// IR-LABEL: define dso_local i64 @add_immediate(
// IR: call i64 asm sideeffect "ADDU $0, $1, $2", "=r,r,I"(i64 %{{[0-9]+}}, i32 255)
// IR-LABEL: define dso_local i64 @fixed_registers(
// IR: call i64 asm sideeffect "ADDU $0, $1, 1", "={r0},{r1}"(i64 %{{[0-9]+}})
// IR-LABEL: define dso_local i64 @load_memory(
// IR: call i64 asm sideeffect "LDO $0, $1", "=r,*m"(ptr elementtype(i64) %{{[0-9]+}})
// IR-LABEL: define dso_local void @store_offsettable(
// IR: call void asm sideeffect "STOU $1, $0", "=*o,r"(ptr elementtype(i64) %{{[0-9]+}}, i64 %{{[0-9]+}})
// IR-LABEL: define dso_local i64 @load_address(
// IR: call i64 asm sideeffect "LDO $0, $1", "=r,p"(ptr %{{[0-9]+}})

// ASM-LABEL: add_immediate:
// ASM: #APP
// ASM-NEXT: ADDU [[ADD:r[0-9]+]], [[ADD]], 255
// ASM: #NO_APP
// ASM-LABEL: fixed_registers:
// ASM: #APP
// ASM-NEXT: ADDU r0, r1, 1
// ASM: #NO_APP
// ASM-LABEL: load_memory:
// ASM: #APP
// ASM-NEXT: LDO [[LOAD:r[0-9]+]], [[LOAD]], 0
// ASM: #NO_APP
// ASM-LABEL: store_offsettable:
// ASM: #APP
// ASM-NEXT: STOU {{r[0-9]+}}, {{r[0-9]+}}, 0
// ASM: #NO_APP
// ASM-LABEL: load_address:
// ASM: #APP
// ASM-NEXT: LDO [[ADDRESS:r[0-9]+]], [[ADDRESS]], 0
// ASM: #NO_APP

// ELF: Format: elf64-mmix
// ELF-NEXT: Arch: mmix
// ELF: Type: Relocatable
// ELF: Relocations [
// ELF-NEXT: ]
// ELF: Name: add_immediate
// ELF: Type: Function
// ELF: Name: fixed_registers
// ELF: Type: Function
// ELF: Name: load_memory
// ELF: Type: Function
// ELF: Name: store_offsettable
// ELF: Type: Function
// ELF: Name: load_address
// ELF: Type: Function

// DIS-LABEL: <add_immediate>:
// DIS: ADDU [[DADD:r[0-9]+]], [[DADD]], 255
// DIS-LABEL: <fixed_registers>:
// DIS: ADDU r0, r1, 1
// DIS-LABEL: <load_memory>:
// DIS: LDO [[DLOAD:r[0-9]+]], [[DLOAD]], 0
// DIS-LABEL: <store_offsettable>:
// DIS: STOU {{r[0-9]+}}, {{r[0-9]+}}, 0
// DIS-LABEL: <load_address>:
// DIS: LDO [[DADDRESS:r[0-9]+]], [[DADDRESS]], 0
