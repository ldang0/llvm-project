//===--- MMIXQEMU.h - MMIX QEMU platform policy ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXQEMU_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXQEMU_H

#include "MMIXPlatform.h"

namespace clang {
namespace driver {
namespace toolchains {
namespace mmix {
namespace qemu {

ExecutionPlatform getExecutionPlatform();

} // namespace qemu
} // namespace mmix
} // namespace toolchains
} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_MMIXQEMU_H
