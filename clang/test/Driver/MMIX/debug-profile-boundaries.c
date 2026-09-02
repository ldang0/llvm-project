// REQUIRES: mmix-registered-target
// RUN: not %clang --target=mmix -g -gsplit-dwarf -ffreestanding -c %s \
// RUN:   -o %t.split.o 2>&1 | FileCheck %s --check-prefix=SPLIT
// RUN: not test -e %t.split.o
// RUN: not test -e %t.split.dwo
// RUN: %clang --target=mmix -g -gno-split-dwarf -ffreestanding -c %s \
// RUN:   -o %t.ordinary.o
// RUN: llvm-readobj --sections %t.ordinary.o \
// RUN:   | FileCheck %s --check-prefix=ORDINARY

// SPLIT: error: unsupported option '-gsplit-dwarf' for target 'mmix'
// ORDINARY: Name: .debug_info

int debug_boundary(void) { return 0; }
