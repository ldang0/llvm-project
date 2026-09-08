//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LIBUNWIND_MMIX_UNWIND_CONFIG_H
#define LIBUNWIND_MMIX_UNWIND_CONFIG_H

// Private native storage, in octabytes. Not a SAVE image or a jmp_buf.
#define _LIBUNWIND_MMIX_CONTEXT_SIZE 266
#define _LIBUNWIND_MMIX_CURSOR_SIZE 278
#define _LIBUNWIND_MMIX_HIGHEST_DWARF_REGISTER 304

// Byte offsets shared with native capture/restore assembly.
#define MMIX_UNWIND_GPR_OFFSET(N) ((N) * 8)
#define MMIX_UNWIND_RD_OFFSET 2048
#define MMIX_UNWIND_RE_OFFSET 2056
#define MMIX_UNWIND_RH_OFFSET 2064
#define MMIX_UNWIND_RJ_OFFSET 2072
#define MMIX_UNWIND_RR_OFFSET 2080
#define MMIX_UNWIND_RO_OFFSET 2088
#define MMIX_UNWIND_RS_OFFSET 2096
#define MMIX_UNWIND_RG_OFFSET 2104
#define MMIX_UNWIND_RL_OFFSET 2112
#define MMIX_UNWIND_IP_OFFSET 2120

#endif
