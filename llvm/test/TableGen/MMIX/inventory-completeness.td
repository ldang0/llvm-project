// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// This is a semantic structural inventory check. It validates the MMIXware
// Section 51 opcode map and related architectural records without requiring an
// assembler, object format, MMO image, or QEMU.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXInventoryRecords =
    !filter(Record, !instances<MMIXInstruction>(),
            !eq(Record.isCodeGenOnly, 0));
defvar MMIXUnclassifiedRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "unclassified"));
defvar MMIXUnclassifiedForms =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryForm, "unclassified"));
defvar MMIXReviewedFormRecords =
    !filter(Record, MMIXInventoryRecords,
            !or(!eq(Record.MMIXInventoryForm, "imm8-imm16"),
                !eq(Record.MMIXInventoryForm, "imm8-reg-imm8"),
                !eq(Record.MMIXInventoryForm, "imm8-reg-reg"),
                !eq(Record.MMIXInventoryForm, "pcrel24-backward"),
                !eq(Record.MMIXInventoryForm, "pcrel24-forward"),
                !eq(Record.MMIXInventoryForm, "reg"),
                !eq(Record.MMIXInventoryForm, "reg-imm16"),
                !eq(Record.MMIXInventoryForm, "reg-imm8-imm8"),
                !eq(Record.MMIXInventoryForm, "reg-imm8-reg"),
                !eq(Record.MMIXInventoryForm,
                    "reg-or-imm8-reg-or-imm8-reg-or-imm8"),
                !eq(Record.MMIXInventoryForm, "reg-pcrel16-backward"),
                !eq(Record.MMIXInventoryForm, "reg-pcrel16-forward"),
                !eq(Record.MMIXInventoryForm, "reg-reg-imm8"),
                !eq(Record.MMIXInventoryForm, "reg-reg-reg"),
                !eq(Record.MMIXInventoryForm, "reg-round-imm8"),
                !eq(Record.MMIXInventoryForm, "reg-round-reg"),
                !eq(Record.MMIXInventoryForm, "reg-special-reg"),
                !eq(Record.MMIXInventoryForm, "resume-mode"),
                !eq(Record.MMIXInventoryForm, "special-reg-imm8"),
                !eq(Record.MMIXInventoryForm, "special-reg-reg"),
                !eq(Record.MMIXInventoryForm, "sync-mode")));
defvar MMIXUnmodeledRecords =
    !filter(Record, MMIXInventoryRecords,
            !ne(Record.MMIXInventoryStatus, "modeled"));

