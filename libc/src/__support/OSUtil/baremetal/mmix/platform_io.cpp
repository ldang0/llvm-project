//===-- MMIX QEMU platform I/O adapter -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "platform_io.h"

#include "hdr/errno_macros.h"
#include "include/llvm-libc-types/size_t.h"
#include "include/llvm-libc-types/ssize_t.h"
#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"

namespace LIBC_NAMESPACE_DECL {

extern "C" {
__llvm_libc_stdio_cookie __llvm_libc_stdin_cookie = {
    0, internal::mmix::PlatformFileMode::TEXT_READ, true, false, false};
__llvm_libc_stdio_cookie __llvm_libc_stdout_cookie = {
    1, internal::mmix::PlatformFileMode::TEXT_WRITE, true, false, false};
__llvm_libc_stdio_cookie __llvm_libc_stderr_cookie = {
    2, internal::mmix::PlatformFileMode::TEXT_WRITE, true, false, false};
}

namespace {

using internal::mmix::FileMode;
using internal::mmix::PlatformFileMode;
using internal::mmix::Semihosting;
using internal::mmix::SemihostingResult;
using internal::mmix::TransferResult;

constexpr unsigned FIRST_FILE_HANDLE = 3;
constexpr unsigned FILE_CAPACITY = internal::mmix::PLATFORM_FILE_CAPACITY;
constexpr unsigned FILE_HANDLE_LIMIT = FIRST_FILE_HANDLE + FILE_CAPACITY;
constexpr __UINT64_TYPE__ MAX_TRANSFER =
    internal::mmix::PLATFORM_IO_MAX_TRANSFER;
constexpr __UINT64_TYPE__ MAX_PATH_LENGTH = 255;

LIBC_CONSTINIT __llvm_libc_stdio_cookie file_slots[FILE_CAPACITY] = {};
LIBC_CONSTINIT int process_errno = 0;

LIBC_INLINE int fail(int error) {
  process_errno = error;
  return -1;
}

LIBC_INLINE __INT64_TYPE__ fail_io(int error) {
  process_errno = error;
  return -1;
}

LIBC_INLINE bool valid_mode(unsigned mode) {
  return mode <= static_cast<unsigned>(PlatformFileMode::BINARY_READ_WRITE);
}

LIBC_INLINE bool can_read(PlatformFileMode mode) {
  return mode == PlatformFileMode::TEXT_READ ||
         mode == PlatformFileMode::BINARY_READ ||
         mode == PlatformFileMode::BINARY_READ_WRITE;
}

LIBC_INLINE bool can_write(PlatformFileMode mode) {
  return mode == PlatformFileMode::TEXT_WRITE ||
         mode == PlatformFileMode::BINARY_WRITE ||
         mode == PlatformFileMode::BINARY_READ_WRITE;
}

LIBC_INLINE FileMode transport_mode(PlatformFileMode mode) {
  return static_cast<FileMode>(static_cast<unsigned>(mode));
}

LIBC_INLINE bool valid_path(const char *path) {
  if (path == nullptr || path[0] == '\0')
    return false;

  __UINT64_TYPE__ length = 0;
  while (path[length] != '\0') {
    if (length == MAX_PATH_LENGTH || path[length] == '/' ||
        path[length] == '\\')
      return false;
    ++length;
  }
  return !(length == 1 && path[0] == '.') &&
         !(length == 2 && path[0] == '.' && path[1] == '.');
}

LIBC_INLINE __llvm_libc_stdio_cookie *slot_for(int handle) {
  if (handle < static_cast<int>(FIRST_FILE_HANDLE) ||
      handle >= static_cast<int>(FILE_HANDLE_LIMIT))
    return nullptr;
  return &file_slots[static_cast<unsigned>(handle) - FIRST_FILE_HANDLE];
}

LIBC_INLINE __llvm_libc_stdio_cookie *stream_for(void *cookie) {
  if (cookie == &__llvm_libc_stdin_cookie)
    return &__llvm_libc_stdin_cookie;
  if (cookie == &__llvm_libc_stdout_cookie)
    return &__llvm_libc_stdout_cookie;
  if (cookie == &__llvm_libc_stderr_cookie)
    return &__llvm_libc_stderr_cookie;
  for (unsigned index = 0; index != FILE_CAPACITY; ++index)
    if (cookie == &file_slots[index])
      return &file_slots[index];
  return nullptr;
}

LIBC_INLINE const __llvm_libc_stdio_cookie *stream_for(const void *cookie) {
  return stream_for(const_cast<void *>(cookie));
}

LIBC_INLINE bool is_regular_stream(const __llvm_libc_stdio_cookie *stream) {
  return stream != nullptr && stream->handle >= FIRST_FILE_HANDLE &&
         stream->handle < FILE_HANDLE_LIMIT;
}

LIBC_INLINE __INT64_TYPE__ fail_stream_io(__llvm_libc_stdio_cookie *stream,
                                          int error) {
  if (stream != nullptr)
    stream->error = true;
  return fail_io(error);
}

// The MMIX TRAP handle is an immediate, so each admitted runtime handle must
// reach a separately instantiated transport operation.
SemihostingResult transport_open(unsigned handle, const char *path,
                                 FileMode mode) {
  switch (handle) {
  case 3:
    return Semihosting::open<3>(path, mode);
  case 4:
    return Semihosting::open<4>(path, mode);
  case 5:
    return Semihosting::open<5>(path, mode);
  case 6:
    return Semihosting::open<6>(path, mode);
  case 7:
    return Semihosting::open<7>(path, mode);
  case 8:
    return Semihosting::open<8>(path, mode);
  case 9:
    return Semihosting::open<9>(path, mode);
  case 10:
    return Semihosting::open<10>(path, mode);
  default:
    __builtin_unreachable();
  }
}

SemihostingResult transport_close(unsigned handle) {
  switch (handle) {
  case 3:
    return Semihosting::close<3>();
  case 4:
    return Semihosting::close<4>();
  case 5:
    return Semihosting::close<5>();
  case 6:
    return Semihosting::close<6>();
  case 7:
    return Semihosting::close<7>();
  case 8:
    return Semihosting::close<8>();
  case 9:
    return Semihosting::close<9>();
  case 10:
    return Semihosting::close<10>();
  default:
    __builtin_unreachable();
  }
}

TransferResult transport_read(unsigned handle, void *buffer,
                              __UINT64_TYPE__ size) {
  switch (handle) {
  case 3:
    return Semihosting::read<3>(buffer, size);
  case 4:
    return Semihosting::read<4>(buffer, size);
  case 5:
    return Semihosting::read<5>(buffer, size);
  case 6:
    return Semihosting::read<6>(buffer, size);
  case 7:
    return Semihosting::read<7>(buffer, size);
  case 8:
    return Semihosting::read<8>(buffer, size);
  case 9:
    return Semihosting::read<9>(buffer, size);
  case 10:
    return Semihosting::read<10>(buffer, size);
  default:
    __builtin_unreachable();
  }
}

TransferResult transport_write(unsigned handle, const void *buffer,
                               __UINT64_TYPE__ size) {
  switch (handle) {
  case 3:
    return Semihosting::write<3>(buffer, size);
  case 4:
    return Semihosting::write<4>(buffer, size);
  case 5:
    return Semihosting::write<5>(buffer, size);
  case 6:
    return Semihosting::write<6>(buffer, size);
  case 7:
    return Semihosting::write<7>(buffer, size);
  case 8:
    return Semihosting::write<8>(buffer, size);
  case 9:
    return Semihosting::write<9>(buffer, size);
  case 10:
    return Semihosting::write<10>(buffer, size);
  default:
    __builtin_unreachable();
  }
}

SemihostingResult transport_seek(unsigned handle, __INT64_TYPE__ offset) {
  switch (handle) {
  case 3:
    return Semihosting::seek<3>(offset);
  case 4:
    return Semihosting::seek<4>(offset);
  case 5:
    return Semihosting::seek<5>(offset);
  case 6:
    return Semihosting::seek<6>(offset);
  case 7:
    return Semihosting::seek<7>(offset);
  case 8:
    return Semihosting::seek<8>(offset);
  case 9:
    return Semihosting::seek<9>(offset);
  case 10:
    return Semihosting::seek<10>(offset);
  default:
    __builtin_unreachable();
  }
}

SemihostingResult transport_tell(unsigned handle) {
  switch (handle) {
  case 3:
    return Semihosting::tell<3>();
  case 4:
    return Semihosting::tell<4>();
  case 5:
    return Semihosting::tell<5>();
  case 6:
    return Semihosting::tell<6>();
  case 7:
    return Semihosting::tell<7>();
  case 8:
    return Semihosting::tell<8>();
  case 9:
    return Semihosting::tell<9>();
  case 10:
    return Semihosting::tell<10>();
  default:
    __builtin_unreachable();
  }
}

} // namespace

extern "C" int *__llvm_libc_errno() noexcept { return &process_errno; }

extern "C" int __llvm_libc_mmix_file_open(const char *path, unsigned mode) {
  if (!valid_path(path) || !valid_mode(mode))
    return fail(EINVAL);

  unsigned index = 0;
  while (index != FILE_CAPACITY && file_slots[index].open)
    ++index;
  if (index == FILE_CAPACITY)
    return fail(EMFILE);

  unsigned handle = FIRST_FILE_HANDLE + index;
  PlatformFileMode platform_mode = static_cast<PlatformFileMode>(mode);
  if (!transport_open(handle, path, transport_mode(platform_mode)).succeeded())
    return fail(EIO);

  file_slots[index] = {handle, platform_mode, true, false, false};
  return static_cast<int>(handle);
}

extern "C" int __llvm_libc_mmix_file_close(int handle) {
  __llvm_libc_stdio_cookie *slot = slot_for(handle);
  if (slot == nullptr || !slot->open)
    return fail(EBADF);

  slot->open = false;
  if (!transport_close(static_cast<unsigned>(handle)).succeeded())
    return fail(EIO);
  return 0;
}

extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_read(int handle, void *buffer,
                                                     __UINT64_TYPE__ size) {
  __llvm_libc_stdio_cookie *slot = slot_for(handle);
  if (slot == nullptr || !slot->open || !can_read(slot->mode))
    return fail_io(EBADF);
  if ((buffer == nullptr && size != 0) || size > MAX_TRANSFER)
    return fail_io(size > MAX_TRANSFER ? EFBIG : EINVAL);

  TransferResult result =
      transport_read(static_cast<unsigned>(handle), buffer, size);
  return result.succeeded ? static_cast<__INT64_TYPE__>(result.count)
                          : fail_io(EIO);
}

extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_write(int handle,
                                                      const void *buffer,
                                                      __UINT64_TYPE__ size) {
  __llvm_libc_stdio_cookie *slot = slot_for(handle);
  if (slot == nullptr || !slot->open || !can_write(slot->mode))
    return fail_io(EBADF);
  if ((buffer == nullptr && size != 0) || size > MAX_TRANSFER)
    return fail_io(size > MAX_TRANSFER ? EFBIG : EINVAL);

  TransferResult result =
      transport_write(static_cast<unsigned>(handle), buffer, size);
  return result.succeeded ? static_cast<__INT64_TYPE__>(result.count)
                          : fail_io(EIO);
}

extern "C" int __llvm_libc_mmix_file_seek(int handle, __INT64_TYPE__ offset) {
  __llvm_libc_stdio_cookie *slot = slot_for(handle);
  if (slot == nullptr || !slot->open)
    return fail(EBADF);
  if (offset < 0)
    return fail(EINVAL);
  if (!transport_seek(static_cast<unsigned>(handle), offset).succeeded())
    return fail(EIO);
  return 0;
}

extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_tell(int handle) {
  __llvm_libc_stdio_cookie *slot = slot_for(handle);
  if (slot == nullptr || !slot->open)
    return fail_io(EBADF);

  SemihostingResult result = transport_tell(static_cast<unsigned>(handle));
  if (!result.succeeded())
    return fail_io(EIO);
  if (result.value > static_cast<__UINT64_TYPE__>(__INT64_MAX__))
    return fail_io(EFBIG);
  return static_cast<__INT64_TYPE__>(result.value);
}

extern "C" __llvm_libc_stdio_cookie *
__llvm_libc_mmix_stream_open(const char *path, unsigned mode) {
  int handle = __llvm_libc_mmix_file_open(path, mode);
  return handle < 0 ? nullptr : slot_for(handle);
}

extern "C" int __llvm_libc_mmix_stream_flush(__llvm_libc_stdio_cookie *stream) {
  stream = stream_for(stream);
  if (stream == nullptr || !stream->open)
    return fail(EBADF);

  // The selected semihosting transport is unbuffered and has no flush
  // service. Reaching this boundary means that all preceding writes have
  // already been submitted to QEMU.
  return 0;
}

extern "C" int __llvm_libc_mmix_stream_close(__llvm_libc_stdio_cookie *stream) {
  stream = stream_for(stream);
  if (!is_regular_stream(stream) || !stream->open)
    return fail(EBADF);
  if (__llvm_libc_mmix_stream_flush(stream) != 0) {
    stream->error = true;
    return -1;
  }
  if (__llvm_libc_mmix_file_close(static_cast<int>(stream->handle)) != 0) {
    stream->error = true;
    return -1;
  }
  return 0;
}

extern "C" int __llvm_libc_mmix_stream_seek(__llvm_libc_stdio_cookie *stream,
                                            __INT64_TYPE__ offset) {
  stream = stream_for(stream);
  if (!is_regular_stream(stream) || !stream->open)
    return fail(EBADF);
  if (__llvm_libc_mmix_file_seek(static_cast<int>(stream->handle), offset) !=
      0) {
    stream->error = true;
    return -1;
  }
  stream->eof = false;
  return 0;
}

extern "C" __INT64_TYPE__
__llvm_libc_mmix_stream_tell(__llvm_libc_stdio_cookie *stream) {
  stream = stream_for(stream);
  if (!is_regular_stream(stream) || !stream->open)
    return fail_stream_io(stream, EBADF);
  __INT64_TYPE__ result =
      __llvm_libc_mmix_file_tell(static_cast<int>(stream->handle));
  if (result < 0)
    stream->error = true;
  return result;
}

extern "C" int
__llvm_libc_mmix_stream_eof(const __llvm_libc_stdio_cookie *stream) {
  stream = stream_for(stream);
  return stream != nullptr && stream->open && stream->eof;
}

extern "C" int
__llvm_libc_mmix_stream_error(const __llvm_libc_stdio_cookie *stream) {
  stream = stream_for(stream);
  return stream != nullptr && stream->open && stream->error;
}

extern "C" void
__llvm_libc_mmix_stream_clearerr(__llvm_libc_stdio_cookie *stream) {
  stream = stream_for(stream);
  if (stream == nullptr || !stream->open)
    return;
  stream->eof = false;
  stream->error = false;
}

extern "C" ssize_t __llvm_libc_stdio_read(void *cookie, char *buffer,
                                          size_t size) {
  __llvm_libc_stdio_cookie *stream = stream_for(cookie);
  if (stream == nullptr || !stream->open || !can_read(stream->mode)) {
    fail_stream_io(stream, EBADF);
    return static_cast<ssize_t>(-EBADF);
  }
  if ((buffer == nullptr && size != 0) || size > MAX_TRANSFER) {
    int error = size > MAX_TRANSFER ? EFBIG : EINVAL;
    fail_stream_io(stream, error);
    return static_cast<ssize_t>(-error);
  }

  __INT64_TYPE__ count;
  if (stream == &__llvm_libc_stdin_cookie) {
    TransferResult result = Semihosting::read<0>(buffer, size);
    if (!result.succeeded) {
      fail_stream_io(stream, EIO);
      return static_cast<ssize_t>(-EIO);
    }
    count = static_cast<__INT64_TYPE__>(result.count);
  } else {
    count = __llvm_libc_mmix_file_read(static_cast<int>(stream->handle), buffer,
                                       size);
    if (count < 0) {
      stream->error = true;
      return static_cast<ssize_t>(-process_errno);
    }
  }
  if (size != 0 && count == 0)
    stream->eof = true;
  return static_cast<ssize_t>(count);
}

extern "C" ssize_t __llvm_libc_stdio_write(void *cookie, const char *buffer,
                                           size_t size) {
  __llvm_libc_stdio_cookie *stream = stream_for(cookie);
  if (stream == nullptr || !stream->open || !can_write(stream->mode)) {
    fail_stream_io(stream, EBADF);
    return static_cast<ssize_t>(-EBADF);
  }
  if ((buffer == nullptr && size != 0) || size > MAX_TRANSFER) {
    int error = size > MAX_TRANSFER ? EFBIG : EINVAL;
    fail_stream_io(stream, error);
    return static_cast<ssize_t>(-error);
  }

  __INT64_TYPE__ count;
  if (stream == &__llvm_libc_stdout_cookie ||
      stream == &__llvm_libc_stderr_cookie) {
    TransferResult result = stream == &__llvm_libc_stdout_cookie
                                ? Semihosting::write<1>(buffer, size)
                                : Semihosting::write<2>(buffer, size);
    if (!result.succeeded) {
      fail_stream_io(stream, EIO);
      return static_cast<ssize_t>(-EIO);
    }
    count = static_cast<__INT64_TYPE__>(result.count);
  } else {
    count = __llvm_libc_mmix_file_write(static_cast<int>(stream->handle),
                                        buffer, size);
    if (count < 0) {
      stream->error = true;
      return static_cast<ssize_t>(-process_errno);
    }
  }
  if (count != static_cast<__INT64_TYPE__>(size)) {
    stream->error = true;
    process_errno = EIO;
  }
  return static_cast<ssize_t>(count);
}

} // namespace LIBC_NAMESPACE_DECL
