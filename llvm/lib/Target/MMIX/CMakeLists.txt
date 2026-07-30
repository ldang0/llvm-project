add_llvm_component_group(MMIX)

add_llvm_target(MMIXCodeGen
  MMIXTarget.cpp

  LINK_COMPONENTS
  MMIXInfo
  Support
  Target

  ADD_TO_COMPONENT
  MMIX
  )

add_subdirectory(TargetInfo)
