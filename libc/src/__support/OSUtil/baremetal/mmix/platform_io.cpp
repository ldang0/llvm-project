//===-- MMIX QEMU platform I/O adapter -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "platform_io.h"

#include "hdr/errno_macros.h"
#include "src/__support/OSUtil/baremetal/io.h"
#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"

namespace LIBC_NAMESPACE_DECL {
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

struct FileSlot {
  bool open;
  PlatformFileMode mode;
};

LIBC_CONSTINIT FileSlot file_slots[FILE_CAPACITY] = {};
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

LIBC_INLINE FileSlot *slot_for(int handle) {
  if (handle < static_cast<int>(FIRST_FILE_HANDLE) ||
      handle >= static_cast<int>(FILE_HANDLE_LIMIT))
    return nullptr;
  return &file_slots[static_cast<unsigned>(handle) - FIRST_FILE_HANDLE];
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

  file_slots[index] = {true, platform_mode};
  return static_cast<int>(handle);
}

extern "C" int __llvm_libc_mmix_file_close(int handle) {
  FileSlot *slot = slot_for(handle);
  if (slot == nullptr || !slot->open)
    return fail(EBADF);

  slot->open = false;
  if (!transport_close(static_cast<unsigned>(handle)).succeeded())
    return fail(EIO);
  return 0;
}

extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_read(int handle, void *buffer,
                                                     __UINT64_TYPE__ size) {
  FileSlot *slot = slot_for(handle);
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
  FileSlot *slot = slot_for(handle);
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
  FileSlot *slot = slot_for(handle);
  if (slot == nullptr || !slot->open)
    return fail(EBADF);
  if (offset < 0)
    return fail(EINVAL);
  if (!transport_seek(static_cast<unsigned>(handle), offset).succeeded())
    return fail(EIO);
  return 0;
}

extern "C" __INT64_TYPE__ __llvm_libc_mmix_file_tell(int handle) {
  FileSlot *slot = slot_for(handle);
  if (slot == nullptr || !slot->open)
    return fail_io(EBADF);

  SemihostingResult result = transport_tell(static_cast<unsigned>(handle));
  if (!result.succeeded())
    return fail_io(EIO);
  if (result.value > static_cast<__UINT64_TYPE__>(__INT64_MAX__))
    return fail_io(EFBIG);
  return static_cast<__INT64_TYPE__>(result.value);
}

struct __llvm_libc_stdio_cookie {
  unsigned handle;
};

extern "C" {
__llvm_libc_stdio_cookie __llvm_libc_stdin_cookie = {0};
__llvm_libc_stdio_cookie __llvm_libc_stdout_cookie = {1};
__llvm_libc_stdio_cookie __llvm_libc_stderr_cookie = {2};
}

extern "C" ssize_t __llvm_libc_stdio_read(void *cookie, char *buffer,
                                          size_t size) {
  if (cookie != &__llvm_libc_stdin_cookie) {
    process_errno = EBADF;
    return static_cast<ssize_t>(-EBADF);
  }
  if ((buffer == nullptr && size != 0) || size > MAX_TRANSFER) {
    int error = size > MAX_TRANSFER ? EFBIG : EINVAL;
    process_errno = error;
    return static_cast<ssize_t>(-error);
  }

  TransferResult result = Semihosting::read<0>(buffer, size);
  if (!result.succeeded) {
    process_errno = EIO;
    return static_cast<ssize_t>(-EIO);
  }
  return static_cast<ssize_t>(result.count);
}

extern "C" ssize_t __llvm_libc_stdio_write(void *cookie, const char *buffer,
                                           size_t size) {
  if ((buffer == nullptr && size != 0) || size > MAX_TRANSFER) {
    int error = size > MAX_TRANSFER ? EFBIG : EINVAL;
    process_errno = error;
    return static_cast<ssize_t>(-error);
  }

  TransferResult result;
  if (cookie == &__llvm_libc_stdout_cookie)
    result = Semihosting::write<1>(buffer, size);
  else if (cookie == &__llvm_libc_stderr_cookie)
    result = Semihosting::write<2>(buffer, size);
  else {
    process_errno = EBADF;
    return static_cast<ssize_t>(-EBADF);
  }
  if (!result.succeeded) {
    process_errno = EIO;
    return static_cast<ssize_t>(-EIO);
  }
  return static_cast<ssize_t>(result.count);
}

} // namespace LIBC_NAMESPACE_DECL
