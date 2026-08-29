add_object_library(
  baremetal_util
  SRCS
    io.cpp
    exit.cpp
  HDRS
    io.h
  DEPENDS
    libc.src.__support.common
    libc.src.__support.CPP.string_view
)

if(LIBC_TARGET_ARCHITECTURE_IS_MMIX)
  add_subdirectory(mmix)
endif()
