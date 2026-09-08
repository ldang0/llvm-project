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

function(add_mmix_crtdso)
  # Generic-system CMake defaults to .obj; this is an ELF CRT object.
  set(CMAKE_C_OUTPUT_EXTENSION .o)
  add_compiler_rt_runtime(clang_rt.crtdso
    OBJECT
    ARCHS mmix
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mmix/crtdso.c
    CFLAGS ${BUILTIN_CFLAGS_mmix}
    PARENT_TARGET builtins)
endfunction()
