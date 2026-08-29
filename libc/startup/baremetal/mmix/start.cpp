//===-- MMIX bare-metal process entry -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"
#include "src/__support/common.h"
#include "src/stdlib/atexit.h"
#include "src/stdlib/exit.h"

extern "C" int main(int argc, char **argv, char **envp);
extern "C" {
extern char _end[];
extern char __llvm_libc_heap_limit[];
extern char __data_source[];
extern char __data_start[];
extern char __data_size[];
extern char __bss_start[];
extern char __bss_size[];
extern __UINTPTR_TYPE__ __preinit_array_start[];
extern __UINTPTR_TYPE__ __preinit_array_end[];
extern __UINTPTR_TYPE__ __init_array_start[];
extern __UINTPTR_TYPE__ __init_array_end[];
extern __UINTPTR_TYPE__ __fini_array_start[];
extern __UINTPTR_TYPE__ __fini_array_end[];

void __llvm_libc_stdio_teardown();
}

namespace {

constexpr unsigned MALFORMED_ARGUMENT_STATUS = 127;
using InitCallback = void();

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

void initialize_memory() {
  __UINTPTR_TYPE__ data_size = reinterpret_cast<__UINTPTR_TYPE__>(__data_size);
  for (__UINTPTR_TYPE__ index = 0; index < data_size; ++index)
    __data_start[index] = __data_source[index];

  __UINTPTR_TYPE__ bss_size = reinterpret_cast<__UINTPTR_TYPE__>(__bss_size);
  for (__UINTPTR_TYPE__ index = 0; index < bss_size; ++index)
    __bss_start[index] = 0;
}

void call_forward(__UINTPTR_TYPE__ *begin, __UINTPTR_TYPE__ *end) {
  for (; begin != end; ++begin)
    reinterpret_cast<InitCallback *>(*begin)();
}

void call_fini_array() {
  for (__UINTPTR_TYPE__ *entry = __fini_array_end; entry != __fini_array_start;)
    reinterpret_cast<InitCallback *>(*--entry)();
}

void terminate_streams() { __llvm_libc_stdio_teardown(); }

[[noreturn]] void startup_failure() {
  LIBC_NAMESPACE::internal::mmix::Semihosting::halt(MALFORMED_ARGUMENT_STATUS);
}

} // namespace

extern "C" [[noreturn, gnu::visibility("hidden")]] void
__llvm_libc_mmix_start_main(__UINT64_TYPE__ argc, char **argv) {
  if (!valid_arguments(argc, argv))
    startup_failure();

  initialize_memory();
  call_forward(__preinit_array_start, __preinit_array_end);
  call_forward(__init_array_start, __init_array_end);

  // LIFO registration runs user callbacks before stream and fini teardown.
  if (LIBC_NAMESPACE::atexit(&call_fini_array) != 0 ||
      LIBC_NAMESPACE::atexit(&terminate_streams) != 0)
    startup_failure();
  LIBC_NAMESPACE::exit(main(static_cast<int>(argc), argv, nullptr));
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
