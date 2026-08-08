add_llvm_component_library(LLVMMMIXDesc
  MMIXALAsmStreamer.cpp
  MMIXALLayout.cpp
  MMIXALSymbolMapper.cpp
  MMIXALSymbolTable.cpp
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
