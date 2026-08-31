//===-- MMIX implementation of signal -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/signal/signal.h"
#include "hdr/errno_macros.h"
#include "hdr/signal_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/signal_state.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

using SignalHandler = void (*)(int);

LLVM_LIBC_FUNCTION(SignalHandler, signal,
                   (int signal_number, SignalHandler handler)) {
  if (!internal::mmix::is_supported_signal(signal_number) ||
      handler == SIG_ERR) {
    libc_errno = EINVAL;
    return SIG_ERR;
  }
  return internal::mmix::set_signal_handler(signal_number, handler);
}

} // namespace LIBC_NAMESPACE_DECL
