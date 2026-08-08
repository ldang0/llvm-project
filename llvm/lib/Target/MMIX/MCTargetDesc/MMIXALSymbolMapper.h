//===-- MMIXALSymbolMapper.h - Map symbols for MMIXAL output ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSYMBOLMAPPER_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSYMBOLMAPPER_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Error.h"
#include <cstddef>
#include <string>

namespace llvm {

class MMIXALSymbolMapper {
  StringSet<> RegisteredUserSymbols;
  StringMap<std::string> MappedUserSymbols;
  bool Finalized = false;

public:
  Error registerUserSymbol(StringRef Name);
  Error finalize();
  Expected<StringRef> getMappedUserSymbol(StringRef Name) const;

  bool isFinalized() const { return Finalized; }
  size_t getNumUserSymbols() const { return RegisteredUserSymbols.size(); }

  static bool isValidOrdinarySymbol(StringRef Name);
  static bool isPredefinedSymbol(StringRef Name);
  static std::string getEscapedUserSymbol(StringRef Name);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSYMBOLMAPPER_H
