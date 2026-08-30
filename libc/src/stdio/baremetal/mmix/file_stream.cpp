//===-- MMIX bare-metal file streams -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fclose.h"
#include "src/stdio/fopen.h"
#include "src/stdio/fseek.h"
#include "src/stdio/ftell.h"
#include "src/stdio/rewind.h"

#include "hdr/errno_macros.h"
#include "hdr/stdio_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {
namespace {

__llvm_libc_stdio_cookie *cookie(::FILE *stream) {
  return reinterpret_cast<__llvm_libc_stdio_cookie *>(stream);
}

bool equal(const char *left, const char *right) {
  if (left == nullptr || right == nullptr)
    return false;
  while (*left != '\0' && *left == *right) {
    ++left;
    ++right;
  }
  return *left == *right;
}

bool parse_mode(const char *mode, unsigned &platform_mode) {
  using internal::mmix::PlatformFileMode;
  if (equal(mode, "r") || equal(mode, "rt"))
    platform_mode = static_cast<unsigned>(PlatformFileMode::TEXT_READ);
  else if (equal(mode, "rb"))
    platform_mode = static_cast<unsigned>(PlatformFileMode::BINARY_READ);
  else if (equal(mode, "w") || equal(mode, "wt"))
    platform_mode = static_cast<unsigned>(PlatformFileMode::TEXT_WRITE);
  else if (equal(mode, "wb"))
    platform_mode = static_cast<unsigned>(PlatformFileMode::BINARY_WRITE);
  else if (equal(mode, "w+") || equal(mode, "w+t") || equal(mode, "wt+") ||
           equal(mode, "w+b") || equal(mode, "wb+"))
    platform_mode = static_cast<unsigned>(PlatformFileMode::BINARY_READ_WRITE);
  else
    return false;
  return true;
}

} // namespace

LLVM_LIBC_FUNCTION(::FILE *, fopen,
                   (const char *__restrict path, const char *__restrict mode)) {
  unsigned platform_mode;
  if (!parse_mode(mode, platform_mode)) {
    libc_errno = EINVAL;
    return nullptr;
  }
  return reinterpret_cast<::FILE *>(
      __llvm_libc_mmix_stream_open(path, platform_mode));
}

LLVM_LIBC_FUNCTION(int, fclose, (::FILE * stream)) {
  return __llvm_libc_mmix_stream_close(cookie(stream)) == 0 ? 0 : EOF;
}

LLVM_LIBC_FUNCTION(int, fseek, (::FILE * stream, long offset, int whence)) {
  __INT64_TYPE__ absolute = offset;
  if (whence == SEEK_CUR) {
    __INT64_TYPE__ current = __llvm_libc_mmix_stream_tell(cookie(stream));
    if (current < 0 || __builtin_add_overflow(current, offset, &absolute)) {
      if (current >= 0)
        libc_errno = EINVAL;
      return -1;
    }
  } else if (whence != SEEK_SET) {
    libc_errno = EINVAL;
    return -1;
  }
  return __llvm_libc_mmix_stream_seek(cookie(stream), absolute);
}

LLVM_LIBC_FUNCTION(long, ftell, (::FILE * stream)) {
  return static_cast<long>(__llvm_libc_mmix_stream_tell(cookie(stream)));
}

LLVM_LIBC_FUNCTION(void, rewind, (::FILE * stream)) {
  __llvm_libc_mmix_stream_seek(cookie(stream), 0);
  __llvm_libc_mmix_stream_clearerr(cookie(stream));
}

} // namespace LIBC_NAMESPACE_DECL
