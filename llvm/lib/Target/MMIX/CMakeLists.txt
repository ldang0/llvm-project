add_llvm_component_group(MMIX)

set(LLVM_TARGET_DEFINITIONS MMIX.td)

tablegen(LLVM MMIXGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(MMIXCommonTableGen)

add_llvm_target(MMIXCodeGen
  MMIXSubtarget.cpp
  MMIXTargetMachine.cpp

  LINK_COMPONENTS
  MMIXDesc
  MMIXInfo
  Support
  Target

  ADD_TO_COMPONENT
  MMIX
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
