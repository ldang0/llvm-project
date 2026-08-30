//===-- MMIX bare-metal ungetc -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/ungetc.h"

#include "hdr/stdio_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, ungetc, (int c, ::FILE *stream)) {
  int result = __llvm_libc_mmix_stream_ungetc(
      c, reinterpret_cast<__llvm_libc_stdio_cookie *>(stream));
  return result < 0 ? EOF : result;
}

} // namespace LIBC_NAMESPACE_DECL
