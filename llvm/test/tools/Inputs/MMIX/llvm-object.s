.text
.local llvm_local_function
.type llvm_local_function,@function
llvm_local_function:
  SWYM 0, 0, 0
.size llvm_local_function, .-llvm_local_function

.global llvm_entry
.type llvm_entry,@function
llvm_entry:
  GETA r1, %geta(gnu_data)
  PUSHJ r2, gnu_function
  JMP llvm_entry
.size llvm_entry, .-llvm_entry

.weak llvm_weak_defined
.type llvm_weak_defined,@function
llvm_weak_defined:
  SWYM 1, 0, 0
.size llvm_weak_defined, .-llvm_weak_defined

.weak llvm_weak_undefined

.data
.p2align 3
.local llvm_local_object
.type llvm_local_object,@object
llvm_local_object:
  .quad 0x0102030405060708
.size llvm_local_object, .-llvm_local_object

.global llvm_object
.type llvm_object,@object
llvm_object:
  .quad gnu_data
.size llvm_object, .-llvm_object

.global llvm_absolute
.set llvm_absolute, 0x42

.comm llvm_common,16,8

.global gnu_function
.global gnu_data
