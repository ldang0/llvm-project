//===-- MMIXALSymbolTable.h - Own MMIXAL symbol identities -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSYMBOLTABLE_H
#define LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSYMBOLTABLE_H

#include "MMIXALSymbolMapper.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace llvm {

class MCSymbol;

class MMIXALSymbolTable {
public:
  enum class PrivateSymbolKind {
    BasicBlock,
    ConstantPool,
    JumpTable,
    FunctionEnd,
    BlockAddress,
    Exception,
    Temporary,
  };

private:
  enum class SymbolClass { User, FunctionPrivate, ModulePrivate };

  struct SymbolRegistration {
    const MCSymbol *Symbol;
    SymbolClass Class;
    std::string RawName;
    PrivateSymbolKind Kind = PrivateSymbolKind::Temporary;
    uint64_t Ordinal = 0;
  };

  struct SourceBlockRegistration {
    const MCSymbol *AddressSymbol;
    std::string FunctionName;
    std::string BlockName;
  };

  MMIXALSymbolMapper UserMapper;
  SmallVector<SymbolRegistration, 0> RegisteredSymbols;
  DenseMap<const MCSymbol *, size_t> SymbolIndices;
  SmallVector<SourceBlockRegistration, 0> SourceBlocks;
  DenseMap<const MCSymbol *, size_t> SourceBlockIndices;
  DenseMap<const MCSymbol *, std::string> MappedSymbols;
  DenseMap<const MCSymbol *, std::string> MappedSourceBlockAliases;
  bool Finalized = false;

  Error checkCanRegister(const MCSymbol &Symbol) const;
  static StringRef getPrivateKindTag(PrivateSymbolKind Kind);

public:
  void reset();

  Error registerUserSymbol(const MCSymbol &Symbol, StringRef RawName);
  Error registerSourceBlock(const MCSymbol &AddressSymbol,
                            StringRef FunctionName, StringRef BlockName);
  Error registerFunctionPrivateSymbol(const MCSymbol &Symbol,
                                      StringRef FunctionName,
                                      PrivateSymbolKind Kind, uint64_t Ordinal);
  Error registerModulePrivateSymbol(const MCSymbol &Symbol,
                                    PrivateSymbolKind Kind, uint64_t Ordinal);

  Error finalize();
  Expected<StringRef> getMappedSymbol(const MCSymbol &Symbol) const;
  Expected<StringRef> getSourceBlockAlias(const MCSymbol &AddressSymbol) const;

  bool isFinalized() const { return Finalized; }
  bool isRegistered(const MCSymbol &Symbol) const {
    return SymbolIndices.contains(&Symbol);
  }
  std::optional<PrivateSymbolKind>
  getPrivateSymbolKind(const MCSymbol &Symbol) const;
  size_t getNumRegisteredSymbols() const { return RegisteredSymbols.size(); }
  size_t getNumSourceBlockAliases() const { return SourceBlocks.size(); }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MMIX_MCTARGETDESC_MMIXALSYMBOLTABLE_H
