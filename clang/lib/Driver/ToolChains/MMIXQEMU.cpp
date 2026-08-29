//===--- MMIXQEMU.cpp - MMIX QEMU platform policy -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXQEMU.h"

using namespace clang::driver::toolchains;

mmix::ExecutionPlatform mmix::qemu::getExecutionPlatform() {
  return ExecutionPlatform::QEMU;
}
