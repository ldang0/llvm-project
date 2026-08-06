//===- IntrinsicsMMIX.td - Defines MMIX intrinsics ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines all of the MMIX-specific intrinsics.
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "mmix" in {

def int_mmix_get
    : DefaultAttrsIntrinsic<[llvm_i64_ty], [llvm_i32_ty],
                            [ImmArg<ArgIndex<0>>]>;
def int_mmix_put
    : DefaultAttrsIntrinsic<[], [llvm_i32_ty, llvm_i64_ty],
                            [ImmArg<ArgIndex<0>>]>;

} // TargetPrefix = "mmix"
