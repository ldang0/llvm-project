//===-- MMIX bare-metal process utilities ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdlib/at_quick_exit.h"
#include "src/stdlib/getenv.h"
#include "src/stdlib/quick_exit.h"
#include "src/stdlib/system.h"

#include "hdr/types/atexithandler_t.h"
#include "src/__support/OSUtil/exit.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {
namespace {

constexpr __SIZE_TYPE__ QUICK_EXIT_CALLBACK_CAPACITY = 64;
__atexithandler_t quick_exit_callbacks[QUICK_EXIT_CALLBACK_CAPACITY];
__SIZE_TYPE__ quick_exit_callback_count;

} // namespace

LLVM_LIBC_FUNCTION(int, at_quick_exit, (__atexithandler_t callback)) {
  if (callback == nullptr ||
      quick_exit_callback_count == QUICK_EXIT_CALLBACK_CAPACITY)
    return -1;
  quick_exit_callbacks[quick_exit_callback_count++] = callback;
  return 0;
}

[[noreturn]] LLVM_LIBC_FUNCTION(void, quick_exit, (int status)) {
  while (quick_exit_callback_count != 0)
    quick_exit_callbacks[--quick_exit_callback_count]();
  internal::exit(status);
}

LLVM_LIBC_FUNCTION(char *, getenv, (const char *)) { return nullptr; }

LLVM_LIBC_FUNCTION(int, system, (const char *command)) {
  if (command == nullptr)
    return 0;
  return -1;
}

} // namespace LIBC_NAMESPACE_DECL
