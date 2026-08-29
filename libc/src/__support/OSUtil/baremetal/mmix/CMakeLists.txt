add_header_library(
  semihosting
  HDRS
    semihosting.h
  DEPENDS
    libc.src.__support.common
)

add_entrypoint_object(
  platform_io
  SRCS
    platform_io.cpp
  HDRS
    platform_io.h
  DEPENDS
    .semihosting
    libc.hdr.errno_macros
    libc.src.__support.OSUtil.baremetal.baremetal_util
    libc.src.__support.common
)

add_entrypoint_object(
  atexit
  SRCS
    atexit.cpp
  DEPENDS
    libc.hdr.types.atexithandler_t
    libc.src.__support.common
)

add_entrypoint_object(
  termination
  SRCS
    termination.cpp
  DEPENDS
    .semihosting
)

add_entrypoint_object(
  abort
  SRCS
    abort.cpp
  DEPENDS
    .semihosting
)
