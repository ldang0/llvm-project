add_lldb_library(lldbPluginABIMMIX PLUGIN
  ABISysV_mmix.cpp

  LINK_COMPONENTS
    Support
    TargetParser
  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
  )
