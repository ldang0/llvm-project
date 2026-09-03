//===-- ABISysV_mmix.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABISysV_mmix.h"

#include "lldb/Core/PluginManager.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/Status.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/TargetParser/Triple.h"

using namespace lldb;
using namespace lldb_private;

LLDB_PLUGIN_DEFINE_ADV(ABISysV_mmix, ABIMMIX)

static uint32_t getMMIXDWARFRegisterNumber(llvm::StringRef name) {
  llvm::StringRef number = name;
  if (number.consume_front("r")) {
    unsigned reg;
    if (llvm::to_integer(number, reg, 10) && reg < 256 &&
        number == std::to_string(reg))
      return reg >= 224 ? reg - 224 : reg + 48;
  }

  return llvm::StringSwitch<uint32_t>(name)
      .Case("rB", 272)
      .Case("rD", 32)
      .Case("rE", 33)
      .Case("rH", 34)
      .Case("rJ", 35)
      .Case("rM", 277)
      .Case("rR", 36)
      .Case("rBB", 279)
      .Case("rC", 280)
      .Case("rN", 281)
      .Case("rO", 38)
      .Case("rS", 283)
      .Case("rI", 284)
      .Case("rT", 285)
      .Case("rTT", 286)
      .Case("rK", 287)
      .Case("rQ", 288)
      .Case("rU", 289)
      .Case("rV", 290)
      .Case("rG", 291)
      .Case("rL", 292)
      .Case("rA", 293)
      .Case("rF", 294)
      .Case("rP", 295)
      .Case("rW", 296)
      .Case("rX", 297)
      .Case("rY", 298)
      .Case("rZ", 299)
      .Case("rWW", 300)
      .Case("rXX", 301)
      .Case("rYY", 302)
      .Case("rZZ", 303)
      .Case("pc", 304)
      .Default(LLDB_INVALID_REGNUM);
}

void ABISysV_mmix::Initialize() {
  PluginManager::RegisterPlugin(
      GetPluginNameStatic(), "System V ABI for MMIX targets", CreateInstance);
}

void ABISysV_mmix::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

ABISP ABISysV_mmix::CreateInstance(ProcessSP process_sp, const ArchSpec &arch) {
  if (arch.GetTriple().getArch() != llvm::Triple::mmix)
    return {};
  return ABISP(
      new ABISysV_mmix(std::move(process_sp), MakeMCRegisterInfo(arch)));
}

std::pair<uint32_t, uint32_t>
ABISysV_mmix::GetEHAndDWARFNums(llvm::StringRef name) {
  uint32_t number = getMMIXDWARFRegisterNumber(name);
  return {number, number};
}

uint32_t ABISysV_mmix::GetGenericNum(llvm::StringRef name) {
  return llvm::StringSwitch<uint32_t>(name)
      .Case("pc", LLDB_REGNUM_GENERIC_PC)
      .Case("r253", LLDB_REGNUM_GENERIC_FP)
      .Case("r254", LLDB_REGNUM_GENERIC_SP)
      .Case("rJ", LLDB_REGNUM_GENERIC_RA)
      .Default(LLDB_INVALID_REGNUM);
}

bool ABISysV_mmix::PrepareTrivialCall(Thread &, addr_t, addr_t, addr_t,
                                      llvm::ArrayRef<addr_t>) const {
  return false;
}

bool ABISysV_mmix::GetArgumentValues(Thread &, ValueList &) const {
  return false;
}

Status ABISysV_mmix::SetReturnValueObject(StackFrameSP &, ValueObjectSP &) {
  return Status::FromErrorString("setting MMIX return values is not supported");
}

ValueObjectSP ABISysV_mmix::GetReturnValueObjectImpl(Thread &,
                                                     CompilerType &) const {
  return {};
}

UnwindPlanSP ABISysV_mmix::CreateFunctionEntryUnwindPlan() { return {}; }

UnwindPlanSP ABISysV_mmix::CreateDefaultUnwindPlan() { return {}; }

bool ABISysV_mmix::RegisterIsVolatile(const RegisterInfo *) { return true; }

bool ABISysV_mmix::CallFrameAddressIsValid(addr_t cfa) {
  return cfa != 0 && (cfa & 7) == 0;
}

bool ABISysV_mmix::CodeAddressIsValid(addr_t pc) { return (pc & 3) == 0; }

void ABISysV_mmix::AugmentRegisterInfo(
    std::vector<DynamicRegisterInfo::Register> &regs) {
  MCBasedABI::AugmentRegisterInfo(regs);

  for (DynamicRegisterInfo::Register &reg : regs) {
    llvm::StringRef number = reg.name.GetStringRef();
    if (!number.consume_front("r"))
      continue;

    unsigned arch_reg;
    if (!llvm::to_integer(number, arch_reg, 10) || arch_reg >= 256 ||
        number != std::to_string(arch_reg))
      continue;

    if (!reg.alt_name)
      reg.alt_name = reg.name;
    reg.name = ConstString("$" + std::to_string(arch_reg));
  }
}
