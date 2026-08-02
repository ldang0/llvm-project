add_llvm_component_library(LLVMMMIXDesc
  MMIXAsmBackend.cpp
  MMIXMCCodeEmitter.cpp
  MMIXMCAsmInfo.cpp
  MMIXMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  MMIXInfo
  Support
  TargetParser

  ADD_TO_COMPONENT
  MMIX
  )
