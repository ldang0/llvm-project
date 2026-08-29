//===-- MMIX bare-metal abort --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdlib/abort.h"
#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(void, abort, ()) { internal::mmix::Semihosting::halt(134); }

} // namespace LIBC_NAMESPACE_DECL
