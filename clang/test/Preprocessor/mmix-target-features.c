// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -target-cpu generic \
// RUN:   -mrelocation-model static -emit-llvm %s -o - | \
// RUN:   FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple mmix-unknown-unknown -target-cpu generic \
// RUN:   -tune-cpu generic -target-feature -system -target-feature -cache \
// RUN:   -target-feature -virtual-memory -mrelocation-model static \
// RUN:   -emit-llvm %s -o - | FileCheck %s --check-prefix=OPTIONAL-DISABLED
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -target-feature -base \
// RUN:   -mrelocation-model static -emit-llvm %s -o /dev/null 2>&1 | \
// RUN:   FileCheck %s --check-prefix=NO-BASE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -target-feature +unknown \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=UNKNOWN-FEATURE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -target-cpu unknown \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=UNKNOWN-CPU
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -tune-cpu unknown \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=UNKNOWN-TUNE

void feature_probe(void) {}

// DEFAULT: attributes #[[ATTR:[0-9]+]] = {{.*}}"target-cpu"="generic"
// DEFAULT-SAME: "target-features"="+base,+cache,+system,+virtual-memory"
// OPTIONAL-DISABLED: attributes #[[ATTR:[0-9]+]] =
// OPTIONAL-DISABLED-SAME: "target-cpu"="generic"
// OPTIONAL-DISABLED-SAME: "target-features"="+base,-cache,-system,-virtual-memory"
// NO-BASE: error: invalid feature combination: MMIX C requires the base feature
// UNKNOWN-FEATURE: unknown MMIX target feature 'unknown'
// UNKNOWN-CPU: error: unknown target CPU 'unknown'
// UNKNOWN-CPU-NEXT: note: valid target CPU values are: generic
// UNKNOWN-TUNE: error: unknown target CPU 'unknown'
// UNKNOWN-TUNE-NEXT: note: valid target CPU values are: generic
