// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the MMIXware system, cache, and virtual-memory opcode allocation.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXSystemInstructions = !listconcat(
  !instances<MMIXCacheRegOpcode>(), !instances<MMIXCacheImmOpcode>(),
  !instances<MMIXVirtualTranslationRegOpcode>(),
  !instances<MMIXVirtualTranslationImmOpcode>(),
  !instances<MMIXUncachedLoadOpcode>(),
  !instances<MMIXUncachedLoadImmOpcode>(),
  !instances<MMIXUncachedStoreOpcode>(),
  !instances<MMIXUncachedStoreImmOpcode>(),
  !instances<MMIXTrapInstructionOpcode>(), !instances<MMIXSync>(),
  !instances<MMIXSwym>(), !instances<MMIXResume>(), !instances<MMIXGet>(),
  !instances<MMIXPutRegOpcode>(), !instances<MMIXPutImmOpcode>(),
  !instances<MMIXStoreConstantRegOpcode>(),
  !instances<MMIXStoreConstantImmOpcode>(), !instances<MMIXSave>(),
  !instances<MMIXUnsave>());

defvar MMIXCacheFeatureInstructions = !listconcat(
  !instances<MMIXCacheRegOpcode>(), !instances<MMIXCacheImmOpcode>(),
  !instances<MMIXUncachedLoadOpcode>(),
  !instances<MMIXUncachedLoadImmOpcode>(),
  !instances<MMIXUncachedStoreOpcode>(),
  !instances<MMIXUncachedStoreImmOpcode>());
defvar MMIXVirtualMemoryFeatureInstructions = !listconcat(
  !instances<MMIXVirtualTranslationRegOpcode>(),
  !instances<MMIXVirtualTranslationImmOpcode>());
defvar MMIXSystemFeatureInstructions = !listconcat(
  !instances<MMIXTrapInstructionOpcode>(), !instances<MMIXSync>(),
  !instances<MMIXResume>(), !instances<MMIXSave>(),
  !instances<MMIXUnsave>());

assert !eq(!size(!instances<MMIXCacheRegOpcode>()), 5),
       "unexpected MMIX cache register forms";
assert !eq(!size(!instances<MMIXCacheImmOpcode>()), 5),
       "unexpected MMIX cache immediate forms";
assert !eq(!size(!instances<MMIXVirtualTranslationRegOpcode>()), 1),
       "unexpected MMIX virtual translation register forms";
assert !eq(!size(!instances<MMIXVirtualTranslationImmOpcode>()), 1),
       "unexpected MMIX virtual translation immediate forms";
assert !eq(!size(!instances<MMIXTrapInstructionOpcode>()), 2),
       "unexpected MMIX trap forms";

foreach Record = MMIXCacheFeatureInstructions in {
  assert !eq(!size(Record.Predicates), 1),
         "MMIX cache instruction must have exactly one feature predicate";
  assert !eq(Record.Predicates[0], HasMMIXCache),
         "MMIX cache instruction has the wrong feature predicate";
}
foreach Record = MMIXVirtualMemoryFeatureInstructions in {
  assert !eq(!size(Record.Predicates), 1),
         "MMIX virtual-memory instruction must have exactly one feature predicate";
  assert !eq(Record.Predicates[0], HasMMIXVirtualMemory),
         "MMIX virtual-memory instruction has the wrong feature predicate";
}
foreach Record = MMIXSystemFeatureInstructions in {
  assert !eq(!size(Record.Predicates), 1),
         "MMIX system instruction must have exactly one feature predicate";
  assert !eq(Record.Predicates[0], HasMMIXSystem),
         "MMIX system instruction has the wrong feature predicate";
}

foreach Record = [GET, PUT, PUTI, STCO, STCOI, SWYM] in
  assert !eq(!size(Record.Predicates), 0),
         "MMIX base instruction unexpectedly requires an optional feature";

foreach Record = !listconcat(
    !instances<MMIXCacheRegOpcode>(), !instances<MMIXCacheImmOpcode>(),
    !instances<MMIXVirtualTranslationRegOpcode>(),
    !instances<MMIXVirtualTranslationImmOpcode>(),
    !instances<MMIXUncachedLoadOpcode>(),
    !instances<MMIXUncachedLoadImmOpcode>(),
    !instances<MMIXUncachedStoreOpcode>(),
    !instances<MMIXUncachedStoreImmOpcode>(), !instances<MMIXSync>(),
    !instances<MMIXSwym>(), !instances<MMIXResume>(), !instances<MMIXGet>(),
    !instances<MMIXPutRegOpcode>(), !instances<MMIXPutImmOpcode>(),
    !instances<MMIXStoreConstantRegOpcode>(),
    !instances<MMIXStoreConstantImmOpcode>()) in {
  assert !ne(Record.MMIXOpcode, 0),
         "MMIX system instruction has no opcode allocation";
  assert !eq(Record.MMIXInventoryFamily, "system"),
         "MMIX system instruction has the wrong family";
}

assert !eq(TRAP.MMIXOpcode, 0x00), "TRAP opcode is incorrect";
assert !eq(LDVTS.MMIXOpcode, 0x98), "LDVTS opcode is incorrect";
assert !eq(PRELD.MMIXOpcode, 0x9A), "PRELD opcode is incorrect";
assert !eq(SYNCD.MMIXOpcode, 0xB8), "SYNCD opcode is incorrect";
assert !eq(LDUNC.MMIXOpcode, 0x96), "LDUNC opcode is incorrect";
assert !eq(STUNC.MMIXOpcode, 0xB6), "STUNC opcode is incorrect";
assert !eq(PUT.MMIXOpcode, 0xF6), "PUT opcode is incorrect";
assert !eq(RESUME.MMIXOpcode, 0xF9), "RESUME opcode is incorrect";
assert !eq(SYNC.MMIXOpcode, 0xFC), "SYNC opcode is incorrect";
assert !eq(SWYM.MMIXOpcode, 0xFD), "SWYM opcode is incorrect";
assert !eq(GET.MMIXOpcode, 0xFE), "GET opcode is incorrect";
assert !eq(TRIP.MMIXOpcode, 0xFF), "TRIP opcode is incorrect";
