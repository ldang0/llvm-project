// RUN: llvm-tblgen -I %p -I %p/../../../include \
// RUN:   -I %p/../../../lib/Target/MMIX %s

include "MMIX.td"

// Keep data and address-taken symbols separate from unresolved direct calls.
// These assertions use the producer's declared semantic role, independent of
// the implementation classes shared by the records.
assert !eq(MMIXload_addr.MMIXAddressProducerUse, "static-address"),
       "static-address DAG producer changed roles";
assert !eq(LOAD_ADDR.MMIXAddressProducerUse, "static-address"),
       "static-address machine producer changed roles";
assert !eq(MMIXload_call_addr.MMIXAddressProducerUse,
           "unresolved-direct-call"),
       "direct-call DAG producer changed roles";
assert !eq(LOAD_CALL_ADDR.MMIXAddressProducerUse, "unresolved-direct-call"),
       "direct-call machine producer changed roles";
assert !eq(LOAD_ADDR.Size, 16),
       "static-address producer no longer models full materialization";
assert !eq(LOAD_CALL_ADDR.Size, 16),
       "direct-call producer no longer preserves canonical materialization";
