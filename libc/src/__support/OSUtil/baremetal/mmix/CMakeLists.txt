add_header_library(
  semihosting
  HDRS
    semihosting.h
  DEPENDS
    libc.src.__support.common
)
