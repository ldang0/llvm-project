//===-- MMIX bare-metal formatted input -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fscanf.h"
#include "src/stdio/scanf.h"
#include "src/stdio/vfscanf.h"
#include "src/stdio/vscanf.h"

#include "hdr/stdio_macros.h"
#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/arg_list.h"
#include "src/__support/common.h"
#include "src/stdio/scanf_core/reader.h"
#include "src/stdio/scanf_core/scanf_main.h"

#include <stdarg.h>

namespace LIBC_NAMESPACE_DECL {

static ::FILE *standard_input() {
  return reinterpret_cast<::FILE *>(&__llvm_libc_stdin_cookie);
}

class StreamReader : public scanf_core::Reader<StreamReader> {
  __llvm_libc_stdio_cookie *stream;
  bool input_failed = false;

public:
  LIBC_INLINE explicit StreamReader(::FILE *file)
      : stream(reinterpret_cast<__llvm_libc_stdio_cookie *>(file)) {}

  LIBC_INLINE char getc() {
    char value;
    if (__llvm_libc_stdio_read(stream, &value, 1) == 1)
      return value;
    input_failed = true;
    return '\0';
  }

  LIBC_INLINE void ungetc(int value) {
    if (input_failed && value == '\0')
      return;
    (void)__llvm_libc_mmix_stream_ungetc(value, stream);
  }

  LIBC_INLINE bool failed() const {
    return input_failed || __llvm_libc_mmix_stream_eof(stream) ||
           __llvm_libc_mmix_stream_error(stream);
  }
};

static int scan_stream(::FILE *stream, const char *format,
                       internal::ArgList &args) {
  StreamReader reader(stream);
  int result = scanf_core::scanf_main(&reader, format, args);
  return result == 0 && reader.failed() ? EOF : result;
}

LLVM_LIBC_FUNCTION(int, vfscanf,
                   (::FILE *__restrict stream, const char *__restrict format,
                    va_list vlist)) {
  internal::ArgList args(vlist);
  return scan_stream(stream, format, args);
}

LLVM_LIBC_FUNCTION(int, fscanf,
                   (::FILE *__restrict stream, const char *__restrict format,
                    ...)) {
  va_list vlist;
  va_start(vlist, format);
  internal::ArgList args(vlist);
  va_end(vlist);
  return scan_stream(stream, format, args);
}

LLVM_LIBC_FUNCTION(int, vscanf,
                   (const char *__restrict format, va_list vlist)) {
  internal::ArgList args(vlist);
  return scan_stream(standard_input(), format, args);
}

LLVM_LIBC_FUNCTION(int, scanf, (const char *__restrict format, ...)) {
  va_list vlist;
  va_start(vlist, format);
  internal::ArgList args(vlist);
  va_end(vlist);
  return scan_stream(standard_input(), format, args);
}

} // namespace LIBC_NAMESPACE_DECL
