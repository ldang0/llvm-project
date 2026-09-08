//===-- MMIX bounded single-process atexit -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdlib/atexit.h"
#include "hdr/types/atexithandler_t.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {
namespace {

using AtExitCallback = void(void *);

constexpr __SIZE_TYPE__ CALLBACK_CAPACITY = 64;
AtExitCallback *callbacks[CALLBACK_CAPACITY];
void *callback_payloads[CALLBACK_CAPACITY];
void *callback_dsos[CALLBACK_CAPACITY];
__SIZE_TYPE__ callback_count;

int add_callback(AtExitCallback *callback, void *payload, void *dso) {
  if (callback == nullptr || callback_count == CALLBACK_CAPACITY)
    return -1;
  callbacks[callback_count] = callback;
  callback_payloads[callback_count] = payload;
  callback_dsos[callback_count++] = dso;
  return 0;
}

void call_stdc_callback(void *payload) {
  reinterpret_cast<__atexithandler_t>(payload)();
}

void call_callbacks(void *dso) {
  __SIZE_TYPE__ index = callback_count;
  while (index != 0) {
    --index;
    if (dso != nullptr && callback_dsos[index] != dso)
      continue;

    AtExitCallback *callback = callbacks[index];
    void *payload = callback_payloads[index];
    --callback_count;
    for (__SIZE_TYPE__ move = index; move != callback_count; ++move) {
      callbacks[move] = callbacks[move + 1];
      callback_payloads[move] = callback_payloads[move + 1];
      callback_dsos[move] = callback_dsos[move + 1];
    }

    callback(payload);
    index = callback_count;
  }
}

} // namespace

extern "C" int __cxa_atexit(AtExitCallback *callback, void *payload,
                            void *dso) {
  return add_callback(callback, payload, dso);
}

extern "C" void __cxa_finalize(void *dso) { call_callbacks(dso); }

LLVM_LIBC_FUNCTION(int, atexit, (__atexithandler_t callback)) {
  return callback == nullptr
             ? -1
             : add_callback(&call_stdc_callback,
                            reinterpret_cast<void *>(callback), nullptr);
}

} // namespace LIBC_NAMESPACE_DECL
