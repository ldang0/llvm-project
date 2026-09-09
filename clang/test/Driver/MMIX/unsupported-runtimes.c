// MMIX has no profile, coverage, or general unwind runtime contract. The
// Driver must reject enabling those producers before an object is created.

// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fprofile-instr-generate -c %s -o %t.profile.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PROFILE
// RUN: not test -e %t.profile.o
// RUN: not %clang --target=mmix-unknown-elf -ffreestanding --coverage \
// RUN:   -c %s -o %t.coverage.o 2>&1 | FileCheck %s --check-prefix=COVERAGE
// RUN: not test -e %t.coverage.o
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -funwind-tables -c %s -o %t.unwind.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=UNWIND
// RUN: not test -e %t.unwind.o
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fsanitize=address -c %s -o %t.asan.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ASAN
// RUN: not test -e %t.asan.o
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fxray-instrument -c %s -o %t.xray.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=XRAY
// RUN: not test -e %t.xray.o

// Disabling unsupported producers remains accepted.
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fno-profile-instr-generate -fno-profile-arcs -fno-test-coverage \
// RUN:   -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-exceptions \
// RUN:   -c %s -o %t.disabled.o
// RUN: test -s %t.disabled.o

// PROFILE: error: unsupported option '-fprofile-instr-generate' for target 'mmix-unknown-unknown'
// COVERAGE: error: unsupported option '--coverage' for target 'mmix-unknown-unknown-elf'
// UNWIND: error: unsupported option '-funwind-tables' for target 'mmix-unknown-unknown'
// ASAN: error: unsupported option '-fsanitize=address' for target 'mmix-unknown-unknown'
// XRAY: error: unsupported option '-fxray-instrument' for target 'mmix-unknown-unknown'

int runtime_free_function(int value) {
  return value + 1;
}
