add_header_library(
  abort_utils
  HDRS
    abort_utils.h
  DEPENDS
    libc.src.__support.OSUtil.baremetal.mmix.abort
)

add_entrypoint_object(
  process_utilities
  SRCS
    process_utilities.cpp
  DEPENDS
    libc.hdr.types.atexithandler_t
    libc.src.__support.OSUtil.osutil
    libc.src.__support.common
    libc.include.stdlib
)
