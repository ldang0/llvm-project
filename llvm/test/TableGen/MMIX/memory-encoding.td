// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// Verify the MMIXware memory opcode allocation. Cache, prefetch, virtual
// translation, and control-flow opcodes are checked by their later tasks.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXLoadReg = !instances<MMIXLoadRegOpcode>();
defvar MMIXLoadImm = !instances<MMIXLoadImmOpcode>();
defvar MMIXStoreReg = !instances<MMIXStoreRegOpcode>();
defvar MMIXStoreImm = !instances<MMIXStoreImmOpcode>();
defvar MMIXFPLoadReg = !instances<MMIXFPLoadRegOpcode>();
defvar MMIXFPLoadImm = !instances<MMIXFPLoadImmOpcode>();
defvar MMIXFPStoreReg = !instances<MMIXFPStoreRegOpcode>();
defvar MMIXFPStoreImm = !instances<MMIXFPStoreImmOpcode>();
defvar MMIXUncachedLoad = !instances<MMIXUncachedLoadOpcode>();
defvar MMIXUncachedLoadImm = !instances<MMIXUncachedLoadImmOpcode>();
defvar MMIXUncachedStore = !instances<MMIXUncachedStoreOpcode>();
defvar MMIXUncachedStoreImm = !instances<MMIXUncachedStoreImmOpcode>();

assert !eq(!size(MMIXLoadReg), 9), "unexpected MMIX load register forms";
assert !eq(!size(MMIXLoadImm), 9), "unexpected MMIX load immediate forms";
assert !eq(!size(MMIXStoreReg), 9), "unexpected MMIX store register forms";
assert !eq(!size(MMIXStoreImm), 9), "unexpected MMIX store immediate forms";
assert !eq(!size(!instances<MMIXCompareSwapRegOpcode>()), 1),
       "unexpected MMIX compare-swap register forms";
assert !eq(!size(!instances<MMIXCompareSwapImmOpcode>()), 1),
       "unexpected MMIX compare-swap immediate forms";
assert !eq(!size(MMIXFPLoadReg), 1), "unexpected MMIX short-float load forms";
assert !eq(!size(MMIXFPLoadImm), 1), "unexpected MMIX short-float load forms";
assert !eq(!size(MMIXFPStoreReg), 1), "unexpected MMIX short-float store forms";
assert !eq(!size(MMIXFPStoreImm), 1), "unexpected MMIX short-float store forms";
assert !eq(!size(!instances<MMIXStoreConstantRegOpcode>()), 1),
       "unexpected MMIX constant store register forms";
assert !eq(!size(!instances<MMIXStoreConstantImmOpcode>()), 1),
       "unexpected MMIX constant store immediate forms";
foreach Record = [STCO, STCOI] in
  assert !eq(Record.mayStore, 1),
         "MMIX constant octa store must retain its memory effect";
assert !eq(!size(MMIXUncachedLoad), 1), "unexpected MMIX uncached load forms";
assert !eq(!size(MMIXUncachedLoadImm), 1),
       "unexpected MMIX uncached load immediate forms";
assert !eq(!size(MMIXUncachedStore), 1),
       "unexpected MMIX uncached store forms";
assert !eq(!size(MMIXUncachedStoreImm), 1),
       "unexpected MMIX uncached store immediate forms";

foreach Record = !listconcat(MMIXLoadReg, MMIXLoadImm, MMIXStoreReg,
                             MMIXStoreImm, MMIXFPLoadReg, MMIXFPLoadImm,
                             MMIXFPStoreReg, MMIXFPStoreImm,
                             MMIXUncachedLoad, MMIXUncachedLoadImm,
                             MMIXUncachedStore, MMIXUncachedStoreImm,
                             !instances<MMIXCompareSwapRegOpcode>(),
                             !instances<MMIXCompareSwapImmOpcode>(),
                             !instances<MMIXStoreConstantRegOpcode>(),
                             !instances<MMIXStoreConstantImmOpcode>()) in {
  assert !ne(Record.MMIXOpcode, 0),
         "MMIX memory instruction has no opcode allocation";
}

assert !eq(LDB.MMIXOpcode, 0x80), "LDB opcode is incorrect";
assert !eq(LDOUI.MMIXOpcode, 0x8F), "LDOUI opcode is incorrect";
assert !eq(LDSF.MMIXOpcode, 0x90), "LDSF opcode is incorrect";
assert !eq(CSWAP.MMIXOpcode, 0x94), "CSWAP opcode is incorrect";
assert !eq(STB.MMIXOpcode, 0xA0), "STB opcode is incorrect";
assert !eq(STOUI.MMIXOpcode, 0xAF), "STOUI opcode is incorrect";
assert !eq(STCO.MMIXOpcode, 0xB4), "STCO opcode is incorrect";
assert !eq(LDUNC.MMIXOpcode, 0x96), "LDUNC opcode is incorrect";
assert !eq(STUNC.MMIXOpcode, 0xB6), "STUNC opcode is incorrect";
