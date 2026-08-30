//===-- MMIX bare-metal stream buffering ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/setbuf.h"
#include "src/stdio/setvbuf.h"

#include "hdr/errno_macros.h"
#include "hdr/stdio_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, setvbuf,
                   (::FILE *__restrict stream, char *__restrict, int mode,
                    size_t)) {
  if (mode != _IONBF) {
    libc_errno = EINVAL;
    return EINVAL;
  }
  return __llvm_libc_mmix_stream_flush(
      reinterpret_cast<__llvm_libc_stdio_cookie *>(stream));
}

LLVM_LIBC_FUNCTION(void, setbuf,
                   (::FILE *__restrict stream, char *__restrict buffer)) {
  if (buffer != nullptr) {
    libc_errno = EINVAL;
    return;
  }
  setvbuf(stream, nullptr, _IONBF, 0);
}

} // namespace LIBC_NAMESPACE_DECL
