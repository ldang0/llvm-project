add_lldb_unittest(ABIMMIXTests
  ABIMMIXTest.cpp

  LINK_COMPONENTS
    Support
    MMIX
  LINK_LIBS
    lldbTarget
    lldbPluginABIMMIX
  )
