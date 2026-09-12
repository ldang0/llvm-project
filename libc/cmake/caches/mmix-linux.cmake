# Supply installed host compilers, LLVM utilities and LIBC_KERNEL_HEADERS.
# This cache builds Linux libc components, not an installed complete runtime.
if(NOT IS_ABSOLUTE "${LIBC_KERNEL_HEADERS}" OR
   NOT EXISTS "${LIBC_KERNEL_HEADERS}/asm/unistd.h" OR
   NOT EXISTS "${LIBC_KERNEL_HEADERS}/asm/unistd_64.h" OR
   NOT EXISTS "${LIBC_KERNEL_HEADERS}/linux/errno.h")
  message(FATAL_ERROR
    "MMIX Linux libc requires an explicit LIBC_KERNEL_HEADERS export")
endif()

set(CMAKE_SYSTEM_NAME Linux CACHE STRING "")
set(CMAKE_SYSTEM_PROCESSOR mmix CACHE STRING "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "")
# libc supplies its own C++ support and does not consume installed libc++.
set(CMAKE_CXX_FLAGS "-nostdinc++" CACHE STRING "")
set(CMAKE_C_COMPILER_TARGET mmix-unknown-linux CACHE STRING "")
set(CMAKE_CXX_COMPILER_TARGET mmix-unknown-linux CACHE STRING "")
set(CMAKE_ASM_COMPILER_TARGET mmix-unknown-linux CACHE STRING "")
set(LLVM_DEFAULT_TARGET_TRIPLE mmix-unknown-linux CACHE STRING "")
# libc's CMake parser expects an explicit environment to locate the OS.
set(LIBC_TARGET_TRIPLE mmix-unknown-linux-unknown CACHE STRING "")
set(LLVM_ENABLE_RUNTIMES libc CACHE STRING "")
set(LLVM_LIBC_FULL_BUILD ON CACHE BOOL "")
set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "")
set(LLVM_INCLUDE_BENCHMARKS OFF CACHE BOOL "")
set(CMAKE_BUILD_TYPE Release CACHE STRING "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY CACHE STRING "")
