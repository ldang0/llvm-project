//===-- ABIMMIXTest.cpp --------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/ABI/MMIX/ABISysV_mmix.h"
#include "lldb/Target/DynamicRegisterInfo.h"
#include "lldb/Utility/ArchSpec.h"
#include "llvm/Support/TargetSelect.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;

namespace {

using Register = DynamicRegisterInfo::Register;

class ABIMMIXTest : public testing::Test {
public:
  static void SetUpTestSuite() {
    LLVMInitializeMMIXTargetInfo();
    LLVMInitializeMMIXTargetMC();
    ABISysV_mmix::Initialize();
  }

  static void TearDownTestSuite() { ABISysV_mmix::Terminate(); }

protected:
  static Register makeRegister(llvm::StringRef name, uint32_t remote) {
    Register reg;
    reg.name = ConstString(name);
    reg.set_name = ConstString("General Purpose Registers");
    reg.byte_size = 8;
    reg.encoding = eEncodingUint;
    reg.format = eFormatHex;
    reg.regnum_remote = remote;
    return reg;
  }

  static ABISP createABI() {
    return ABI::FindPlugin(ProcessSP(), ArchSpec("mmix-unknown-elf"));
  }
};

TEST_F(ABIMMIXTest, AugmentsCompleteRemoteRegisterSet) {
  ABISP abi = createABI();
  ASSERT_TRUE(abi);

  std::vector<Register> regs;
  for (unsigned reg = 0; reg != 256; ++reg)
    regs.push_back(makeRegister("r" + std::to_string(reg), reg));
  regs[253].alt_name = ConstString("fp");
  regs[254].alt_name = ConstString("sp");

  static constexpr const char *SpecialNames[] = {
      "rB", "rD", "rE", "rH",  "rJ", "rM", "rR",  "rBB", "rC",  "rN", "rO",
      "rS", "rI", "rT", "rTT", "rK", "rQ", "rU",  "rV",  "rG",  "rL", "rA",
      "rF", "rP", "rW", "rX",  "rY", "rZ", "rWW", "rXX", "rYY", "rZZ"};
  static constexpr uint32_t SpecialDWARF[] = {
      272, 32,  33,  34,  35,  277, 36,  279, 280, 281, 38,
      283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293,
      294, 295, 296, 297, 298, 299, 300, 301, 302, 303};
  static_assert(std::size(SpecialNames) == std::size(SpecialDWARF));
  for (auto [index, name] : llvm::enumerate(SpecialNames))
    regs.push_back(makeRegister(name, 256 + index));

  Register pc = makeRegister("pc", 288);
  pc.encoding = eEncodingUint;
  pc.format = eFormatAddressInfo;
  regs.push_back(pc);

  abi->AugmentRegisterInfo(regs);

  ASSERT_EQ(regs.size(), 289U);
  for (unsigned reg = 0; reg != 256; ++reg) {
    EXPECT_EQ(regs[reg].name.GetStringRef(), "$" + std::to_string(reg));
    EXPECT_EQ(regs[reg].regnum_dwarf, reg >= 224 ? reg - 224 : reg + 48);
    EXPECT_EQ(regs[reg].regnum_ehframe, regs[reg].regnum_dwarf);
    EXPECT_EQ(regs[reg].regnum_remote, reg);
    EXPECT_EQ(regs[reg].byte_size, 8U);
  }
  EXPECT_EQ(regs[0].alt_name.GetStringRef(), "r0");
  EXPECT_EQ(regs[253].alt_name.GetStringRef(), "fp");
  EXPECT_EQ(regs[254].alt_name.GetStringRef(), "sp");
  for (unsigned index = 0; index != std::size(SpecialNames); ++index) {
    const Register &reg = regs[256 + index];
    EXPECT_EQ(reg.regnum_dwarf, SpecialDWARF[index]);
    EXPECT_EQ(reg.regnum_remote, 256U + index);
  }

  EXPECT_EQ(regs[253].regnum_generic,
            static_cast<uint32_t>(LLDB_REGNUM_GENERIC_FP));
  EXPECT_EQ(regs[254].regnum_generic,
            static_cast<uint32_t>(LLDB_REGNUM_GENERIC_SP));
  EXPECT_EQ(regs[260].regnum_generic,
            static_cast<uint32_t>(LLDB_REGNUM_GENERIC_RA));
  for (unsigned index = 0; index != 8; ++index)
    EXPECT_EQ(regs[231 + index].regnum_generic,
              static_cast<uint32_t>(LLDB_REGNUM_GENERIC_ARG1 + index));
  EXPECT_EQ(regs[239].regnum_generic, LLDB_INVALID_REGNUM);
  EXPECT_EQ(regs[288].regnum_dwarf, 304U);
  EXPECT_EQ(regs[288].regnum_generic,
            static_cast<uint32_t>(LLDB_REGNUM_GENERIC_PC));
  EXPECT_EQ(regs[288].regnum_remote, 288U);
}

TEST_F(ABIMMIXTest, ImplementsBoundedGNUABIRoles) {
  ABISP abi = createABI();
  ASSERT_TRUE(abi);

  auto is_volatile = [&](const char *name) {
    RegisterInfo info{};
    info.name = name;
    return abi->RegisterIsVolatile(&info);
  };
  for (const char *name : {"$0", "$30", "$253", "$254", "rG", "rO"})
    EXPECT_FALSE(is_volatile(name)) << name;
  for (const char *name :
       {"$31", "$32", "$230", "$231", "$252", "$255", "rJ", "rL", "rS", "rQ"})
    EXPECT_TRUE(is_volatile(name)) << name;

  EXPECT_EQ(abi->GetRedZoneSize(), 0U);
  EXPECT_TRUE(abi->CallFrameAddressIsValid(8));
  EXPECT_FALSE(abi->CallFrameAddressIsValid(0));
  EXPECT_FALSE(abi->CallFrameAddressIsValid(4));
  EXPECT_TRUE(abi->CodeAddressIsValid(0x100));
  EXPECT_FALSE(abi->CodeAddressIsValid(0x102));

  UnwindPlanSP plan = abi->CreateFunctionEntryUnwindPlan();
  ASSERT_TRUE(plan);
  EXPECT_EQ(plan->GetRegisterKind(), eRegisterKindDWARF);
  const UnwindPlan::Row *row = plan->GetRowAtIndex(0);
  ASSERT_NE(row, nullptr);
  EXPECT_EQ(row->GetCFAValue().GetRegisterNumber(), 30U);
  UnwindPlan::Row::AbstractRegisterLocation pc;
  ASSERT_TRUE(row->GetRegisterInfo(304, pc));
  EXPECT_TRUE(pc.IsInOtherRegister());
  EXPECT_EQ(pc.GetRegisterNumber(), 35U);
  EXPECT_FALSE(abi->CreateDefaultUnwindPlan());
}

TEST_F(ABIMMIXTest, DoesNotInventOrAliasRegisters) {
  ABISP abi = createABI();
  ASSERT_TRUE(abi);

  std::vector<Register> regs{
      makeRegister("r255", 255), makeRegister("reserved", 289),
      makeRegister("r256", 290), makeRegister("r0253", 291)};
  abi->AugmentRegisterInfo(regs);

  ASSERT_EQ(regs.size(), 4U);
  EXPECT_EQ(regs[0].name.GetStringRef(), "$255");
  EXPECT_EQ(regs[0].regnum_dwarf, 31U);
  for (unsigned index : {1U, 2U, 3U}) {
    EXPECT_NE(regs[index].name.GetStringRef(), "$253");
    EXPECT_EQ(regs[index].regnum_dwarf, LLDB_INVALID_REGNUM);
    EXPECT_EQ(regs[index].regnum_ehframe, LLDB_INVALID_REGNUM);
    EXPECT_EQ(regs[index].regnum_generic, LLDB_INVALID_REGNUM);
  }
}

} // namespace
