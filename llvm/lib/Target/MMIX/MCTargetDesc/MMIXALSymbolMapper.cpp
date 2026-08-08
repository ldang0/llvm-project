//===-- MMIXALSymbolMapper.cpp - Map symbols for MMIXAL output -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALSymbolMapper.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include <array>
#include <system_error>
#include <utility>

using namespace llvm;

namespace {

constexpr std::array<StringLiteral, 76> PredefinedSymbols = {"rB",
                                                             "rD",
                                                             "rE",
                                                             "rH",
                                                             "rJ",
                                                             "rM",
                                                             "rR",
                                                             "rBB",
                                                             "rC",
                                                             "rN",
                                                             "rO",
                                                             "rS",
                                                             "rI",
                                                             "rT",
                                                             "rTT",
                                                             "rK",
                                                             "rQ",
                                                             "rU",
                                                             "rV",
                                                             "rG",
                                                             "rL",
                                                             "rA",
                                                             "rF",
                                                             "rP",
                                                             "rW",
                                                             "rX",
                                                             "rY",
                                                             "rZ",
                                                             "rWW",
                                                             "rXX",
                                                             "rYY",
                                                             "rZZ",
                                                             "ROUND_CURRENT",
                                                             "ROUND_OFF",
                                                             "ROUND_UP",
                                                             "ROUND_DOWN",
                                                             "ROUND_NEAR",
                                                             "Inf",
                                                             "Data_Segment",
                                                             "Pool_Segment",
                                                             "Stack_Segment",
                                                             "D_BIT",
                                                             "V_BIT",
                                                             "W_BIT",
                                                             "I_BIT",
                                                             "O_BIT",
                                                             "U_BIT",
                                                             "Z_BIT",
                                                             "X_BIT",
                                                             "D_Handler",
                                                             "V_Handler",
                                                             "W_Handler",
                                                             "I_Handler",
                                                             "O_Handler",
                                                             "U_Handler",
                                                             "Z_Handler",
                                                             "X_Handler",
                                                             "StdIn",
                                                             "StdOut",
                                                             "StdErr",
                                                             "TextRead",
                                                             "TextWrite",
                                                             "BinaryRead",
                                                             "BinaryWrite",
                                                             "BinaryReadWrite",
                                                             "Halt",
                                                             "Fopen",
                                                             "Fclose",
                                                             "Fread",
                                                             "Fgets",
                                                             "Fgetws",
                                                             "Fwrite",
                                                             "Fputs",
                                                             "Fputws",
                                                             "Fseek",
                                                             "Ftell"};

bool isASCIILetter(char C) {
  return (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z');
}

bool isASCIIDigit(char C) { return C >= '0' && C <= '9'; }

} // namespace

bool MMIXALSymbolMapper::isValidOrdinarySymbol(StringRef Name) {
  if (Name.empty() || (!isASCIILetter(Name.front()) && Name.front() != '_'))
    return false;

  return llvm::all_of(Name.drop_front(), [](char C) {
    return isASCIILetter(C) || isASCIIDigit(C) || C == '_';
  });
}

bool MMIXALSymbolMapper::isPredefinedSymbol(StringRef Name) {
  return llvm::is_contained(PredefinedSymbols, Name);
}

std::string MMIXALSymbolMapper::getEscapedUserSymbol(StringRef Name) {
  return (Twine("__LLVM_U_") + Twine(Name.size()) + "_" + toHex(Name)).str();
}

Error MMIXALSymbolMapper::registerUserSymbol(StringRef Name) {
  if (Finalized)
    return createStringError(
        std::errc::invalid_argument,
        "cannot register an MMIXAL user symbol after finalization");

  if (!RegisteredUserSymbols.insert(Name).second)
    return createStringError(Twine("duplicate MMIXAL user symbol identity: ") +
                             getEscapedUserSymbol(Name));

  return Error::success();
}

Error MMIXALSymbolMapper::finalize() {
  if (Finalized)
    return createStringError(std::errc::invalid_argument,
                             "MMIXAL symbol mapper is already finalized");

  StringMap<std::string> NewMappings;
  StringMap<std::string> SourcesByMappedName;
  for (const auto &Entry : RegisteredUserSymbols) {
    const StringRef Source = Entry.getKey();
    const bool Preserve = isValidOrdinarySymbol(Source) &&
                          !Source.starts_with("__LLVM_") &&
                          !isPredefinedSymbol(Source);
    std::string Mapped = Preserve ? Source.str() : getEscapedUserSymbol(Source);

    auto [It, Inserted] = SourcesByMappedName.try_emplace(Mapped, Source.str());
    if (!Inserted && StringRef(It->second) != Source)
      return createStringError(Twine("MMIXAL user symbol mapping collision: ") +
                               Mapped);

    NewMappings.try_emplace(Source, std::move(Mapped));
  }

  MappedUserSymbols = std::move(NewMappings);
  Finalized = true;
  return Error::success();
}

Expected<StringRef>
MMIXALSymbolMapper::getMappedUserSymbol(StringRef Name) const {
  if (!Finalized)
    return createStringError(std::errc::invalid_argument,
                             "MMIXAL symbol mapper is not finalized");

  const auto It = MappedUserSymbols.find(Name);
  if (It == MappedUserSymbols.end())
    return createStringError(
        Twine("unregistered MMIXAL user symbol identity: ") +
        getEscapedUserSymbol(Name));

  return StringRef(It->second);
}
