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
  mmix/cxx_new.c)
