//===-- MMIX bare-metal stream status ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/clearerr.h"
#include "src/stdio/feof.h"
#include "src/stdio/ferror.h"
#include "src/stdio/fflush.h"

#include "hdr/stdio_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {
namespace {

__llvm_libc_stdio_cookie *cookie(::FILE *stream) {
  return reinterpret_cast<__llvm_libc_stdio_cookie *>(stream);
}

} // namespace

LLVM_LIBC_FUNCTION(int, fflush, (::FILE * stream)) {
  int result = stream == nullptr
                   ? __llvm_libc_mmix_stream_flush_all()
                   : __llvm_libc_mmix_stream_flush(cookie(stream));
  return result == 0 ? 0 : EOF;
}

LLVM_LIBC_FUNCTION(int, feof, (::FILE * stream)) {
  return __llvm_libc_mmix_stream_eof(cookie(stream));
}

LLVM_LIBC_FUNCTION(int, ferror, (::FILE * stream)) {
  return __llvm_libc_mmix_stream_error(cookie(stream));
}

LLVM_LIBC_FUNCTION(void, clearerr, (::FILE * stream)) {
  __llvm_libc_mmix_stream_clearerr(cookie(stream));
}

} // namespace LIBC_NAMESPACE_DECL
