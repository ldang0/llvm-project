set(mmix_SOURCES
  divdc3.c
  divsc3.c
  ffsdi2.c
  muldc3.c
  mulsc3.c)

set(mmix_ATOMIC_SOURCES
  mmix/atomic.c)

set(mmix_STACK_PROTECTOR_SOURCES
  mmix/stack_protector_fail.c
  mmix/stack_protector_guard.c)

set(mmix_CXX_SOURCES
  mmix/cxx_delete.c
  mmix/cxx_dso_handle.c
  mmix/cxx_failure.c
  mmix/cxx_guard.c
  mmix/cxx_new.c)

option(COMPILER_RT_BUILD_MMIX_CXX_SUPPORT
  "Build the MMIX allocation and DSO companion for libc++abi" OFF)

function(add_mmix_cxx_support)
  if(COMPILER_RT_BUILD_MMIX_CXX_SUPPORT)
    # libc++abi owns guards and virtual-call failures in this composition.
    add_compiler_rt_runtime(clang_rt.cxx_support
      STATIC
      ARCHS mmix
      DEPS ${deps_mmix}
      SOURCES mmix/cxx_new.c mmix/cxx_delete.c mmix/cxx_dso_handle.c
      DEFS ${BUILTIN_DEFS}
      CFLAGS ${BUILTIN_CFLAGS_mmix}
      C_STANDARD 11
      PARENT_TARGET builtins)
  endif()
endfunction()
