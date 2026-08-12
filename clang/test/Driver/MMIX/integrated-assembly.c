// RUN: %clang -### --target=mmix-unknown-unknown -c \
// RUN:   %S/Inputs/canonical.s -o %t.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=JOB \
// RUN:       --implicit-check-not='"-cc1"' --implicit-check-not=-isysroot \
// RUN:       --implicit-check-not=qemu \
// RUN:       --implicit-check-not='{{[/\\](as|ld|gcc)[^/\\"]*"}}'

// RUN: %clang --target=mmix-unknown-unknown -c \
// RUN:   %S/Inputs/canonical.s -o %t.o
// RUN: llvm-readobj --file-headers --sections --symbols --relocations \
// RUN:   --expand-relocs %t.o | FileCheck %s --check-prefix=ELF
// RUN: llvm-objdump --no-print-imm-hex -dr %t.o \
// RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

// JOB: (in-process)
// JOB-NEXT: {{.*}}clang{{.*}} "-cc1as" "-triple" "mmix-unknown-unknown"
// JOB-SAME: "-filetype" "obj"
// JOB-SAME: "-main-file-name" "canonical.s"
// JOB-SAME: "-mrelocation-model" "static"
// JOB-SAME: "-o" "{{.*}}.o"
// JOB-SAME: "{{.*}}Inputs{{/|\\}}canonical.s"

// ELF: Format: elf64-mmix
// ELF-NEXT: Arch: mmix
// ELF-NEXT: AddressSize: 64bit
// ELF: DataEncoding: BigEndian
// ELF: Type: Relocatable
// ELF: Machine: EM_MMIX
// ELF: Name: .text
// ELF: Type: SHT_PROGBITS
// ELF: Name: .rela.text
// ELF: Type: SHT_RELA
// ELF: Name: .rodata
// ELF: Type: SHT_PROGBITS
// ELF: Name: .rela.rodata
// ELF: Type: SHT_RELA

// ELF: Type: R_MMIX_GETA (13)
// ELF-NEXT: Symbol: shared_data
// ELF: Type: R_MMIX_64 (5)
// ELF-NEXT: Symbol: external_data

// ELF: Name: local_callee
// ELF: Binding: Local
// ELF-NEXT: Type: Function
// ELF: Name: canonical_entry
// ELF: Binding: Global
// ELF-NEXT: Type: Function
// ELF: Name: shared_data
// ELF: Binding: Global
// ELF-NEXT: Type: Object
// ELF: Section: .rodata
// ELF: Name: external_data
// ELF: Section: Undefined

// DIS-LABEL: <canonical_entry>:
// DIS: SETL r1, 42
// DIS-NEXT: PUSHJ r31,
// DIS-NEXT: GETA r2, 0
// DIS-NEXT: {{.*}} R_MMIX_GETA shared_data
// DIS-NEXT: SWYM 0, 0, 0
// DIS-NEXT: SWYM 0, 0, 0
// DIS-NEXT: SWYM 0, 0, 0
// DIS-NEXT: LDOU r3, r2, 0
// DIS-NEXT: POP 0, 0

// DIS-LABEL: <local_callee>:
// DIS: ADDU r1, r1, 1
// DIS-NEXT: POP 0, 0
