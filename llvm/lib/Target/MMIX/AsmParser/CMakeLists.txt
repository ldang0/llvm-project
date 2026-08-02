add_llvm_component_library(LLVMMMIXAsmParser
  MMIXAsmParser.cpp

  LINK_COMPONENTS
  MC
  MMIXDesc
  MMIXInfo
  Support
  MCParser
  TargetParser

  ADD_TO_COMPONENT
  MMIX
  )
