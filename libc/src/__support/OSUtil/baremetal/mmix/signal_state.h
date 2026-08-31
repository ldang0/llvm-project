//===-- MMIX synchronous signal state --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_SIGNAL_STATE_H
#define LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_SIGNAL_STATE_H

#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {
namespace internal::mmix {

using SignalHandler = void (*)(int);

bool is_supported_signal(int signal);
SignalHandler set_signal_handler(int signal, SignalHandler handler);
int dispatch_signal(int signal);

} // namespace internal::mmix
} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_SIGNAL_STATE_H
