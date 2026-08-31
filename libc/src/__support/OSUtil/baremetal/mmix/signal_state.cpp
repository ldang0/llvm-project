//===-- MMIX synchronous signal state ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/OSUtil/baremetal/mmix/signal_state.h"
#include "hdr/signal_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"

namespace LIBC_NAMESPACE_DECL {
namespace internal::mmix {
namespace {

constexpr int SIGNAL_TABLE_SIZE = SIGTERM + 1;
SignalHandler handlers[SIGNAL_TABLE_SIZE];
bool active[SIGNAL_TABLE_SIZE];

} // namespace

bool is_supported_signal(int signal) {
  switch (signal) {
  case SIGABRT:
  case SIGFPE:
  case SIGILL:
  case SIGINT:
  case SIGSEGV:
  case SIGTERM:
    return true;
  default:
    return false;
  }
}

SignalHandler set_signal_handler(int signal, SignalHandler handler) {
  SignalHandler previous = handlers[signal];
  handlers[signal] = handler;
  return previous;
}

int dispatch_signal(int signal) {
  SignalHandler handler = handlers[signal];
  if (handler == SIG_IGN)
    return 0;
  if (handler == SIG_DFL)
    Semihosting::halt(128 + static_cast<unsigned>(signal));

  // Recursive synchronous delivery has no queue or masking service.
  if (active[signal])
    return -1;
  active[signal] = true;
  handler(signal);
  active[signal] = false;
  return 0;
}

} // namespace internal::mmix
} // namespace LIBC_NAMESPACE_DECL
