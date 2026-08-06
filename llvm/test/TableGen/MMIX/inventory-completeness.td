// RUN: llvm-tblgen -I %p/../../../include -I %p/../../../lib/Target/MMIX %s

// This is a structural inventory check. It validates the MMIX TableGen
// records without requiring an assembler, object format, MMO image, or QEMU.
include "../../../lib/Target/MMIX/MMIX.td"

defvar MMIXInventoryRecords =
    !filter(Record, !instances<MMIXInstruction>(),
            !eq(Record.isCodeGenOnly, 0));
defvar MMIXUnclassifiedRecords =
    !filter(Record, MMIXInventoryRecords,
            !eq(Record.MMIXInventoryFamily, "unclassified"));
defvar MMIXUnmodeledRecords =
    !filter(Record, MMIXInventoryRecords,
            !ne(Record.MMIXInventoryStatus, "modeled"));

// This list is the reviewed MMIXware instruction inventory. Referencing each
// record makes a missing architectural definition a TableGen error instead
// of allowing a replacement record to preserve only the aggregate count.
defvar MMIXExpectedRecords = [
  ADD, ADDI, ADDU, ADDUI, AND, ANDI, ANDN, ANDNH, ANDNI, ANDNL, ANDNMH,
  ANDNML, BDIF, BDIFI, BEV, BEVB, BN, BNB, BNN, BNNB, BNP, BNPB, BNZ, BNZB,
  BOD, BODB, BP, BPB, BZ, BZB, CMP, CMPI, CMPU, CMPUI, CSEV, CSEVI, CSN,
  CSNI, CSNN, CSNNI, CSNP, CSNPI, CSNZ, CSNZI, CSOD, CSODI, CSP, CSPI,
  CSWAP, CSWAPI, CSZ, CSZI, DIV, DIVI, DIVU, DIVUI, EIGHTADDU, EIGHTADDUI,
  FADD, FCMP, FCMPE, FDIV, FEQL, FEQLE, FINT, FIX, FIXU, FLOT, FLOTI, FLOTU,
  FLOTUI, FMUL, FOURADDU, FOURADDUI, FREM, FSQRT, FSUB, FUN, FUNE, GET, GETA,
  GETAB, GO, GOI, INCH, INCL, INCMH, INCML, JMP, JMPB, LDB, LDBI, LDBU, LDBUI,
  LDHT, LDHTI, LDO, LDOI, LDOU, LDOUI, LDSF, LDSFI, LDT, LDTI, LDTU, LDTUI,
  LDUNC, LDUNCI, LDVTS, LDVTSI, LDW, LDWI, LDWU, LDWUI, MOR, MORI, MUL, MULI,
  MULU, MULUI, MUX, MUXI, MXOR, MXORI, NAND, NANDI, NEG, NEGI, NEGU, NEGUI,
  NOR, NORI, NXOR, NXORI, ODIF, ODIFI, OR, ORH, ORI, ORL, ORMH, ORML, ORN,
  ORNI, PBEV, PBEVB, PBN, PBNB, PBNN, PBNNB, PBNP, PBNPB, PBNZ, PBNZB, PBOD,
  PBODB, PBP, PBPB, PBZ, PBZB, POP, PREGO, PREGOI, PRELD, PRELDI, PREST,
  PRESTI, PUSHGO, PUSHGOI, PUSHJ, PUSHJB, PUT, PUTI, RESUME, SADD, SADDI, SAVE,
  SETH, SETL, SETMH, SETML, SFLOT, SFLOTI, SFLOTU, SFLOTUI, SIXTEENADDU,
  SIXTEENADDUI, SL, SLI, SLU, SLUI, SR, SRI, SRU, SRUI, STB, STBI, STBU, STBUI,
  STCO, STCOI, STHT, STHTI, STO, STOI, STOU, STOUI, STSF, STSFI, STT, STTI,
  STTU, STTUI, STUNC, STUNCI, STW, STWI, STWU, STWUI, SUB, SUBI, SUBU, SUBUI,
  SWYM, SYNC, SYNCD, SYNCDI, SYNCID, SYNCIDI, TDIF, TDIFI, TRAP, TRIP,
  TWOADDU, TWOADDUI, UNSAVE, WDIF, WDIFI, XOR, XORI, ZSEV, ZSEVI, ZSN, ZSNI,
  ZSNN, ZSNNI, ZSNP, ZSNPI, ZSNZ, ZSNZI, ZSOD, ZSODI, ZSP, ZSPI, ZSZ, ZSZI
];

// The count is the reviewed MMIX architectural instruction inventory at this
// stage. A changed count requires an accompanying inventory review.
assert !eq(!size(MMIXInventoryRecords), 256),
       "MMIX architectural instruction inventory count changed";
assert !eq(!size(MMIXExpectedRecords), !size(MMIXInventoryRecords)),
       "MMIX target records do not match the reviewed inventory";
assert !eq(!size(MMIXUnclassifiedRecords), 0),
       "MMIX architectural instruction has no inventory family";
assert !eq(!size(MMIXUnmodeledRecords), 0),
       "MMIX architectural instruction is not marked modeled";
