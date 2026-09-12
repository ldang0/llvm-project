# REQUIRES: asserts
# RUN: llvm-mc -triple=mmix -debug-only=asm-matcher %s 2>&1 | FileCheck %s

# Check the generated matcher's instruction-name lookup in assertions builds.
# CHECK: Trying to match opcode ADD{{$}}
ADD r1, r2, r3
