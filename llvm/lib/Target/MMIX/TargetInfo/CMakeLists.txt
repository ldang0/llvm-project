add_llvm_component_library(LLVMMMIXInfo
  MMIXTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  MMIX
  )