// Section 51 assigns every opcode from 0x00 through 0xff. Keeping this list in
// numeric opcode order makes each assertion below prove both record identity
// and opcode allocation; a duplicate or missing slot cannot preserve the test.
defvar MMIXExpectedRecordsByOpcode = [
  TRAP, FCMP, FUN, FEQL, FADD, FIX, FSUB, FIXU,
  FLOT, FLOTI, FLOTU, FLOTUI, SFLOT, SFLOTI, SFLOTU, SFLOTUI,
  FMUL, FCMPE, FUNE, FEQLE, FDIV, FSQRT, FREM, FINT,
  MUL, MULI, MULU, MULUI, DIV, DIVI, DIVU, DIVUI,
  ADD, ADDI, ADDU, ADDUI, SUB, SUBI, SUBU, SUBUI,
  TWOADDU, TWOADDUI, FOURADDU, FOURADDUI,
  EIGHTADDU, EIGHTADDUI, SIXTEENADDU, SIXTEENADDUI,
  CMP, CMPI, CMPU, CMPUI, NEG, NEGI, NEGU, NEGUI,
  SL, SLI, SLU, SLUI, SR, SRI, SRU, SRUI,
  BN, BNB, BZ, BZB, BP, BPB, BOD, BODB,
  BNN, BNNB, BNZ, BNZB, BNP, BNPB, BEV, BEVB,
  PBN, PBNB, PBZ, PBZB, PBP, PBPB, PBOD, PBODB,
  PBNN, PBNNB, PBNZ, PBNZB, PBNP, PBNPB, PBEV, PBEVB,
  CSN, CSNI, CSZ, CSZI, CSP, CSPI, CSOD, CSODI,
  CSNN, CSNNI, CSNZ, CSNZI, CSNP, CSNPI, CSEV, CSEVI,
  ZSN, ZSNI, ZSZ, ZSZI, ZSP, ZSPI, ZSOD, ZSODI,
  ZSNN, ZSNNI, ZSNZ, ZSNZI, ZSNP, ZSNPI, ZSEV, ZSEVI,
  LDB, LDBI, LDBU, LDBUI, LDW, LDWI, LDWU, LDWUI,
  LDT, LDTI, LDTU, LDTUI, LDO, LDOI, LDOU, LDOUI,
  LDSF, LDSFI, LDHT, LDHTI, CSWAP, CSWAPI, LDUNC, LDUNCI,
  LDVTS, LDVTSI, PRELD, PRELDI, PREGO, PREGOI, GO, GOI,
  STB, STBI, STBU, STBUI, STW, STWI, STWU, STWUI,
  STT, STTI, STTU, STTUI, STO, STOI, STOU, STOUI,
  STSF, STSFI, STHT, STHTI, STCO, STCOI, STUNC, STUNCI,
  SYNCD, SYNCDI, PREST, PRESTI, SYNCID, SYNCIDI,
  PUSHGO, PUSHGOI,
  OR, ORI, ORN, ORNI, NOR, NORI, XOR, XORI,
  AND, ANDI, ANDN, ANDNI, NAND, NANDI, NXOR, NXORI,
  BDIF, BDIFI, WDIF, WDIFI, TDIF, TDIFI, ODIF, ODIFI,
  MUX, MUXI, SADD, SADDI, MOR, MORI, MXOR, MXORI,
  SETH, SETMH, SETML, SETL, INCH, INCMH, INCML, INCL,
  ORH, ORMH, ORML, ORL, ANDNH, ANDNMH, ANDNML, ANDNL,
  JMP, JMPB, PUSHJ, PUSHJB, GETA, GETAB, PUT, PUTI,
  POP, RESUME, SAVE, UNSAVE, SYNC, SWYM, GET, TRIP
];

assert !eq(!size(MMIXInventoryRecords), 256),
       "MMIX architectural instruction inventory count changed";
assert !eq(!size(MMIXExpectedRecordsByOpcode), 256),
       "MMIXware Section 51 opcode list is incomplete";
assert !eq(!size(MMIXExpectedRecordsByOpcode),
           !size(MMIXInventoryRecords)),
       "MMIX target records do not match the reviewed inventory";

foreach Opcode = 0-255 in {
  assert !eq(MMIXExpectedRecordsByOpcode[Opcode].MMIXOpcode, Opcode),
         "MMIXware Section 51 opcode record is incorrect";
  assert !eq(MMIXExpectedRecordsByOpcode[Opcode].isCodeGenOnly, 0),
         "CodeGen-only instruction entered the architectural opcode map";
}

assert !eq(!size(MMIXUnclassifiedRecords), 0),
       "MMIX architectural instruction has no inventory family";
assert !eq(!size(MMIXUnclassifiedForms), 0),
       "MMIX architectural instruction has no operand form";
assert !eq(!size(MMIXReviewedFormRecords), 256),
       "MMIX architectural instruction has an unreviewed operand form";
assert !eq(!size(MMIXUnmodeledRecords), 0),
       "MMIX architectural instruction is not marked modeled";

defvar MMIXIntegerRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "integer"));
defvar MMIXControlFlowRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "control-flow"));
defvar MMIXMemoryRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "memory"));
defvar MMIXFloatingPointRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "floating-point"));
defvar MMIXSystemRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "system"));

assert !eq(!size(MMIXIntegerRecords), 120),
       "MMIX integer inventory changed";
assert !eq(!size(MMIXControlFlowRecords), 43),
       "MMIX control-flow inventory changed";
assert !eq(!size(MMIXMemoryRecords), 38),
       "MMIX memory inventory changed";
assert !eq(!size(MMIXFloatingPointRecords), 27),
       "MMIX floating-point inventory changed";
assert !eq(!size(MMIXSystemRecords), 28),
       "MMIX system inventory changed";

