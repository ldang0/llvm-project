function(compiler_rt_validate_mmix_runtime_selection)
  if(NOT COMPILER_RT_BUILD_BUILTINS)
    message(FATAL_ERROR "The MMIX compiler-rt configuration requires builtins")
  endif()

  set(unsupported_runtimes
    COMPILER_RT_BUILD_CRT
    COMPILER_RT_BUILD_SANITIZERS
    COMPILER_RT_BUILD_XRAY
    COMPILER_RT_BUILD_LIBFUZZER
    COMPILER_RT_BUILD_PROFILE
    COMPILER_RT_BUILD_CTX_PROFILE
    COMPILER_RT_BUILD_MEMPROF
    COMPILER_RT_BUILD_ORC
    COMPILER_RT_BUILD_GWP_ASAN
    COMPILER_RT_BUILD_PROFILE_ROCM
    COMPILER_RT_BUILD_STANDALONE_LIBATOMIC)

  foreach(runtime ${unsupported_runtimes})
    if(${runtime})
      message(FATAL_ERROR
        "${runtime} is not supported by the MMIX compiler-rt configuration; "
        "only the MMIX builtins, atomic fallback, stack protector, and C++ "
        "foundation archives may be enabled")
    endif()
  endforeach()
endfunction()
