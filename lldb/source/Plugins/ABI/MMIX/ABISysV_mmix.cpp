//===-- ABISysV_mmix.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABISysV_mmix.h"

#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Value.h"
#include "lldb/Symbol/TypeSystem.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/ValueObject/ValueObjectConstResult.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/bit.h"
#include "llvm/Support/Endian.h"
#include "llvm/TargetParser/Triple.h"

#include <algorithm>

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
      .Case("r231", LLDB_REGNUM_GENERIC_ARG1)
      .Case("r232", LLDB_REGNUM_GENERIC_ARG2)
      .Case("r233", LLDB_REGNUM_GENERIC_ARG3)
      .Case("r234", LLDB_REGNUM_GENERIC_ARG4)
      .Case("r235", LLDB_REGNUM_GENERIC_ARG5)
      .Case("r236", LLDB_REGNUM_GENERIC_ARG6)
      .Case("r237", LLDB_REGNUM_GENERIC_ARG7)
      .Case("r238", LLDB_REGNUM_GENERIC_ARG8)
      .Default(LLDB_INVALID_REGNUM);
}

bool ABISysV_mmix::PrepareTrivialCall(Thread &, addr_t, addr_t, addr_t,
                                      llvm::ArrayRef<addr_t>) const {
  return false;
}

static const RegisterInfo *getMMIXArgumentRegister(RegisterContext &reg_ctx,
                                                   unsigned slot) {
  if (slot >= 16)
    return nullptr;
  return reg_ctx.GetRegisterInfoByName(
      ("$" + std::to_string(231 + slot)).c_str());
}

static bool readRegister(RegisterContext &reg_ctx, const RegisterInfo *reg_info,
                         uint64_t &value) {
  if (!reg_info)
    return false;
  RegisterValue reg_value;
  Scalar scalar;
  if (!reg_ctx.ReadRegister(reg_info, reg_value) ||
      !reg_value.GetScalarValue(scalar))
    return false;
  value = scalar.ULongLong();
  return true;
}

static bool readMMIXSlot(Thread &thread, RegisterContext &reg_ctx,
                         unsigned slot, uint64_t &value) {
  if (slot < 16)
    return readRegister(reg_ctx, getMMIXArgumentRegister(reg_ctx, slot), value);

  const RegisterInfo *sp_info =
      reg_ctx.GetRegisterInfo(eRegisterKindGeneric, LLDB_REGNUM_GENERIC_SP);
  uint64_t sp;
  if (!readRegister(reg_ctx, sp_info, sp))
    return false;

  Status error;
  Scalar scalar;
  ProcessSP process = thread.GetProcess();
  if (!process || !process->ReadScalarIntegerFromMemory(sp + 8 * (slot - 16), 8,
                                                        false, scalar, error))
    return false;
  value = scalar.ULongLong();
  return true;
}

static void setMMIXInteger(Scalar &scalar, uint64_t value, uint64_t byte_size,
                           bool is_signed) {
  switch (byte_size) {
  case 1:
    scalar = is_signed ? Scalar(int8_t(value)) : Scalar(uint8_t(value));
    break;
  case 2:
    scalar = is_signed ? Scalar(int16_t(value)) : Scalar(uint16_t(value));
    break;
  case 4:
    scalar = is_signed ? Scalar(int32_t(value)) : Scalar(uint32_t(value));
    break;
  default:
    scalar = is_signed ? Scalar(int64_t(value)) : Scalar(value);
    break;
  }
}

static void setMMIXDirectBytes(Value &value, uint64_t raw, uint64_t byte_size) {
  uint8_t bytes[8];
  llvm::support::endian::write64be(bytes, raw);
  value.SetBytes(bytes + 8 - byte_size, byte_size);
}

static bool readMMIXObject(Thread &thread, uint64_t address, uint64_t byte_size,
                           Value &value) {
  if (byte_size == 0) {
    value.SetBytes(nullptr, 0);
    return true;
  }
  std::vector<uint8_t> bytes(byte_size);
  Status error;
  ProcessSP process = thread.GetProcess();
  if (!process ||
      process->ReadMemory(address, bytes.data(), byte_size, error) != byte_size)
    return false;
  value.SetBytes(bytes.data(), byte_size);
  return true;
}

static bool setMMIXScalarValue(Value &value, const CompilerType &type,
                               uint64_t raw, uint64_t byte_size) {
  bool is_signed = false;
  if (type.IsIntegerOrEnumerationType(is_signed)) {
    if (byte_size == 0 || byte_size > 8)
      return false;
    setMMIXInteger(value.GetScalar(), raw, byte_size, is_signed);
  } else if (type.IsPointerOrReferenceType()) {
    if (byte_size != 8)
      return false;
    value.GetScalar() = raw;
  } else if (type.IsRealFloatingPointType()) {
    if (byte_size == 4)
      value.GetScalar() = llvm::bit_cast<float>(uint32_t(raw));
    else if (byte_size == 8)
      value.GetScalar() = llvm::bit_cast<double>(raw);
    else
      return false;
  } else {
    return false;
  }
  value.SetValueType(Value::ValueType::Scalar);
  return true;
}

static bool canPassMMIXAggregateDirectly(const CompilerType &type) {
  auto type_system = type.GetTypeSystem();
  return type_system && type_system->CanPassInRegisters(type);
}

enum class MMIXRecordModeKind { Scalar, Block, AlignmentOnlyBlock };

struct MMIXRecordMode {
  MMIXRecordModeKind kind;
  uint64_t size_in_bits;
  uint64_t align_in_bits;
  bool is_integer;
};

static MMIXRecordMode getMMIXIntegerMode(uint64_t size_in_bits) {
  switch (size_in_bits) {
  case 8:
  case 16:
  case 32:
  case 64:
  case 128:
    return {MMIXRecordModeKind::Scalar, size_in_bits,
            std::min(size_in_bits, uint64_t(64)), true};
  default:
    return {MMIXRecordModeKind::Block, size_in_bits, 0, false};
  }
}

// Mirror the frozen GNU record-mode rule used by Clang so LLDB can distinguish
// direct aggregate results from results returned through $251 and $231.
static MMIXRecordMode getMMIXRecordMode(const CompilerType &input,
                                        ExecutionContextScope *scope) {
  CompilerType type = input.GetCanonicalType();
  std::optional<uint64_t> bit_size =
      llvm::expectedToOptional(type.GetBitSize(scope));
  std::optional<size_t> bit_align = type.GetTypeBitAlign(scope);
  if (!bit_size || !bit_align)
    return {MMIXRecordModeKind::Block, 0, 0, false};

  CompilerType element_type;
  uint64_t element_count = 0;
  bool incomplete = false;
  if (type.IsArrayType(&element_type, &element_count, &incomplete)) {
    if (incomplete)
      return {MMIXRecordModeKind::Block, *bit_size, 0, false};
    if (element_count == 1) {
      MMIXRecordMode element_mode = getMMIXRecordMode(element_type, scope);
      if (element_mode.kind == MMIXRecordModeKind::Scalar)
        return element_mode;
      return {MMIXRecordModeKind::Block, *bit_size, 0, false};
    }
    return getMMIXIntegerMode(*bit_size);
  }

  if (type.IsAggregateType()) {
    MMIXRecordMode whole_field = {MMIXRecordModeKind::Block, 0, 0, false};
    const bool is_union = type.GetTypeClass() == eTypeClassUnion;
    for (uint32_t index = 0; index != type.GetNumFields(); ++index) {
      std::string name;
      uint64_t bit_offset = 0;
      uint32_t bitfield_size = 0;
      bool is_bitfield = false;
      CompilerType field = type.GetFieldAtIndex(index, name, &bit_offset,
                                                &bitfield_size, &is_bitfield);
      std::optional<uint64_t> field_bit_size =
          llvm::expectedToOptional(field.GetBitSize(scope));
      if (!field || !field_bit_size)
        return {MMIXRecordModeKind::Block, *bit_size, 0, false};
      uint64_t effective_size = is_bitfield ? bitfield_size : *field_bit_size;
      if (effective_size == 0)
        continue;

      MMIXRecordMode field_mode = getMMIXRecordMode(field, scope);
      if (field_mode.kind == MMIXRecordModeKind::Block)
        return {MMIXRecordModeKind::Block, *bit_size, 0, false};
      if (effective_size == *bit_size &&
          field_mode.kind == MMIXRecordModeKind::Scalar &&
          (!is_union || field_mode.is_integer) &&
          field_mode.size_in_bits > whole_field.size_in_bits)
        whole_field = field_mode;
    }

    MMIXRecordMode mode = whole_field.kind == MMIXRecordModeKind::Scalar
                              ? whole_field
                              : getMMIXIntegerMode(*bit_size);
    if (mode.kind == MMIXRecordModeKind::Block)
      return mode;
    if (*bit_align < mode.align_in_bits)
      return {MMIXRecordModeKind::AlignmentOnlyBlock, *bit_size, 0, false};
    return mode;
  }

  const uint32_t type_info = type.GetTypeInfo();
  return {MMIXRecordModeKind::Scalar, *bit_size,
          std::min(*bit_size, uint64_t(64)),
          (type_info & (eTypeIsInteger | eTypeIsPointer)) != 0};
}

bool ABISysV_mmix::GetArgumentValues(Thread &thread, ValueList &values) const {
  RegisterContextSP reg_ctx = thread.GetRegisterContext();
  if (!reg_ctx)
    return false;

  unsigned slot = 0;
  for (unsigned index = 0; index != values.GetSize(); ++index) {
    Value *value = values.GetValueAtIndex(index);
    if (!value)
      return false;
    CompilerType type = value->GetCompilerType();
    std::optional<uint64_t> byte_size =
        llvm::expectedToOptional(type.GetByteSize(&thread));
    std::optional<size_t> bit_align = type.GetTypeBitAlign(&thread);
    if (!type || !byte_size || !bit_align || *bit_align > 64)
      return false;

    if (type.IsAggregateType() && !type.IsVectorType() && *byte_size == 0) {
      value->SetBytes(nullptr, 0);
      continue;
    }

    uint64_t raw;
    if (!readMMIXSlot(thread, *reg_ctx, slot++, raw))
      return false;

    if (type.IsComplexType() || type.IsVectorType()) {
      if (*byte_size <= 8)
        setMMIXDirectBytes(*value, raw, *byte_size);
      else if (!readMMIXObject(thread, raw, *byte_size, *value))
        return false;
      continue;
    }

    if (type.IsAggregateType()) {
      if (*byte_size <= 8 && canPassMMIXAggregateDirectly(type))
        setMMIXDirectBytes(*value, raw, *byte_size);
      else if (!readMMIXObject(thread, raw, *byte_size, *value))
        return false;
      continue;
    }

    if (!setMMIXScalarValue(*value, type, raw, *byte_size))
      return false;
  }
  return true;
}

Status ABISysV_mmix::SetReturnValueObject(StackFrameSP &, ValueObjectSP &) {
  return Status::FromErrorString("setting MMIX return values is not supported");
}

static ValueObjectSP makeMMIXObjectValue(Thread &thread, CompilerType &type,
                                         llvm::ArrayRef<uint8_t> bytes) {
  WritableDataBufferSP buffer(new DataBufferHeap(bytes.data(), bytes.size()));
  DataExtractor data(buffer, eByteOrderBig, 8);
  return ValueObjectConstResult::Create(&thread, type, ConstString(""), data);
}

ValueObjectSP ABISysV_mmix::GetReturnValueObjectImpl(Thread &thread,
                                                     CompilerType &type) const {
  RegisterContextSP reg_ctx = thread.GetRegisterContext();
  std::optional<uint64_t> byte_size =
      llvm::expectedToOptional(type.GetByteSize(&thread));
  if (!reg_ctx || !type || !byte_size)
    return {};

  if (type.IsAggregateType() && !type.IsVectorType() && *byte_size == 0)
    return makeMMIXObjectValue(thread, type, {});

  uint64_t first;
  if (!readRegister(*reg_ctx, getMMIXArgumentRegister(*reg_ctx, 0), first))
    return {};

  if (type.IsComplexType() && *byte_size == 16) {
    uint64_t second;
    if (!readRegister(*reg_ctx, getMMIXArgumentRegister(*reg_ctx, 1), second))
      return {};
    uint8_t bytes[16];
    llvm::support::endian::write64be(bytes, first);
    llvm::support::endian::write64be(bytes + 8, second);
    return makeMMIXObjectValue(thread, type, bytes);
  }

  if (type.IsAggregateType()) {
    std::optional<size_t> bit_align = type.GetTypeBitAlign(&thread);
    if (!bit_align || *bit_align > 64)
      return {};
    MMIXRecordMode mode = getMMIXRecordMode(type, &thread);
    if (mode.kind == MMIXRecordModeKind::Scalar && mode.size_in_bits <= 64) {
      uint8_t bytes[8];
      llvm::support::endian::write64be(bytes, first);
      return makeMMIXObjectValue(
          thread, type, llvm::ArrayRef(bytes + 8 - *byte_size, *byte_size));
    }

    std::vector<uint8_t> bytes(*byte_size);
    Status error;
    ProcessSP process = thread.GetProcess();
    if (!process || process->ReadMemory(first, bytes.data(), bytes.size(),
                                        error) != bytes.size())
      return {};
    return makeMMIXObjectValue(thread, type, bytes);
  }

  if (type.IsVectorType() || type.IsComplexType()) {
    if (*byte_size > 8)
      return {};
    uint8_t bytes[8];
    llvm::support::endian::write64be(bytes, first);
    return makeMMIXObjectValue(
        thread, type, llvm::ArrayRef(bytes + 8 - *byte_size, *byte_size));
  }

  Value value;
  value.SetCompilerType(type);
  if (!setMMIXScalarValue(value, type, first, *byte_size))
    return {};
  return ValueObjectConstResult::Create(thread.GetStackFrameAtIndex(0).get(),
                                        value, ConstString(""));
}

UnwindPlanSP ABISysV_mmix::CreateFunctionEntryUnwindPlan() {
  UnwindPlan::Row row;
  row.GetCFAValue().SetIsRegisterPlusOffset(30, 0);
  row.SetRegisterLocationToRegister(304, 35, true);

  auto plan = std::make_shared<UnwindPlan>(eRegisterKindDWARF);
  plan->AppendRow(std::move(row));
  plan->SetSourceName("MMIX function-entry unwind plan");
  plan->SetSourcedFromCompiler(eLazyBoolNo);
  plan->SetUnwindPlanForSignalTrap(eLazyBoolNo);
  return plan;
}

UnwindPlanSP ABISysV_mmix::CreateDefaultUnwindPlan() { return {}; }

bool ABISysV_mmix::RegisterIsVolatile(const RegisterInfo *reg_info) {
  if (!reg_info || !reg_info->name)
    return true;

  llvm::StringRef name(reg_info->name);
  llvm::StringRef number = name;
  if (number.consume_front("$")) {
    unsigned reg;
    if (llvm::to_integer(number, reg, 10) && number == std::to_string(reg))
      return !(reg <= 30 || reg == 253 || reg == 254);
  }
  return name != "rG" && name != "rO";
}

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
