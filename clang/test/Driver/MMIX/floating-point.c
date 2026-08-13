// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x -O1 \
// RUN:   -S -emit-llvm %S/Inputs/floating-point.c -o %t.ll
// RUN: FileCheck %s --check-prefix=IR --implicit-check-not=fp128 \
// RUN:   --implicit-check-not=x86_fp80 < %t.ll

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x -O1 \
// RUN:   -S %S/Inputs/floating-point.c -o %t.s
// RUN: FileCheck %s --check-prefix=ASM --implicit-check-not=MMIXAL \
// RUN:   --implicit-check-not='PUT rA' < %t.s

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x -O1 \
// RUN:   -c %S/Inputs/floating-point.c -o %t.o
// RUN: llvm-readobj --file-headers --sections --symbols --relocations \
// RUN:   --expand-relocs %t.o | FileCheck %s --check-prefix=ELF
// RUN: llvm-objdump --no-print-imm-hex -dr %t.o \
// RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

// IR-LABEL: define dso_local{{.*}} float @float_arithmetic(
// IR-SAME: float noundef
// IR: fadd float
// IR: fmul float
// IR: ret float
// IR-LABEL: define dso_local{{.*}} double @double_arithmetic(
// IR-SAME: double noundef
// IR: fsub double
// IR: fdiv double
// IR: ret double
// A distinct C long double uses the frozen binary64 LLVM representation.
// IR-LABEL: define dso_local{{.*}} double @long_double_arithmetic(
// IR: fadd double
// IR-LABEL: define dso_local{{.*}} i32 @compare_float(
// IR: fcmp ole float
// IR-LABEL: define dso_local{{.*}} i32 @compare_double(
// IR: fcmp une double
// IR-LABEL: define dso_local{{.*}} double @widen_float(
// IR: fpext float
// IR-LABEL: define dso_local{{.*}} float @narrow_double(
// IR: fptrunc double
// IR-LABEL: define dso_local{{.*}} double @signed_to_double(
// IR: sitofp i64
// IR-LABEL: define dso_local i64 @double_to_signed(
// IR: fptosi double
// IR-LABEL: define dso_local float @fused_float(
// IR: call float @fmaf(float noundef
// IR-LABEL: define dso_local double @remainder_double(
// IR: call double @fmod(double noundef
// IR-LABEL: define dso_local double @nearby_long_double(
// IR: call double @nearbyintl(double noundef

// ASM-LABEL: float_arithmetic:
// ASM: STTU
// ASM: LDSF
// ASM: FADD
// ASM: STSF
// ASM: LDSF
// ASM: FMUL
// ASM: STSF
// ASM: LDTU r231
// ASM-LABEL: double_arithmetic:
// ASM: FSUB
// ASM: FDIV
// ASM-LABEL: long_double_arithmetic:
// ASM: FADD
// ASM-LABEL: compare_float:
// ASM: FUN
// ASM: FCMP
// ASM-LABEL: compare_double:
// ASM: FEQL
// ASM-LABEL: widen_float:
// ASM: STTU
// ASM: LDSF r231
// ASM-LABEL: narrow_double:
// ASM: STSF
// ASM: LDTU r231
// ASM-LABEL: signed_to_double:
// ASM: FLOT r231, 4, r231
// ASM-LABEL: double_to_signed:
// ASM: FIXU r231, 1, r231
// ASM-LABEL: fused_float:
// ASM: SETH {{r[0-9]+}}, (fmaf>>48)&65535
// ASM: PUSHGO r31, {{r[0-9]+}}, 0
// ASM-LABEL: remainder_double:
// ASM: SETH {{r[0-9]+}}, (fmod>>48)&65535
// ASM: PUSHGO r31, {{r[0-9]+}}, 0
// ASM-LABEL: nearby_long_double:
// ASM: SETH {{r[0-9]+}}, (nearbyintl>>48)&65535
// ASM: PUSHGO r31, {{r[0-9]+}}, 0

// ELF: Format: elf64-mmix
// ELF-NEXT: Arch: mmix
// ELF: Type: Relocatable
// ELF: Machine: EM_MMIX
// ELF: Type: R_MMIX_PUSHJ_STUBBABLE (36)
// ELF-NEXT: Symbol: fmaf
// ELF: Type: R_MMIX_PUSHJ_STUBBABLE (36)
// ELF-NEXT: Symbol: fmod
// ELF: Type: R_MMIX_PUSHJ_STUBBABLE (36)
// ELF-NEXT: Symbol: nearbyintl
// ELF: Name: float_arithmetic
// ELF: Type: Function
// ELF: Name: double_arithmetic
// ELF: Type: Function
// ELF: Name: long_double_arithmetic
// ELF: Type: Function
// ELF: Name: fmaf
// ELF: Section: Undefined
// ELF: Name: fmod
// ELF: Section: Undefined
// ELF: Name: nearbyintl
// ELF: Section: Undefined

// DIS-LABEL: <float_arithmetic>:
// DIS: FADD
// DIS: FMUL
// DIS: STSF
// DIS: LDTU r231
// DIS-LABEL: <double_arithmetic>:
// DIS: FSUB
// DIS: FDIV
// DIS-LABEL: <long_double_arithmetic>:
// DIS: FADD
// DIS-LABEL: <compare_float>:
// DIS: FUN
// DIS: FCMP
// DIS-LABEL: <compare_double>:
// DIS: FEQL
// DIS-LABEL: <widen_float>:
// DIS: LDSF r231
// DIS-LABEL: <narrow_double>:
// DIS: STSF
// DIS: LDTU r231
// DIS-LABEL: <signed_to_double>:
// DIS: FLOT r231, 4, r231
// DIS-LABEL: <double_to_signed>:
// DIS: FIXU r231, 1, r231
// DIS-LABEL: <fused_float>:
// DIS: PUSHJ r31, 0
// DIS-NEXT: {{.*}} R_MMIX_PUSHJ_STUBBABLE fmaf
// DIS-LABEL: <remainder_double>:
// DIS: PUSHJ r31, 0
// DIS-NEXT: {{.*}} R_MMIX_PUSHJ_STUBBABLE fmod
// DIS-LABEL: <nearby_long_double>:
// DIS: PUSHJ r31, 0
// DIS-NEXT: {{.*}} R_MMIX_PUSHJ_STUBBABLE nearbyintl
