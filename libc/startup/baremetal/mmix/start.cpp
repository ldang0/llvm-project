//===-- MMIX bare-metal process entry -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"
#include "src/__support/common.h"

extern "C" int main(int argc, char **argv, char **envp);
extern "C" char _end[];
extern "C" char __llvm_libc_heap_limit[];

namespace {

constexpr unsigned MALFORMED_ARGUMENT_STATUS = 127;

bool valid_arguments(__UINT64_TYPE__ argc, char **argv) {
  if (argc > 0x7fffffff || argv == nullptr)
    return false;

  __UINT64_TYPE__ address =
      static_cast<__UINT64_TYPE__>(reinterpret_cast<__UINTPTR_TYPE__>(argv));
  if ((address & (alignof(char *) - 1)) != 0)
    return false;

  __UINT64_TYPE__ entries = argc + 1;
  __UINT64_TYPE__ pool_begin =
      static_cast<__UINT64_TYPE__>(reinterpret_cast<__UINTPTR_TYPE__>(_end));
  __UINT64_TYPE__ pool_end = static_cast<__UINT64_TYPE__>(
      reinterpret_cast<__UINTPTR_TYPE__>(__llvm_libc_heap_limit));
  if (address < pool_begin || address >= pool_end ||
      entries > (pool_end - address) / sizeof(char *))
    return false;

  for (__UINT64_TYPE__ index = 0; index < argc; ++index)
    if (argv[index] == nullptr)
      return false;
  return argv[argc] == nullptr;
}

} // namespace

extern "C" [[noreturn, gnu::visibility("hidden")]] void
__llvm_libc_mmix_start_main(__UINT64_TYPE__ argc, char **argv) {
  using LIBC_NAMESPACE::internal::mmix::Semihosting;

  if (!valid_arguments(argc, argv))
    Semihosting::halt(MALFORMED_ARGUMENT_STATUS);
  Semihosting::halt(
      static_cast<unsigned>(main(static_cast<int>(argc), argv, nullptr)));
}

// QEMU supplies argc and argv in local registers $0 and $1 and initializes
// rO to the bottom of a 32-KiB register-stack slot. Establish the disjoint
// descending software stack at the top of that slot, then enter the ordinary
// GNU C ABI through global argument registers $231 and $232.
asm(R"(
  .section .text.init.enter,"ax",@progbits
  .globl _start
  .type _start,@function
_start:
  SETL r255,231
  PUT rG,r255
  GET r254,rO
  SETL r255,0x8000
  ADDU r254,r254,r255
  OR r231,r0,0
  OR r232,r1,0
  PUSHJ r31,__llvm_libc_mmix_start_main
  SETL r255,127
  TRAP 0,0,0
  .size _start,.-_start
)");
