//===-- MMIX bare-metal formatted output ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fprintf.h"
#include "src/stdio/printf.h"
#include "src/stdio/vfprintf.h"
#include "src/stdio/vprintf.h"

#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/arg_list.h"
#include "src/__support/common.h"
#include "src/stdio/baremetal/vfprintf_internal.h"

#include <stdarg.h>

namespace LIBC_NAMESPACE_DECL {

static ::FILE *standard_output() {
  return reinterpret_cast<::FILE *>(&__llvm_libc_stdout_cookie);
}

LLVM_LIBC_FUNCTION(int, vfprintf,
                   (::FILE *__restrict stream, const char *__restrict format,
                    va_list vlist)) {
  internal::ArgList args(vlist);
  return vfprintf_internal(stream, format, args);
}

LLVM_LIBC_FUNCTION(int, fprintf,
                   (::FILE *__restrict stream, const char *__restrict format,
                    ...)) {
  va_list vlist;
  va_start(vlist, format);
  internal::ArgList args(vlist);
  va_end(vlist);
  return vfprintf_internal(stream, format, args);
}

LLVM_LIBC_FUNCTION(int, vprintf,
                   (const char *__restrict format, va_list vlist)) {
  internal::ArgList args(vlist);
  return vfprintf_internal(standard_output(), format, args);
}

LLVM_LIBC_FUNCTION(int, printf, (const char *__restrict format, ...)) {
  va_list vlist;
  va_start(vlist, format);
  internal::ArgList args(vlist);
  va_end(vlist);
  return vfprintf_internal(standard_output(), format, args);
}

} // namespace LIBC_NAMESPACE_DECL
