//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LIBUNWIND_MMIX_CONTEXT_H
#define LIBUNWIND_MMIX_CONTEXT_H

#include <mmix/UnwindConfig.h>
#include <stddef.h>
#include <stdint.h>

namespace libunwind {
struct MMIXUnwindContext {
  uint64_t gpr[256];
  uint64_t rd, re, rh, rj, rr, ro, rs, rg, rl;
  uint64_t ip;
};
static_assert(sizeof(MMIXUnwindContext) == _LIBUNWIND_MMIX_CONTEXT_SIZE * 8,
              "MMIX context size must match assembly");
static_assert(alignof(MMIXUnwindContext) == 8,
              "MMIX context must be octa aligned");
static_assert(offsetof(MMIXUnwindContext, gpr) == MMIX_UNWIND_GPR_OFFSET(0),
              "MMIX GPR offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rd) == MMIX_UNWIND_RD_OFFSET,
              "MMIX rd offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, re) == MMIX_UNWIND_RE_OFFSET,
              "MMIX re offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rh) == MMIX_UNWIND_RH_OFFSET,
              "MMIX rh offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rj) == MMIX_UNWIND_RJ_OFFSET,
              "MMIX rj offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rr) == MMIX_UNWIND_RR_OFFSET,
              "MMIX rr offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, ro) == MMIX_UNWIND_RO_OFFSET,
              "MMIX ro offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rs) == MMIX_UNWIND_RS_OFFSET,
              "MMIX rs offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rg) == MMIX_UNWIND_RG_OFFSET,
              "MMIX rg offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, rl) == MMIX_UNWIND_RL_OFFSET,
              "MMIX rl offset must match assembly");
static_assert(offsetof(MMIXUnwindContext, ip) == MMIX_UNWIND_IP_OFFSET,
              "MMIX ip offset must match assembly");
} // namespace libunwind

#endif
