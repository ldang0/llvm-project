// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x -O1 \
// RUN:   -S -emit-llvm %S/Inputs/global-data.c -o %t.ll
// RUN: FileCheck %s --check-prefix=IR < %t.ll

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x -O1 \
// RUN:   -S %S/Inputs/global-data.c -o %t.s
// RUN: FileCheck %s --check-prefix=ASM --implicit-check-not=MMIXAL < %t.s

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x -O1 \
// RUN:   -c %S/Inputs/global-data.c -o %t.o
// RUN: llvm-readobj --file-headers --sections --section-data --symbols \
// RUN:   --relocations --expand-relocs %t.o \
// RUN:   | FileCheck %s --check-prefix=ELF \
// RUN:       --implicit-check-not='Section: Common' \
// RUN:       --implicit-check-not=STT_TLS --implicit-check-not=SHT_DYNAMIC
// RUN: llvm-objdump --no-print-imm-hex -dr %t.o \
// RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

// IR: @initialized_global ={{.*}} global i64 1234605616436508552, align 8
// IR: @zero_initialized_global ={{.*}} global i64 0, align 8
// IR: @readonly_global ={{.*}} constant i32 16909060, align 4
// IR: @defined_data_pointer ={{.*}} global ptr @initialized_global, align 8
// IR: @external_global = external{{.*}} global i64, align 8
// IR: @external_data_pointer ={{.*}} global ptr @external_global, align 8
// IR: @external_data_addend_pointer ={{.*}} global ptr getelementptr (i8, ptr @external_global, i64 24), align 8
// IR: @internal_global = internal global i16 4660, align 4
// IR: @internal_data_pointer ={{.*}} global ptr @internal_global, align 8
// IR: @defined_function_pointer ={{.*}} global ptr @internal_function, align 8
// IR: @external_function_pointer ={{.*}} global ptr @external_function, align 8
// IR: @readonly_addend_pointer ={{.*}} global ptr getelementptr inbounds{{.*}} (i8, ptr @readonly_global, i64 2), align 8
// The default tentative definition is zero storage, not common allocation.
// IR: @tentative_global ={{.*}} global i64 0, align 8
// IR-LABEL: define internal void @internal_function()
// IR: declare dso_local void @external_function()
// IR-LABEL: define dso_local{{.*}} ptr @address_defined_data()
// IR: ret ptr @initialized_global
// IR-LABEL: define dso_local{{.*}} ptr @address_external_data()
// IR: ret ptr @external_global
// IR-LABEL: define dso_local{{.*}} ptr @address_defined_function()
// IR: ret ptr @internal_function
// IR-LABEL: define dso_local{{.*}} ptr @address_external_function()
// IR: ret ptr @external_function

// ASM-LABEL: address_defined_data:
// ASM: SETH r231, (initialized_global>>48)&65535
// ASM: INCMH r231, (initialized_global>>32)&65535
// ASM: INCML r231, (initialized_global>>16)&65535
// ASM: INCL r231, initialized_global&65535
// ASM-LABEL: address_external_data:
// ASM: SETH r231, (external_global>>48)&65535
// ASM-LABEL: address_defined_function:
// ASM: SETH r231, (internal_function>>48)&65535
// ASM-LABEL: address_external_function:
// ASM: SETH r231, (external_function>>48)&65535
// ASM-LABEL: initialized_global:
// ASM-NEXT: .8byte 1234605616436508552
// ASM: .section .bss,"aw",@nobits
// ASM-LABEL: zero_initialized_global:
// ASM-NEXT: .8byte 0
// ASM: .section .rodata,"a",@progbits
// ASM-LABEL: readonly_global:
// ASM-NEXT: .4byte 16909060
// ASM-LABEL: defined_data_pointer:
// ASM-NEXT: .8byte initialized_global
// ASM-LABEL: external_data_pointer:
// ASM-NEXT: .8byte external_global
// ASM-LABEL: external_data_addend_pointer:
// ASM-NEXT: .8byte external_global+24
// ASM-LABEL: internal_global:
// ASM-NEXT: .2byte 4660
// ASM-LABEL: internal_data_pointer:
// ASM-NEXT: .8byte internal_global
// ASM-LABEL: defined_function_pointer:
// ASM-NEXT: .8byte internal_function
// ASM-LABEL: external_function_pointer:
// ASM-NEXT: .8byte external_function
// ASM-LABEL: readonly_addend_pointer:
// ASM-NEXT: .8byte readonly_global+2
// ASM: .section .bss,"aw",@nobits
// ASM-LABEL: tentative_global:
// ASM-NEXT: .8byte 0

