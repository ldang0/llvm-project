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
    libc.include.llvm-libc-types.size_t
    libc.include.llvm-libc-types.ssize_t
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

add_entrypoint_library(
  mmixplatform
  DEPENDS
    ${TARGET_LIBC_PLATFORM_ENTRYPOINTS}
)
set_target_properties(
  mmixplatform
  PROPERTIES
    ARCHIVE_OUTPUT_NAME mmixplatform
)
add_custom_target(libc-platform DEPENDS mmixplatform)
install(
  TARGETS mmixplatform
  ARCHIVE DESTINATION ${LIBC_INSTALL_LIBRARY_DIR}
  COMPONENT libc-platform-runtime
)
add_custom_target(
  install-libc-platform
  DEPENDS mmixplatform
  COMMAND "${CMAKE_COMMAND}"
          -DCMAKE_INSTALL_COMPONENT=libc-platform-runtime
          -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  COMMAND "${CMAKE_COMMAND}"
          -DCMAKE_INSTALL_COMPONENT=libc-platform
          -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
)
