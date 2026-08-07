add_llvm_component_library(LLVMMMIXDesc
  MMIXAsmBackend.cpp
  MMIXMCCodeEmitter.cpp
  MMIXMCAsmInfo.cpp
  MMIXInstPrinter.cpp
  MMIXALInstPrinter.cpp
  MMIXMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  MMIXInfo
  Support
  TargetParser

  ADD_TO_COMPONENT
  MMIX
  )
