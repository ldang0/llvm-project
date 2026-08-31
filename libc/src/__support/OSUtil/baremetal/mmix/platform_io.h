//===-- MMIX QEMU platform I/O adapter --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_PLATFORM_IO_H
#define LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_PLATFORM_IO_H

#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {
namespace internal {
namespace mmix {

enum class PlatformFileMode : unsigned {
  TEXT_READ = 0,
  TEXT_WRITE = 1,
  BINARY_READ = 2,
  BINARY_WRITE = 3,
  BINARY_READ_WRITE = 4,
};

enum class PlatformStreamOrientation : unsigned {
  UNORIENTED = 0,
  BYTE = 1,
  WIDE = 2,
};

inline constexpr __UINT64_TYPE__ PLATFORM_IO_MAX_TRANSFER = 1024 * 1024;
inline constexpr unsigned PLATFORM_FILE_CAPACITY = 8;

} // namespace mmix
} // namespace internal

struct __llvm_libc_stdio_cookie {
  unsigned handle;
  internal::mmix::PlatformFileMode mode;
  bool open;
  bool eof;
  bool error;
  internal::mmix::PlatformStreamOrientation orientation;
  unsigned pushback_count;
  unsigned char pushback[4];
};

extern "C" __llvm_libc_stdio_cookie __llvm_libc_stdin_cookie;
extern "C" __llvm_libc_stdio_cookie __llvm_libc_stdout_cookie;
extern "C" __llvm_libc_stdio_cookie __llvm_libc_stderr_cookie;

extern "C" __llvm_libc_stdio_cookie *
__llvm_libc_mmix_stream_open(const char *path, unsigned mode);
extern "C" int __llvm_libc_mmix_stream_close(__llvm_libc_stdio_cookie *stream);
extern "C" int __llvm_libc_mmix_stream_seek(__llvm_libc_stdio_cookie *stream,
                                            __INT64_TYPE__ offset);
extern "C" __INT64_TYPE__
__llvm_libc_mmix_stream_tell(__llvm_libc_stdio_cookie *stream);
extern "C" int __llvm_libc_mmix_stream_flush(__llvm_libc_stdio_cookie *stream);
extern "C" int
__llvm_libc_mmix_stream_eof(const __llvm_libc_stdio_cookie *stream);
extern "C" int
__llvm_libc_mmix_stream_error(const __llvm_libc_stdio_cookie *stream);
extern "C" void
__llvm_libc_mmix_stream_clearerr(__llvm_libc_stdio_cookie *stream);
extern "C" int __llvm_libc_mmix_stream_ungetc(int c,
                                              __llvm_libc_stdio_cookie *stream);
extern "C" int
__llvm_libc_mmix_stream_pushback(const unsigned char *bytes, unsigned count,
                                 __llvm_libc_stdio_cookie *stream);
extern "C" int __llvm_libc_mmix_stream_orient(__llvm_libc_stdio_cookie *stream,
                                              int mode);
extern "C" void
__llvm_libc_mmix_stream_set_error(__llvm_libc_stdio_cookie *stream);
extern "C" int __llvm_libc_mmix_stream_flush_all();
extern "C" void __llvm_libc_mmix_stream_teardown();

extern "C" int __llvm_libc_mmix_file_open(const char *path, unsigned mode);
extern "C" int __llvm_libc_mmix_file_close(int handle);
extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_read(int handle, void *buffer,
                                                     __UINT64_TYPE__ size);
extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_write(int handle,
                                                      const void *buffer,
                                                      __UINT64_TYPE__ size);
extern "C" int __llvm_libc_mmix_file_seek(int handle, __INT64_TYPE__ offset);
extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_tell(int handle);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_PLATFORM_IO_H
