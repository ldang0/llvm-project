//===-- crtdso.c - MMIX static executable identity -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

__attribute__((visibility("hidden"))) void *__dso_handle = &__dso_handle;
