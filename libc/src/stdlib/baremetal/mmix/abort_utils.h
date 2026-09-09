//===-- MMIX abort utilities -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_STDLIB_BAREMETAL_MMIX_ABORT_UTILS_H
#define LLVM_LIBC_SRC_STDLIB_BAREMETAL_MMIX_ABORT_UTILS_H

#include "src/__support/common.h"
#include "src/stdlib/abort.h"

namespace LIBC_NAMESPACE_DECL {
namespace abort_utils {

// Internal failures must use the same SIGABRT policy as public abort().
[[noreturn]] LIBC_INLINE void abort() { LIBC_NAMESPACE::abort(); }

} // namespace abort_utils
} // namespace LIBC_NAMESPACE_DECL

#endif
