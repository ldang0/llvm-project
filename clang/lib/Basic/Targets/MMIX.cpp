//===--- MMIX.cpp - Implement MMIX target feature support ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the MMIX TargetInfo object.
//
//===----------------------------------------------------------------------===//

#include "MMIX.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/MacroBuilder.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

static bool isMMIXFeature(StringRef Feature) {
  return llvm::StringSwitch<bool>(Feature)
      .Cases({"base", "system", "cache", "virtual-memory"}, true)
      .Default(false);
}

void MMIXTargetInfo::getTargetDefines(const LangOptions &,
                                      MacroBuilder &Builder) const {
  Builder.defineMacro("__mmix__");
  Builder.defineMacro("__MMIX__");
  Builder.defineMacro("__MMIX_ABI_GNU__");
}

bool MMIXTargetInfo::initFeatureMap(
    llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags, StringRef CPU,
    const std::vector<std::string> &FeatureVec) const {
  Features["base"] = true;
  Features["system"] = true;
  Features["cache"] = true;
  Features["virtual-memory"] = true;

  for (StringRef Feature : FeatureVec) {
    if (Feature.empty() || (Feature[0] != '+' && Feature[0] != '-'))
      continue;
    if (!isMMIXFeature(Feature.drop_front())) {
      Diags.Report(diag::err_invalid_feature_combination)
          << ("unknown MMIX target feature '" + Feature.drop_front() + "'")
                 .str();
      return false;
    }
  }

  if (!TargetInfo::initFeatureMap(Features, Diags, CPU, FeatureVec))
    return false;

  if (!Features.lookup("base")) {
    Diags.Report(diag::err_invalid_feature_combination)
        << "MMIX C requires the base feature";
    return false;
  }
  return true;
}

bool MMIXTargetInfo::handleTargetFeatures(std::vector<std::string> &Features,
                                          DiagnosticsEngine &) {
  for (StringRef Feature : Features) {
    bool Enabled = Feature.consume_front("+");
    if (!Enabled && !Feature.consume_front("-"))
      continue;

    if (Feature == "base")
      HasBase = Enabled;
    else if (Feature == "system")
      HasSystem = Enabled;
    else if (Feature == "cache")
      HasCache = Enabled;
    else if (Feature == "virtual-memory")
      HasVirtualMemory = Enabled;
  }
  return true;
}

bool MMIXTargetInfo::hasFeature(StringRef Feature) const {
  return llvm::StringSwitch<bool>(Feature)
      .Case("base", HasBase)
      .Case("system", HasSystem)
      .Case("cache", HasCache)
      .Case("virtual-memory", HasVirtualMemory)
      .Default(false);
}

bool MMIXTargetInfo::isValidCPUName(StringRef Name) const {
  return Name == "generic";
}

void MMIXTargetInfo::fillValidCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  Values.emplace_back("generic");
}
