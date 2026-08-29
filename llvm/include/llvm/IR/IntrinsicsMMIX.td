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
def int_mmix_preld
    : Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty],
                [IntrInaccessibleMemOnly, NoCapture<ArgIndex<0>>,
                 ReadNone<ArgIndex<0>>, ImmArg<ArgIndex<1>>,
                 IntrNoDuplicate]>;
def int_mmix_prego
    : Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty],
                [IntrInaccessibleMemOnly, NoCapture<ArgIndex<0>>,
                 ReadNone<ArgIndex<0>>, ImmArg<ArgIndex<1>>,
                 IntrNoDuplicate]>;
def int_mmix_prest
    : Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty],
                [IntrInaccessibleMemOnly, NoCapture<ArgIndex<0>>,
                 ReadNone<ArgIndex<0>>, ImmArg<ArgIndex<1>>,
                 IntrNoDuplicate]>;
def int_mmix_syncd
    : Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty],
                [IntrInaccessibleMemOrArgMemOnly, NoCapture<ArgIndex<0>>,
                 ImmArg<ArgIndex<1>>, IntrNoDuplicate]>;
def int_mmix_syncid
    : Intrinsic<[], [llvm_ptr_ty, llvm_i32_ty],
                [IntrInaccessibleMemOrArgMemOnly, NoCapture<ArgIndex<0>>,
                 ImmArg<ArgIndex<1>>, IntrNoDuplicate]>;
def int_mmix_sync
    : Intrinsic<[], [llvm_i32_ty],
                [ImmArg<ArgIndex<0>>, IntrNoDuplicate]>;
def int_mmix_ldunc
    : Intrinsic<[llvm_i64_ty], [llvm_ptr_ty],
                [IntrRead<[ArgMem, InaccessibleMem]>,
                 IntrWrite<[InaccessibleMem]>, NoCapture<ArgIndex<0>>,
                 ReadOnly<ArgIndex<0>>, Align<ArgIndex<0>, 8>,
                 IntrNoDuplicate],
                "", [SDNPMemOperand]>;
def int_mmix_stunc
    : Intrinsic<[], [llvm_ptr_ty, llvm_i64_ty],
                [IntrRead<[InaccessibleMem]>,
                 IntrWrite<[ArgMem, InaccessibleMem]>,
                 NoCapture<ArgIndex<0>>,
                 WriteOnly<ArgIndex<0>>, Align<ArgIndex<0>, 8>,
                 IntrNoDuplicate],
                "", [SDNPMemOperand]>;
def int_mmix_ldvts
    : Intrinsic<[llvm_i64_ty], [llvm_i64_ty],
                [IntrInaccessibleMemOnly, IntrNoDuplicate,
                 NoUndef<RetIndex>, Range<RetIndex, 0, 4>]>;
def int_mmix_trap
    : Intrinsic<[llvm_i64_ty], [llvm_i64_ty, llvm_i32_ty, llvm_i32_ty],
                [ImmArg<ArgIndex<1>>, ImmArg<ArgIndex<2>>, IntrNoDuplicate]>;

} // TargetPrefix = "mmix"
