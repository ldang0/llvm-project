# Detect the installed provider with the same compiler flags as the runtime.
# The project hook runs after compiler setup, before HandleLibC creates targets.
include_guard(GLOBAL)

function(mmix_detect_libc language result)
  separate_arguments(flags NATIVE_COMMAND "${CMAKE_${language}_FLAGS}")
  separate_arguments(compiler_arg NATIVE_COMMAND "${CMAKE_${language}_COMPILER_ARG1}")
  if(language STREQUAL "CXX")
    set(input_language c++)
  else()
    set(input_language c)
  endif()
  if(NOT CMAKE_SYSROOT)
    message(FATAL_ERROR "MMIX installed runtimes require CMAKE_SYSROOT")
  endif()
  execute_process(
    COMMAND "${CMAKE_${language}_COMPILER}" ${compiler_arg} ${flags}
      "--target=${CMAKE_${language}_COMPILER_TARGET}" "--sysroot=${CMAKE_SYSROOT}"
      -E -P -x ${input_language} "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/DetectLibc.cpp"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE diagnostics)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "Cannot identify the MMIX ${language} C provider: ${diagnostics}")
  endif()
  if(output MATCHES "(^|\n)MMIX_LIBCXX_LLVM_LIBC(\n|$)")
    set(${result} llvm-libc PARENT_SCOPE)
  elseif(output MATCHES "(^|\n)MMIX_LIBCXX_NEWLIB(\n|$)")
    set(${result} newlib PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Cannot identify the MMIX ${language} C provider")
  endif()
endfunction()

mmix_detect_libc(C c_provider)
mmix_detect_libc(CXX cxx_provider)
if(NOT c_provider STREQUAL cxx_provider)
  message(FATAL_ERROR "MMIX C and C++ compilers select different C providers")
endif()
if(DEFINED RUNTIMES_USE_LIBC AND NOT RUNTIMES_USE_LIBC STREQUAL "system"
    AND NOT RUNTIMES_USE_LIBC STREQUAL cxx_provider)
  message(FATAL_ERROR "RUNTIMES_USE_LIBC conflicts with the MMIX compiler C provider")
endif()
set(RUNTIMES_USE_LIBC "${cxx_provider}" CACHE STRING "Selected installed MMIX C library" FORCE)

# HandleLibC's existing target override supports an installed library without
# introducing an in-tree libc dependency or overriding Driver search paths.
add_library(runtimes-libc-headers INTERFACE)
add_library(runtimes-libc-static INTERFACE)
add_library(runtimes-libc-shared INTERFACE)
