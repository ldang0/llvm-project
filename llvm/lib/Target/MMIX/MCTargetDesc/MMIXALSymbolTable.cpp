//===-- MMIXALSymbolTable.cpp - Own MMIXAL symbol identities -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALSymbolTable.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include <system_error>
#include <utility>

using namespace llvm;

void MMIXALSymbolTable::reset() { *this = MMIXALSymbolTable(); }

Error MMIXALSymbolTable::checkCanRegister(const MCSymbol &Symbol) const {
  if (Finalized)
    return createStringError(
        std::errc::invalid_argument,
        "cannot register an MMIXAL symbol after finalization");
  if (SymbolIndices.contains(&Symbol))
    return createStringError(
        Twine("duplicate MMIXAL MC symbol registration: ") + Symbol.getName());
  return Error::success();
}

StringRef MMIXALSymbolTable::getPrivateKindTag(PrivateSymbolKind Kind) {
  switch (Kind) {
  case PrivateSymbolKind::BasicBlock:
    return "BB";
  case PrivateSymbolKind::ConstantPool:
    return "CP";
  case PrivateSymbolKind::JumpTable:
    return "JT";
  case PrivateSymbolKind::FunctionEnd:
    return "END";
  case PrivateSymbolKind::BlockAddress:
    return "BA";
  case PrivateSymbolKind::Exception:
    return "EH";
  case PrivateSymbolKind::Temporary:
    return "TMP";
  }
  llvm_unreachable("invalid MMIXAL private symbol kind");
}

Error MMIXALSymbolTable::registerUserSymbol(const MCSymbol &Symbol,
                                            StringRef RawName) {
  if (Error Err = checkCanRegister(Symbol))
    return Err;
  if (Error Err = UserMapper.registerUserSymbol(RawName))
    return Err;

  SymbolIndices.try_emplace(&Symbol, RegisteredSymbols.size());
  RegisteredSymbols.push_back({&Symbol, SymbolClass::User, RawName.str(),
                               PrivateSymbolKind::Temporary, 0});
  return Error::success();
}

Error MMIXALSymbolTable::registerSourceBlock(const MCSymbol &AddressSymbol,
                                             StringRef FunctionName,
                                             StringRef BlockName) {
  if (Finalized)
    return createStringError(
        std::errc::invalid_argument,
        "cannot register an MMIXAL source block after finalization");
  if (!SymbolIndices.contains(&AddressSymbol))
    return createStringError(
        Twine("MMIXAL source block has no registered address symbol: ") +
        AddressSymbol.getName());
  if (SourceBlockIndices.contains(&AddressSymbol))
    return createStringError(
        Twine("duplicate MMIXAL source-block address registration: ") +
        AddressSymbol.getName());
  if (Error Err = UserMapper.registerSemanticName(BlockName))
    return Err;

  SourceBlockIndices.try_emplace(&AddressSymbol, SourceBlocks.size());
  SourceBlocks.push_back({&AddressSymbol, FunctionName.str(), BlockName.str()});
  return Error::success();
}

Error MMIXALSymbolTable::registerFunctionPrivateSymbol(const MCSymbol &Symbol,
                                                       StringRef FunctionName,
                                                       PrivateSymbolKind Kind,
                                                       uint64_t Ordinal) {
  if (Error Err = checkCanRegister(Symbol))
    return Err;

  SymbolIndices.try_emplace(&Symbol, RegisteredSymbols.size());
  RegisteredSymbols.push_back({&Symbol, SymbolClass::FunctionPrivate,
                               FunctionName.str(), Kind, Ordinal});
  return Error::success();
}

Error MMIXALSymbolTable::registerModulePrivateSymbol(const MCSymbol &Symbol,
                                                     PrivateSymbolKind Kind,
                                                     uint64_t Ordinal) {
  if (Error Err = checkCanRegister(Symbol))
    return Err;

  SymbolIndices.try_emplace(&Symbol, RegisteredSymbols.size());
  RegisteredSymbols.push_back(
      {&Symbol, SymbolClass::ModulePrivate, {}, Kind, Ordinal});
  return Error::success();
}

Error MMIXALSymbolTable::finalize() {
  if (Finalized)
    return createStringError(std::errc::invalid_argument,
                             "MMIXAL symbol table is already finalized");
  if (Error Err = UserMapper.finalize())
    return Err;

  DenseMap<const MCSymbol *, std::string> NewMappedSymbols;
  DenseMap<const MCSymbol *, std::string> NewMappedAliases;
  StringSet<> ClaimedMappedNames;

  for (const SymbolRegistration &Registration : RegisteredSymbols) {
    std::string Mapped;
    switch (Registration.Class) {
    case SymbolClass::User: {
      Expected<StringRef> UserName =
          UserMapper.getMappedUserSymbol(Registration.RawName);
      if (!UserName)
        return UserName.takeError();
      Mapped = UserName->str();
      break;
    }
    case SymbolClass::FunctionPrivate:
      Mapped = (Twine("__LLVM_L_F_") + toHex(Registration.RawName) + "_" +
                getPrivateKindTag(Registration.Kind) + "_" +
                Twine(Registration.Ordinal))
                   .str();
      break;
    case SymbolClass::ModulePrivate:
      Mapped = (Twine("__LLVM_M_") + getPrivateKindTag(Registration.Kind) +
                "_" + Twine(Registration.Ordinal))
                   .str();
      break;
    }

    if (!ClaimedMappedNames.insert(Mapped).second)
      return createStringError(Twine("MMIXAL symbol mapping collision: ") +
                               Mapped);
    NewMappedSymbols.try_emplace(Registration.Symbol, std::move(Mapped));
  }

  for (const SourceBlockRegistration &Block : SourceBlocks) {
    Expected<bool> Preserve = UserMapper.canPreserveName(Block.BlockName);
    if (!Preserve)
      return Preserve.takeError();
    std::string Mapped =
        *Preserve ? Block.BlockName
                  : (Twine("__LLVM_B_F_") + toHex(Block.FunctionName) + "_B_" +
                     toHex(Block.BlockName))
                        .str();

    if (!ClaimedMappedNames.insert(Mapped).second)
      return createStringError(
          Twine("MMIXAL source-block mapping collision: ") + Mapped);
    NewMappedAliases.try_emplace(Block.AddressSymbol, std::move(Mapped));
  }

  MappedSymbols = std::move(NewMappedSymbols);
  MappedSourceBlockAliases = std::move(NewMappedAliases);
  Finalized = true;
  return Error::success();
}

Expected<StringRef>
MMIXALSymbolTable::getMappedSymbol(const MCSymbol &Symbol) const {
  if (!Finalized)
    return createStringError(std::errc::invalid_argument,
                             "MMIXAL symbol table is not finalized");
  const auto It = MappedSymbols.find(&Symbol);
  if (It == MappedSymbols.end())
    return createStringError(Twine("unregistered MMIXAL MC symbol: ") +
                             Symbol.getName());
  return StringRef(It->second);
}

Expected<StringRef>
MMIXALSymbolTable::getSourceBlockAlias(const MCSymbol &AddressSymbol) const {
  if (!Finalized)
    return createStringError(std::errc::invalid_argument,
                             "MMIXAL symbol table is not finalized");
  const auto It = MappedSourceBlockAliases.find(&AddressSymbol);
  if (It == MappedSourceBlockAliases.end())
    return createStringError(
        Twine("MMIXAL symbol has no source-block alias: ") +
        AddressSymbol.getName());
  return StringRef(It->second);
}
