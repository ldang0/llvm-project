//===-- MMIX implementation of raise ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/signal/raise.h"
#include "hdr/errno_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/signal_state.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, raise, (int signal_number)) {
  if (!internal::mmix::is_supported_signal(signal_number)) {
    libc_errno = EINVAL;
    return -1;
  }
  return internal::mmix::dispatch_signal(signal_number);
}

} // namespace LIBC_NAMESPACE_DECL
