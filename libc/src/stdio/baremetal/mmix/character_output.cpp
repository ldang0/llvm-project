//===-- MMIX bare-metal character output ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fputc.h"
#include "src/stdio/getchar.h"
#include "src/stdio/putc.h"
#include "src/stdio/putchar.h"
#include "src/stdio/puts.h"

#include "hdr/stdio_macros.h"
#include "src/__support/OSUtil/baremetal/io.h"
#include "src/__support/OSUtil/baremetal/mmix/platform_io.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {
namespace {

int write_character(int c, ::FILE *stream) {
  unsigned char value = static_cast<unsigned char>(c);
  ssize_t result = __llvm_libc_stdio_write(
      stream, reinterpret_cast<const char *>(&value), 1);
  return result == 1 ? static_cast<int>(value) : EOF;
}

} // namespace

LLVM_LIBC_FUNCTION(int, fputc, (int c, ::FILE *stream)) {
  return write_character(c, stream);
}

LLVM_LIBC_FUNCTION(int, putc, (int c, ::FILE *stream)) {
  return write_character(c, stream);
}

LLVM_LIBC_FUNCTION(int, getchar, ()) {
  unsigned char value;
  ssize_t result = __llvm_libc_stdio_read(&__llvm_libc_stdin_cookie,
                                          reinterpret_cast<char *>(&value), 1);
  return result == 1 ? static_cast<int>(value) : EOF;
}

LLVM_LIBC_FUNCTION(int, putchar, (int c)) {
  return write_character(
      c, reinterpret_cast<::FILE *>(&__llvm_libc_stdout_cookie));
}

LLVM_LIBC_FUNCTION(int, puts, (const char *__restrict string)) {
  size_t length = 0;
  while (string[length] != '\0')
    ++length;
  if (__llvm_libc_stdio_write(&__llvm_libc_stdout_cookie, string, length) !=
          static_cast<ssize_t>(length) ||
      __llvm_libc_stdio_write(&__llvm_libc_stdout_cookie, "\n", 1) != 1)
    return EOF;
  return 0;
}

} // namespace LIBC_NAMESPACE_DECL
