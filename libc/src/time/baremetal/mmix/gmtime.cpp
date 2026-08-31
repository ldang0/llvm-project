//===-- MMIX bare-metal gmtime ---------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/time/gmtime.h"

#include "hdr/types/struct_tm.h"
#include "hdr/types/time_t.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"
#include "src/time/time_utils.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(struct tm *, gmtime, (const time_t *timer)) {
  if (timer == nullptr)
    return nullptr;

  // Explicit aggregate initialization avoids a guard variable while the MMIX
  // C++ producer profile does not support dynamic local initialization.
  static struct tm result{};
  auto converted = time_utils::gmtime_internal(timer, &result);
  if (!converted) {
    libc_errno = converted.error();
    return nullptr;
  }
  return converted.value();
}

} // namespace LIBC_NAMESPACE_DECL
