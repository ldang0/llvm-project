//===-- MMIXALDependencyGraph.h - Schedule MMIXAL items -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALDEPENDENCYGRAPH_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALDEPENDENCYGRAPH_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"
#include <cstddef>

namespace llvm {

using MMIXALItemDependencies = SmallVector<size_t, 2>;

Expected<SmallVector<size_t, 0>>
scheduleMMIXALItems(ArrayRef<MMIXALItemDependencies> Dependencies);

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALDEPENDENCYGRAPH_H
