//===-- MMIXALModuleValidator.cpp - Validate MMIXAL modules --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MMIXALModuleValidator.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalIFunc.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

StringRef getMMIXALLinkageName(GlobalValue::LinkageTypes Linkage) {
  switch (Linkage) {
  case GlobalValue::ExternalLinkage:
    return "external";
  case GlobalValue::AvailableExternallyLinkage:
    return "available_externally";
  case GlobalValue::LinkOnceAnyLinkage:
    return "linkonce";
  case GlobalValue::LinkOnceODRLinkage:
    return "linkonce_odr";
  case GlobalValue::WeakAnyLinkage:
    return "weak";
  case GlobalValue::WeakODRLinkage:
    return "weak_odr";
  case GlobalValue::AppendingLinkage:
    return "appending";
  case GlobalValue::InternalLinkage:
    return "internal";
  case GlobalValue::PrivateLinkage:
    return "private";
  case GlobalValue::ExternalWeakLinkage:
    return "extern_weak";
  case GlobalValue::CommonLinkage:
    return "common";
  }
  llvm_unreachable("unknown LLVM linkage");
}

bool isMMIXALRuntimeRegistrationSection(StringRef Section) {
  for (StringRef Prefix :
       {".init_array", ".fini_array", ".preinit_array", ".ctors", ".dtors"})
    if (Section.starts_with(Prefix))
      return true;
  return false;
}

bool containsNonzeroAddressSpace(const Type *Ty,
                                 SmallPtrSetImpl<const Type *> &Visited) {
  if (const auto *Pointer = dyn_cast<PointerType>(Ty))
    return Pointer->getAddressSpace() != 0;
  if (!Visited.insert(Ty).second)
    return false;

  if (const auto *Function = dyn_cast<FunctionType>(Ty)) {
    if (containsNonzeroAddressSpace(Function->getReturnType(), Visited))
      return true;
    for (const Type *Param : Function->params())
      if (containsNonzeroAddressSpace(Param, Visited))
        return true;
    return false;
  }
  if (const auto *Struct = dyn_cast<StructType>(Ty)) {
    for (const Type *Element : Struct->elements())
      if (containsNonzeroAddressSpace(Element, Visited))
        return true;
    return false;
  }
  if (const auto *Array = dyn_cast<ArrayType>(Ty))
    return containsNonzeroAddressSpace(Array->getElementType(), Visited);
  if (const auto *Vector = dyn_cast<VectorType>(Ty))
    return containsNonzeroAddressSpace(Vector->getElementType(), Visited);
  return false;
}

bool containsNonzeroAddressSpace(const Type *Ty) {
  SmallPtrSet<const Type *, 8> Visited;
  return containsNonzeroAddressSpace(Ty, Visited);
}

bool constantUsesNonzeroAddressSpace(
    const Constant &C, SmallPtrSetImpl<const Constant *> &Visited) {
  if (containsNonzeroAddressSpace(C.getType()))
    return true;
  if (!Visited.insert(&C).second || isa<GlobalValue>(C))
    return false;
  for (const Value *Operand : C.operands())
    if (const auto *Nested = dyn_cast<Constant>(Operand);
        Nested && constantUsesNonzeroAddressSpace(*Nested, Visited))
      return true;
  return false;
}

bool constantUsesNonzeroAddressSpace(const Constant &C) {
  SmallPtrSet<const Constant *, 8> Visited;
  return constantUsesNonzeroAddressSpace(C, Visited);
}

bool globalValueUsesNonzeroAddressSpace(const GlobalValue &GV) {
  if (containsNonzeroAddressSpace(GV.getType()) ||
      containsNonzeroAddressSpace(GV.getValueType()))
    return true;
  if (const auto *Global = dyn_cast<GlobalVariable>(&GV))
    return Global->hasInitializer() &&
           constantUsesNonzeroAddressSpace(*Global->getInitializer());
  if (const auto *Alias = dyn_cast<GlobalAlias>(&GV))
    return constantUsesNonzeroAddressSpace(*Alias->getAliasee());
  return false;
}

bool instructionUsesNonzeroAddressSpace(const Instruction &I) {
  if (containsNonzeroAddressSpace(I.getType()))
    return true;
  if (const auto *Alloca = dyn_cast<AllocaInst>(&I);
      Alloca && containsNonzeroAddressSpace(Alloca->getAllocatedType()))
    return true;
  if (const auto *GEP = dyn_cast<GetElementPtrInst>(&I);
      GEP && containsNonzeroAddressSpace(GEP->getSourceElementType()))
    return true;
  for (const Value *Operand : I.operands()) {
    if (containsNonzeroAddressSpace(Operand->getType()))
      return true;
    if (const auto *C = dyn_cast<Constant>(Operand);
        C && constantUsesNonzeroAddressSpace(*C))
      return true;
  }
  return false;
}

StringRef getMMIXALRuntimeInstrumentationAttribute(const Function &F) {
  struct RuntimeAttribute {
    Attribute::AttrKind Kind;
    const char *Name;
  };
  static constexpr RuntimeAttribute Attributes[] = {
      {Attribute::SanitizeAddress, "sanitize_address"},
      {Attribute::SanitizeThread, "sanitize_thread"},
      {Attribute::SanitizeType, "sanitize_type"},
      {Attribute::SanitizeMemory, "sanitize_memory"},
      {Attribute::SanitizeHWAddress, "sanitize_hwaddress"},
      {Attribute::SanitizeMemTag, "sanitize_memtag"},
      {Attribute::SanitizeNumericalStability, "sanitize_numerical_stability"},
      {Attribute::SanitizeRealtime, "sanitize_realtime"},
      {Attribute::SanitizeRealtimeBlocking, "sanitize_realtime_blocking"},
      {Attribute::SanitizeAllocToken, "sanitize_alloc_token"},
  };
  for (const RuntimeAttribute &Attribute : Attributes)
    if (F.hasFnAttribute(Attribute.Kind))
      return Attribute.Name;
  return {};
}

bool isMMIXALRuntimeMetadataIntrinsic(Intrinsic::ID ID) {
  switch (ID) {
  case Intrinsic::experimental_stackmap:
  case Intrinsic::experimental_patchpoint_void:
  case Intrinsic::experimental_patchpoint:
  case Intrinsic::experimental_gc_statepoint:
  case Intrinsic::experimental_gc_result:
  case Intrinsic::experimental_gc_relocate:
  case Intrinsic::gcroot:
  case Intrinsic::gcread:
  case Intrinsic::gcwrite:
  case Intrinsic::instrprof_cover:
  case Intrinsic::instrprof_increment:
  case Intrinsic::instrprof_increment_step:
  case Intrinsic::instrprof_callsite:
  case Intrinsic::instrprof_timestamp:
  case Intrinsic::instrprof_value_profile:
  case Intrinsic::instrprof_mcdc_parameters:
  case Intrinsic::instrprof_mcdc_tvbitmap_update:
  case Intrinsic::pseudoprobe:
  case Intrinsic::xray_customevent:
  case Intrinsic::xray_typedevent:
    return true;
  default:
    return false;
  }
}

bool isMMIXALExceptionInstruction(const Instruction &I) {
  return isa<InvokeInst, ResumeInst, CatchReturnInst, CleanupReturnInst>(I) ||
         I.isEHPad();
}

Error validateMMIXALSymbolSemantics(const GlobalValue &GV) {
  if (isa<GlobalIFunc>(GV))
    return createStringError(
        Twine("MMIXAL output variant 1 does not support GlobalIFunc '") +
        GV.getName() + "'");

  switch (GV.getLinkage()) {
  case GlobalValue::ExternalLinkage:
  case GlobalValue::InternalLinkage:
  case GlobalValue::PrivateLinkage:
    break;
  default:
    return createStringError(
        Twine("MMIXAL output variant 1 does not support linkage '") +
        getMMIXALLinkageName(GV.getLinkage()) + "' for symbol '" +
        GV.getName() + "'");
  }

  if (GV.hasComdat())
    return createStringError(
        Twine(
            "MMIXAL output variant 1 does not support COMDAT membership for ") +
        "symbol '" + GV.getName() + "'");

  if (!GV.hasDefaultVisibility()) {
    StringRef Visibility = GV.hasHiddenVisibility() ? "hidden" : "protected";
    return createStringError(
        Twine("MMIXAL output variant 1 does not support ") + Visibility +
        " visibility for symbol '" + GV.getName() + "'");
  }

  if (GV.getDLLStorageClass() != GlobalValue::DefaultStorageClass) {
    StringRef Storage =
        GV.hasDLLImportStorageClass() ? "dllimport" : "dllexport";
    return createStringError(
        Twine("MMIXAL output variant 1 does not support ") + Storage +
        " storage for symbol '" + GV.getName() + "'");
  }

  if (GV.hasPartition())
    return createStringError(
        Twine("MMIXAL output variant 1 does not support partition '") +
        GV.getPartition() + "' for symbol '" + GV.getName() + "'");

  return Error::success();
}

class MMIXALModuleValidatorLegacy final : public ModulePass {
public:
  static char ID;

  MMIXALModuleValidatorLegacy() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    Expected<const Function *> Entry = validateMMIXALModule(M);
    if (!Entry) {
      std::string Message = toString(Entry.takeError());
      reportFatalUsageError(StringRef(Message));
    }
    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }
};

} // namespace

char MMIXALModuleValidatorLegacy::ID = 0;

ModulePass *llvm::createMMIXALModuleValidatorPass() {
  return new MMIXALModuleValidatorLegacy();
}

Expected<const Function *> llvm::validateMMIXALRawEntry(const Module &M) {
  const GlobalValue *Main = M.getNamedValue("Main");
  if (!Main)
    return createStringError(
        "MMIXAL bare-metal module has no entry named 'Main'");

  const auto *Entry = dyn_cast<Function>(Main);
  if (!Entry)
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must be a function definition");
  if (Entry->isDeclaration())
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must be a definition");
  if (!Entry->isStrongDefinitionForLinker())
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must be a strong definition");
  if (Entry->getCallingConv() != CallingConv::C)
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must use the C calling convention");

  const FunctionType *EntryType = Entry->getFunctionType();
  if (EntryType->isVarArg())
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must not be variadic");
  if (!EntryType->getReturnType()->isVoidTy() || EntryType->getNumParams() != 0)
    return createStringError(
        "MMIXAL bare-metal entry 'Main' must have type 'void ()'");

  for (const BasicBlock *BB : depth_first(&Entry->getEntryBlock()))
    if (isa<ReturnInst>(BB->getTerminator()))
      return createStringError(
          "MMIXAL bare-metal entry 'Main' must not have a reachable return");

  return Entry;
}

Expected<const Function *> llvm::validateMMIXALModule(const Module &M) {
  Expected<const Function *> Entry = validateMMIXALRawEntry(M);
  if (!Entry)
    return Entry.takeError();

  for (const Module::GlobalAsmFragment &Fragment : M.getModuleInlineAsm())
    if (!StringRef(Fragment.Asm).trim().empty())
      return createStringError(
          "MMIXAL output variant 1 does not support module-level inline "
          "assembly");

  for (const Function &F : M)
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB)
        if (const auto *Call = dyn_cast<CallBase>(&I);
            Call && Call->isInlineAsm())
          return createStringError(
              Twine("MMIXAL output variant 1 does not support inline assembly "
                    "in function '") +
              F.getName() + "'");

  if (const NamedMDNode *Symvers = M.getNamedMetadata("symvers");
      Symvers && Symvers->getNumOperands() != 0) {
    const MDNode *Version = Symvers->getOperand(0);
    const auto *Symbol = Version->getNumOperands() == 0
                             ? nullptr
                             : dyn_cast<MDString>(Version->getOperand(0));
    if (Symbol)
      return createStringError(
          Twine(
              "MMIXAL output variant 1 does not support ELF symbol version ") +
          "for '" + Symbol->getString() + "'");
    return createStringError(
        "MMIXAL output variant 1 does not support ELF symbol-version metadata");
  }

  for (const GlobalVariable &Global : M.globals()) {
    if (Global.getName() == "llvm.global_ctors" ||
        Global.getName() == "llvm.global_dtors")
      return createStringError(Twine("MMIXAL output variant 1 does not support "
                                     "runtime registration ") +
                               "symbol '" + Global.getName() + "'");
    if (Global.hasSection() &&
        isMMIXALRuntimeRegistrationSection(Global.getSection()))
      return createStringError(Twine("MMIXAL output variant 1 does not support "
                                     "runtime registration ") +
                               "section '" + Global.getSection() +
                               "' for symbol '" + Global.getName() + "'");
  }

  for (const Function &F : M) {
    if (F.isIntrinsic())
      continue;
    if (F.hasPersonalityFn())
      return createStringError(
          Twine("MMIXAL output variant 1 does not support an exception ") +
          "personality in function '" + F.getName() + "'");
    if (F.hasUWTable())
      return createStringError(
          Twine("MMIXAL output variant 1 does not support unwind-table ") +
          "generation in function '" + F.getName() + "'");
    if (F.hasGC())
      return createStringError(
          Twine(
              "MMIXAL output variant 1 does not support garbage-collection ") +
          "strategy '" + F.getGC() + "' in function '" + F.getName() + "'");
    if (StringRef Attribute = getMMIXALRuntimeInstrumentationAttribute(F);
        !Attribute.empty())
      return createStringError(
          Twine("MMIXAL output variant 1 does not support runtime ") +
          "instrumentation attribute '" + Attribute + "' in function '" +
          F.getName() + "'");
  }

  for (const GlobalValue &GV : M.global_values()) {
    if (const auto *F = dyn_cast<Function>(&GV); F && F->isIntrinsic())
      continue;
    if (const auto *Global = dyn_cast<GlobalVariable>(&GV);
        Global && Global->isThreadLocal())
      return createStringError(
          Twine(
              "MMIXAL output variant 1 does not support thread-local symbol ") +
          "'" + GV.getName() + "'");
    if (globalValueUsesNonzeroAddressSpace(GV))
      return createStringError(
          Twine("MMIXAL output variant 1 does not support nonzero address ") +
          "space in symbol '" + GV.getName() + "'");
    if (GV.isTagged())
      return createStringError(
          Twine("MMIXAL output variant 1 does not support sanitizer ") +
          "allocation metadata for symbol '" + GV.getName() + "'");
    if (Error Err = validateMMIXALSymbolSemantics(GV))
      return std::move(Err);
    if (!GV.isDeclaration() || GV.use_empty())
      continue;
    return createStringError(
        Twine("MMIXAL output variant 1 cannot resolve referenced symbol '") +
        GV.getName() + "'");
  }

  for (const Function &F : M)
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB) {
        if (isMMIXALExceptionInstruction(I))
          return createStringError(
              Twine("MMIXAL output variant 1 does not support exception ") +
              "instruction '" + I.getOpcodeName() + "' in function '" +
              F.getName() + "'");
        if (const auto *Call = dyn_cast<CallBase>(&I))
          if (const Function *Callee = Call->getCalledFunction()) {
            Intrinsic::ID ID = Callee->getIntrinsicID();
            if (ID == Intrinsic::thread_pointer ||
                ID == Intrinsic::threadlocal_address) {
              StringRef Name = ID == Intrinsic::thread_pointer
                                   ? "llvm.thread.pointer"
                                   : "llvm.threadlocal.address";
              return createStringError(
                  Twine("MMIXAL output variant 1 does not support TLS ") +
                  "intrinsic '" + Name + "' in function '" + F.getName() + "'");
            }
            if (isMMIXALRuntimeMetadataIntrinsic(ID))
              return createStringError(
                  Twine("MMIXAL output variant 1 does not support runtime ") +
                  "metadata intrinsic '" + Intrinsic::getBaseName(ID) +
                  "' in function '" + F.getName() + "'");
          }
        if (instructionUsesNonzeroAddressSpace(I))
          return createStringError(
              Twine(
                  "MMIXAL output variant 1 does not support nonzero address ") +
              "space in instruction '" + I.getOpcodeName() + "' in function '" +
              F.getName() + "'");
      }

  return *Entry;
}
