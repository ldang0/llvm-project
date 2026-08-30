//===-- MMIX bare-metal block I/O ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fread.h"
#include "src/stdio/fwrite.h"

#include "hdr/errno_macros.h"
#include "src/__support/OSUtil/io.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(size_t, fread,
                   (void *__restrict buffer, size_t size, size_t count,
                    ::FILE *stream)) {
  if (size == 0 || count == 0)
    return 0;
  size_t bytes;
  if (__builtin_mul_overflow(size, count, &bytes)) {
    libc_errno = EINVAL;
    return 0;
  }
  ssize_t result =
      __llvm_libc_stdio_read(stream, static_cast<char *>(buffer), bytes);
  return result < 0 ? 0 : static_cast<size_t>(result) / size;
}

LLVM_LIBC_FUNCTION(size_t, fwrite,
                   (const void *__restrict buffer, size_t size, size_t count,
                    ::FILE *stream)) {
  if (size == 0 || count == 0)
    return 0;
  size_t bytes;
  if (__builtin_mul_overflow(size, count, &bytes)) {
    libc_errno = EINVAL;
    return 0;
  }
  ssize_t result =
      __llvm_libc_stdio_write(stream, static_cast<const char *>(buffer), bytes);
  return result < 0 ? 0 : static_cast<size_t>(result) / size;
}

} // namespace LIBC_NAMESPACE_DECL
