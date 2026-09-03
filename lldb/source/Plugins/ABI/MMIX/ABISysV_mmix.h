//===-- ABISysV_mmix.h -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_MMIX_ABISYSV_MMIX_H
#define LLDB_SOURCE_PLUGINS_ABI_MMIX_ABISYSV_MMIX_H

#include "lldb/Target/ABI.h"

class ABISysV_mmix : public lldb_private::MCBasedABI {
public:
  ~ABISysV_mmix() override = default;

  static void Initialize();
  static void Terminate();

  static lldb::ABISP CreateInstance(lldb::ProcessSP process_sp,
                                    const lldb_private::ArchSpec &arch);

  static llvm::StringRef GetPluginNameStatic() { return "sysv-mmix"; }
  llvm::StringRef GetPluginName() override { return GetPluginNameStatic(); }

  size_t GetRedZoneSize() const override { return 0; }

  bool PrepareTrivialCall(lldb_private::Thread &thread, lldb::addr_t sp,
                          lldb::addr_t function_address,
                          lldb::addr_t return_address,
                          llvm::ArrayRef<lldb::addr_t> args) const override;

  bool GetArgumentValues(lldb_private::Thread &thread,
                         lldb_private::ValueList &values) const override;

  lldb_private::Status
  SetReturnValueObject(lldb::StackFrameSP &frame_sp,
                       lldb::ValueObjectSP &new_value) override;

  lldb::UnwindPlanSP CreateFunctionEntryUnwindPlan() override;
  lldb::UnwindPlanSP CreateDefaultUnwindPlan() override;
  bool RegisterIsVolatile(const lldb_private::RegisterInfo *reg_info) override;

  bool CallFrameAddressIsValid(lldb::addr_t cfa) override;
  bool CodeAddressIsValid(lldb::addr_t pc) override;

  void AugmentRegisterInfo(
      std::vector<lldb_private::DynamicRegisterInfo::Register> &regs) override;

protected:
  std::pair<uint32_t, uint32_t>
  GetEHAndDWARFNums(llvm::StringRef name) override;
  uint32_t GetGenericNum(llvm::StringRef name) override;

  lldb::ValueObjectSP
  GetReturnValueObjectImpl(lldb_private::Thread &thread,
                           lldb_private::CompilerType &type) const override;

private:
  using lldb_private::MCBasedABI::MCBasedABI;
};

#endif // LLDB_SOURCE_PLUGINS_ABI_MMIX_ABISYSV_MMIX_H
