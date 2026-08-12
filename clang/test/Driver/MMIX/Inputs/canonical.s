	.text
	.globl	canonical_entry
	.type	canonical_entry,@function
canonical_entry:
	SETL r1, 42
	PUSHJ r31, local_callee
	GETA r2, %geta(shared_data)
	LDOU r3, r2, 0
	POP 0, 0
	.size	canonical_entry, .-canonical_entry

	.type	local_callee,@function
local_callee:
	ADDU r1, r1, 1
	POP 0, 0
	.size	local_callee, .-local_callee

	.section	.rodata,"a",@progbits
	.globl	shared_data
	.type	shared_data,@object
	.p2align	3
shared_data:
	.8byte	external_data
	.size	shared_data, 8

	.globl	external_data
