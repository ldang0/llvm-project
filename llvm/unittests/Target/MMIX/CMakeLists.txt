include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Target/MMIX
  ${LLVM_BINARY_DIR}/lib/Target/MMIX
  )

set(LLVM_LINK_COMPONENTS
  MC
  MMIXDesc
  MMIXInfo
  Support
  TargetParser
  )

add_llvm_target_unittest(MMIXTests
  MMIXALInstPrinterTest.cpp
  )