defvar MMIXBaseFeatureRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFeature, "base"));
defvar MMIXSystemFeatureRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFeature, "system"));
defvar MMIXCacheFeatureRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFeature, "cache"));
defvar MMIXVirtualMemoryFeatureRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFeature, "virtual-memory"));

assert !eq(!size(MMIXBaseFeatureRecords), 234),
       "MMIX base feature inventory changed";
assert !eq(!size(MMIXSystemFeatureRecords), 6),
       "MMIX system feature inventory changed";
assert !eq(!size(MMIXCacheFeatureRecords), 14),
       "MMIX cache feature inventory changed";
assert !eq(!size(MMIXVirtualMemoryFeatureRecords), 2),
       "MMIX virtual-memory feature inventory changed";

foreach Record = MMIXBaseFeatureRecords in
  assert !eq(!size(Record.Predicates), 0),
         "MMIX base instruction unexpectedly requires an optional feature";
foreach Record = MMIXSystemFeatureRecords in
  assert !and(!eq(!size(Record.Predicates), 1),
              !eq(Record.Predicates[0], HasMMIXSystem)),
         "MMIX system feature classification disagrees with its predicate";
foreach Record = MMIXCacheFeatureRecords in
  assert !and(!eq(!size(Record.Predicates), 1),
              !eq(Record.Predicates[0], HasMMIXCache)),
         "MMIX cache feature classification disagrees with its predicate";
foreach Record = MMIXVirtualMemoryFeatureRecords in
  assert !and(!eq(!size(Record.Predicates), 1),
              !eq(Record.Predicates[0], HasMMIXVirtualMemory)),
         "MMIX virtual-memory classification disagrees with its predicate";

// General and special registers are architectural inventories, not aliases or
// ABI assignments. The special-register list follows the GET/PUT encoding.
defvar MMIXExpectedSpecialRegisters = [
  RB, RD, RE, RH, RJ, RM, RR, RBB,
  RC, RN, RO, RS, RI, RT, RTT, RK,
  RQ, RU, RV, RG, RL, RA, RF, RP,
  RW, RX, RY, RZ, RWW, RXX, RYY, RZZ
];

assert !eq(!size(!instances<MMIXReg>()), 256),
       "MMIX general-register inventory changed";
assert !eq(!repr(GPR64.MemberList), "(sequence \"R%u\", 0, 255)"),
       "MMIX architectural register class no longer contains r0 through r255";
assert !eq(!size(!instances<MMIXSpecialReg>()), 32),
       "MMIX special-register inventory changed";
foreach Encoding = 0-31 in
  assert !eq(MMIXExpectedSpecialRegisters[Encoding].HWEncoding, Encoding),
         "MMIX special-register encoding is incorrect";

// Expansion aliases and compiler-only instructions are supplementary records;
// none contributes another architectural opcode slot.
assert !eq(!size(!instances<MMIXAlias>()), 2),
       "MMIX reviewed expansion-alias inventory changed";
assert !and(!eq(!size(MMIXSetAlias.MMIXExpansionInstructions), 1),
            !eq(MMIXSetAlias.MMIXExpansionInstructions[0], ORI)),
       "MMIX SET expansion relationship changed";
assert !and(!eq(!size(MMIXLDAAlias.MMIXExpansionInstructions), 2),
            !eq(MMIXLDAAlias.MMIXExpansionInstructions[0], ADDU),
            !eq(MMIXLDAAlias.MMIXExpansionInstructions[1], ADDUI)),
       "MMIX LDA expansion relationship changed";

foreach Record = !instances<MMIXPseudoInstruction>() in {
  assert !eq(Record.isPseudo, 1),
         "MMIX pseudo record is not marked as a pseudo";
  assert !eq(Record.isCodeGenOnly, 1),
         "MMIX pseudo record is not isolated from MC";
}
foreach Record = [RET, RET_VALUE] in {
  assert !eq(Record.isCodeGenOnly, 1),
         "MMIX fixed return form entered the architectural inventory";
  assert !eq(Record.MMIXOpcode, POP.MMIXOpcode),
         "MMIX fixed return form does not expand to POP";
}
