# FIXME: Use the ordinary Driver spelling after public exception admission.
# Preserve the generic C sources' exception semantics, including non-nounwind
# calls under LTO, without opening MMIX's public exception Driver profile.
set_source_files_properties(${LIBUNWIND_C_SOURCES} PROPERTIES
  COMPILE_FLAGS "-std=c99 -Xclang -fexceptions")