// ELF: Format: elf64-mmix
// ELF-NEXT: Arch: mmix
// ELF: DataEncoding: BigEndian
// ELF: Type: Relocatable
// ELF: Machine: EM_MMIX
// ELF: Name: .text
// ELF-NEXT: Type: SHT_PROGBITS
// ELF: Name: .rela.text
// ELF-NEXT: Type: SHT_RELA
// ELF: Name: .data
// ELF-NEXT: Type: SHT_PROGBITS
// ELF: Size: 72
// ELF: AddressAlignment: 8
// ELF: 0000: 11223344 55667788 00000000 00000000
// ELF: 0020: 12340000 00000000 00000000 00000000
// ELF: Name: .rela.data
// ELF-NEXT: Type: SHT_RELA
// ELF: Name: .bss
// ELF-NEXT: Type: SHT_NOBITS
// ELF: Size: 16
// ELF: AddressAlignment: 8
// ELF: Name: .rodata
// ELF-NEXT: Type: SHT_PROGBITS
// ELF: Size: 4
// ELF: AddressAlignment: 4
// ELF: 0000: 01020304

// ELF: Section {{.*}} .rela.text {
// ELF: Type: R_MMIX_GETA (13)
// ELF-NEXT: Symbol: initialized_global
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_GETA (13)
// ELF-NEXT: Symbol: external_global
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_GETA (13)
// ELF-NEXT: Symbol: .text
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_GETA (13)
// ELF-NEXT: Symbol: external_function
// ELF-NEXT: Addend: 0x0
// ELF: Section {{.*}} .rela.data {
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: initialized_global
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: external_global
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: external_global
// ELF-NEXT: Addend: 0x18
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: .data
// ELF-NEXT: Addend: 0x20
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: .text
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: external_function
// ELF-NEXT: Addend: 0x0
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: readonly_global
// ELF-NEXT: Addend: 0x2

// ELF: Name: internal_function
// ELF: Binding: Local
// ELF-NEXT: Type: Function
// ELF: Name: internal_global
// ELF: Binding: Local
// ELF-NEXT: Type: Object
// ELF: Name: initialized_global
// ELF: Binding: Global
// ELF-NEXT: Type: Object
// ELF: Section: .data
// Undefined symbols remain unresolved in this relocatable object.
// ELF: Name: external_global
// ELF: Section: Undefined
// ELF: Name: external_function
// ELF: Section: Undefined
// ELF: Name: zero_initialized_global
// ELF: Type: Object
// ELF: Section: .bss
// ELF: Name: readonly_global
// ELF: Type: Object
// ELF: Section: .rodata
// ELF: Name: tentative_global
// ELF: Type: Object
// ELF: Section: .bss

// DIS-LABEL: <address_defined_data>:
// DIS: GETA r231, 0
// DIS-NEXT: {{.*}} R_MMIX_GETA initialized_global
// DIS-LABEL: <address_external_data>:
// DIS: GETA r231, 0
// DIS-NEXT: {{.*}} R_MMIX_GETA external_global
// DIS-LABEL: <address_defined_function>:
// DIS: GETA r231, 0
// DIS-NEXT: {{.*}} R_MMIX_GETA .text
// DIS-LABEL: <address_external_function>:
// DIS: GETA r231, 0
// DIS-NEXT: {{.*}} R_MMIX_GETA external_function
