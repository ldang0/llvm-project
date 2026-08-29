//===-- MMIX QEMU semihosting transport ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_SEMIHOSTING_H
#define LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_SEMIHOSTING_H

#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {
namespace internal {
namespace mmix {

enum class FileMode : __UINT64_TYPE__ {
  TEXT_READ = 0,
  TEXT_WRITE = 1,
  BINARY_READ = 2,
  BINARY_WRITE = 3,
  BINARY_READ_WRITE = 4,
};

struct SemihostingResult {
  __UINT64_TYPE__ value;

  LIBC_INLINE constexpr bool succeeded() const {
    return value != static_cast<__UINT64_TYPE__>(-1);
  }
};

struct TransferResult {
  __UINT64_TYPE__ count;
  bool succeeded;
};

class Semihosting {
  static __UINT64_TYPE__ trap_intrinsic(__UINT64_TYPE__, unsigned,
                                        unsigned) __asm__("llvm.mmix.trap");

  enum class Service : unsigned {
    HALT = 0,
    FOPEN = 1,
    FCLOSE = 2,
    FREAD = 3,
    FGETS = 4,
    FWRITE = 6,
    FPUTS = 7,
    FSEEK = 9,
    FTELL = 10,
  };

  struct Arguments {
    __UINT64_TYPE__ value;
    __UINT64_TYPE__ size_or_mode;
  };

  static_assert(sizeof(Arguments) == 16 && alignof(Arguments) == 8,
                "MMIX semihosting argument blocks contain two octas");
  static_assert(__builtin_offsetof(Arguments, size_or_mode) == 8,
                "MMIX semihosting arguments are consecutive octas");

  template <Service S, unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static __UINT64_TYPE__
  trap(__UINT64_TYPE__ argument) {
    static_assert(Handle <= 255, "MMIX TRAP handles occupy one byte");

    return trap_intrinsic(argument, static_cast<unsigned>(S), Handle);
  }

  template <Service S, unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static __UINT64_TYPE__
  trap(const Arguments &arguments) {
    return trap<S, Handle>(static_cast<__UINT64_TYPE__>(
        reinterpret_cast<__UINTPTR_TYPE__>(&arguments)));
  }

  LIBC_INLINE static TransferResult decode_transfer(__UINT64_TYPE__ raw,
                                                    __UINT64_TYPE__ requested) {
    __UINT64_TYPE__ count = raw + requested;
    if (count == static_cast<__UINT64_TYPE__>(-1))
      return {0, false};
    return {count, true};
  }

public:
  [[noreturn]] [[gnu::always_inline]] LIBC_INLINE static void
  halt(unsigned status) {
    trap<Service::HALT, 0>(status & 0xff);
    __builtin_unreachable();
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static SemihostingResult
  open(const char *path, FileMode mode) {
    static_assert(Handle >= 3, "Fopen requires a regular-file handle");
    Arguments arguments = {
        static_cast<__UINT64_TYPE__>(reinterpret_cast<__UINTPTR_TYPE__>(path)),
        static_cast<__UINT64_TYPE__>(mode),
    };
    return {trap<Service::FOPEN, Handle>(arguments)};
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static SemihostingResult close() {
    static_assert(Handle >= 3, "Fclose requires a regular-file handle");
    return {trap<Service::FCLOSE, Handle>(0)};
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static TransferResult
  read(void *buffer, __UINT64_TYPE__ size) {
    static_assert(Handle == 0 || Handle >= 3,
                  "Fread requires stdin or a regular-file handle");
    Arguments arguments = {
        static_cast<__UINT64_TYPE__>(
            reinterpret_cast<__UINTPTR_TYPE__>(buffer)),
        size,
    };
    return decode_transfer(trap<Service::FREAD, Handle>(arguments), size);
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static TransferResult
  get_line(char *buffer, __UINT64_TYPE__ size) {
    static_assert(Handle == 0 || Handle >= 3,
                  "Fgets requires stdin or a regular-file handle");
    Arguments arguments = {
        static_cast<__UINT64_TYPE__>(
            reinterpret_cast<__UINTPTR_TYPE__>(buffer)),
        size,
    };
    __UINT64_TYPE__ result = trap<Service::FGETS, Handle>(arguments);
    return {result == static_cast<__UINT64_TYPE__>(-1) ? 0 : result,
            result != static_cast<__UINT64_TYPE__>(-1)};
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static TransferResult
  write(const void *buffer, __UINT64_TYPE__ size) {
    static_assert(Handle == 1 || Handle == 2 || Handle >= 3,
                  "Fwrite requires stdout, stderr, or a file handle");
    Arguments arguments = {
        static_cast<__UINT64_TYPE__>(
            reinterpret_cast<__UINTPTR_TYPE__>(buffer)),
        size,
    };
    return decode_transfer(trap<Service::FWRITE, Handle>(arguments), size);
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static SemihostingResult
  put_string(const char *string) {
    static_assert(Handle == 1 || Handle == 2,
                  "Fputs is restricted to stdout and stderr");
    return {trap<Service::FPUTS, Handle>(static_cast<__UINT64_TYPE__>(
        reinterpret_cast<__UINTPTR_TYPE__>(string)))};
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static SemihostingResult
  seek(__INT64_TYPE__ offset) {
    static_assert(Handle >= 3, "Fseek requires a regular-file handle");
    return {trap<Service::FSEEK, Handle>(static_cast<__UINT64_TYPE__>(offset))};
  }

  template <unsigned Handle>
  [[gnu::always_inline]] LIBC_INLINE static SemihostingResult tell() {
    static_assert(Handle >= 3, "Ftell requires a regular-file handle");
    return {trap<Service::FTELL, Handle>(0)};
  }
};

} // namespace mmix
} // namespace internal
} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC___SUPPORT_OSUTIL_BAREMETAL_MMIX_SEMIHOSTING_H
