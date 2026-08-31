// RUN: llvm-tblgen -I %p -I %p/../../../include \
// RUN:   -I %p/../../../lib/Target/MMIX %s

// Freeze the reviewed MMIX MC, CodeGen-disposition, and access-interface
// classifications. These sets describe reviewed semantics and deliberately do
// not infer membership from the instruction-format inheritance hierarchy.
include "inventory-completeness.td"

class MMIXComplianceClassification<string Layer, string Classification,
                                   list<Instruction> Records> {
  string MMIXComplianceLayer = Layer;
  string MMIXComplianceClassification = Classification;
  list<Instruction> MMIXComplianceRecords = Records;
}

defvar MMIXGenericCodeGenRecords = [
  TRAP, FCMP, FUN, FEQL, FADD, FSUB, FIXU, FLOT,
  FLOTI, FLOTU, FLOTUI, SFLOT, SFLOTI, SFLOTU, SFLOTUI, FMUL,
  FDIV, FSQRT, FINT, MULU, MULUI, DIV, DIVI, DIVU,
  DIVUI, ADDU, ADDUI, SUBU, SUBUI, CMP, CMPI, CMPU,
  CMPUI, NEGU, NEGUI, SLU, SLUI, SR, SRI, SRU,
  SRUI, BN, BNB, BZ, BZB, BP, BPB, BOD,
  BODB, BNN, BNNB, BNZ, BNZB, BNP, BNPB, BEV,
  BEVB, LDB, LDBI, LDBU, LDBUI, LDW, LDWI, LDWU,
  LDWUI, LDT, LDTI, LDTU, LDTUI, LDO, LDOI, LDOU,
  LDOUI, LDSF, LDSFI, CSWAP, CSWAPI, GO, GOI, STBU,
  STBUI, STWU, STWUI, STTU, STTUI, STO, STOI, STOU,
  STOUI, STSF, STSFI, PUSHGO, PUSHGOI, OR, ORI, ORN,
  ORNI, NOR, NORI, XOR, XORI, AND, ANDI, ANDN,
  ANDNI, NAND, NANDI, NXOR, NXORI, SETH, SETMH, SETML,
  SETL, INCH, INCMH, INCML, INCL, ORH, ORMH, ORML,
  ORL, ANDNH, ANDNMH, ANDNML, ANDNL, JMP, JMPB, PUSHJ,
  PUSHJB, GETA, GETAB, PUT, PUTI, POP, SYNC, GET
];

defvar MMIXCombineCodeGenRecords = [
  FIX, FCMPE, FUNE, FEQLE, FREM, MUL, MULI, ADD,
  ADDI, SUB, SUBI, TWOADDU, TWOADDUI, FOURADDU, FOURADDUI, EIGHTADDU,
  EIGHTADDUI, SIXTEENADDU, SIXTEENADDUI, NEG, NEGI, SL, SLI, PBN,
  PBNB, PBZ, PBZB, PBP, PBPB, PBOD, PBODB, PBNN,
  PBNNB, PBNZ, PBNZB, PBNP, PBNPB, PBEV, PBEVB, CSN,
  CSNI, CSZ, CSZI, CSP, CSPI, CSOD, CSODI, CSNN,
  CSNNI, CSNZ, CSNZI, CSNP, CSNPI, CSEV, CSEVI, ZSN,
  ZSNI, ZSZ, ZSZI, ZSP, ZSPI, ZSOD, ZSODI, ZSNN,
  ZSNNI, ZSNZ, ZSNZI, ZSNP, ZSNPI, ZSEV, ZSEVI, LDHT,
  LDHTI, PRELD, PRELDI, PREGO, PREGOI, STB, STBI, STW,
  STWI, STT, STTI, STHT, STHTI, STCO, STCOI, PREST,
  PRESTI, BDIF, BDIFI, WDIF, WDIFI, TDIF, TDIFI, ODIF,
  ODIFI, MUX, MUXI, SADD, SADDI, MOR, MORI, MXOR,
  MXORI
];

defvar MMIXTargetSpecificCodeGenRecords = [
  LDUNC, LDUNCI, LDVTS, LDVTSI, STUNC, STUNCI, SYNCD, SYNCDI,
  SYNCID, SYNCIDI, RESUME, SAVE, UNSAVE, TRIP
];

defvar MMIXNoCodeGenRecords = [SWYM];

def MMIXMCComplete
    : MMIXComplianceClassification<"mc", "complete",
                                   MMIXExpectedRecordsByOpcode>;
def MMIXCodeGenGeneric
    : MMIXComplianceClassification<"codegen-disposition", "generic",
                                   MMIXGenericCodeGenRecords>;
def MMIXCodeGenCombine
    : MMIXComplianceClassification<"codegen-disposition", "combine",
                                   MMIXCombineCodeGenRecords>;
def MMIXCodeGenTargetSpecific
    : MMIXComplianceClassification<"codegen-disposition", "target-specific",
                                   MMIXTargetSpecificCodeGenRecords>;
def MMIXCodeGenNone
    : MMIXComplianceClassification<"codegen-disposition", "none",
                                   MMIXNoCodeGenRecords>;

defvar MMIXGenericLoweringRecords = [
  TRAP, FCMP, FUN, FEQL, FADD, FSUB, FIXU, FLOT,
  FLOTI, FLOTU, FLOTUI, SFLOT, SFLOTI, SFLOTU, SFLOTUI, FMUL,
  FDIV, FSQRT, FINT, MULU, MULUI, DIV, DIVI, DIVU,
  DIVUI, ADDU, ADDUI, SUBU, SUBUI, TWOADDU, TWOADDUI, FOURADDU,
  FOURADDUI, EIGHTADDU, EIGHTADDUI, SIXTEENADDU, SIXTEENADDUI, CMP, CMPI, CMPU,
  CMPUI, NEGU, NEGUI, SLU, SLUI, SR, SRI, SRU,
  SRUI, BN, BNB, BZ, BZB, BP, BPB, BOD,
  BODB, BNN, BNNB, BNZ, BNZB, BNP, BNPB, BEV,
  BEVB, CSZI, CSNZ, CSNZI, ZSNI, ZSZI, ZSPI, ZSNNI,
  ZSNZI, ZSNPI, LDB, LDBI, LDBU, LDBUI, LDW, LDWI,
  LDWU, LDWUI, LDT, LDTI, LDTU, LDTUI, LDO, LDOI,
  LDOU, LDOUI, LDSF, LDSFI, CSWAP, CSWAPI, GO, GOI,
  STBU, STBUI, STWU, STWUI, STTU, STTUI, STO, STOI,
  STOU, STOUI, STSF, STSFI, PUSHGO, PUSHGOI, OR, ORI,
  ORN, ORNI, NOR, NORI, XOR, XORI, AND, ANDI,
  ANDN, ANDNI, NAND, NANDI, NXOR, NXORI, ODIF, ODIFI,
  SADDI, SETH, SETMH, SETML, SETL, INCH, INCMH, INCML,
  INCL, ORH, ORMH, ORML, ORL, ANDNH, ANDNMH, ANDNML,
  ANDNL, JMP, JMPB, PUSHJ, PUSHJB, GETA, GETAB, PUT,
  PUTI, POP, SYNC, GET
];

defvar MMIXTargetIntrinsicRecords = [
  TRAP, LDUNC, LDUNCI, LDVTS, LDVTSI, PRELD, PRELDI, PREGO, PREGOI,
  STUNC, STUNCI, SYNCD, SYNCDI, PREST, PRESTI, SYNCID, SYNCIDI,
  PUT, PUTI, SYNC, GET
];
defvar MMIXFunctionInlineAssemblyRecords = [SYNC, SWYM];
defvar MMIXModuleInlineAssemblyRecords = [
  TRAP, GO, GOI, PUT, PUTI, RESUME, SAVE, UNSAVE, TRIP
];
defvar MMIXMCOnlyRecords = [
  FIX, FCMPE, FUNE, FEQLE, FREM, MUL, MULI, ADD,
  ADDI, SUB, SUBI, NEG, NEGI, SL, SLI, PBN,
  PBNB, PBZ, PBZB, PBP, PBPB, PBOD, PBODB, PBNN,
  PBNNB, PBNZ, PBNZB, PBNP, PBNPB, PBEV, PBEVB, CSN,
  CSNI, CSZ, CSP, CSPI, CSOD, CSODI, CSNN, CSNNI,
  CSNP, CSNPI, CSEV, CSEVI, ZSN, ZSZ, ZSP, ZSOD,
  ZSODI, ZSNN, ZSNZ, ZSNP, ZSEV, ZSEVI, LDHT, LDHTI,
  STB, STBI, STW, STWI, STT, STTI, STHT, STHTI,
  STCO, STCOI, BDIF, BDIFI, WDIF, WDIFI, TDIF, TDIFI,
  MUX, MUXI, SADD, MOR, MORI, MXOR, MXORI
];

def MMIXAccessGenericLowering
    : MMIXComplianceClassification<"codegen-interface", "generic-lowering",
                                   MMIXGenericLoweringRecords>;
def MMIXAccessTargetIntrinsic
    : MMIXComplianceClassification<"codegen-interface", "target-intrinsic",
                                   MMIXTargetIntrinsicRecords>;
def MMIXAccessFunctionAssembly
    : MMIXComplianceClassification<"codegen-interface",
                                   "function-inline-assembly",
                                   MMIXFunctionInlineAssemblyRecords>;
def MMIXAccessModuleAssembly
    : MMIXComplianceClassification<"codegen-interface",
                                   "module-inline-assembly",
                                   MMIXModuleInlineAssemblyRecords>;
def MMIXAccessMCOnly
    : MMIXComplianceClassification<"codegen-interface", "mc-only",
                                   MMIXMCOnlyRecords>;

defvar MMIXClassifiedCodeGenRecords =
    !listconcat(MMIXGenericCodeGenRecords, MMIXCombineCodeGenRecords,
                MMIXTargetSpecificCodeGenRecords, MMIXNoCodeGenRecords);
defvar MMIXClassifiedAccessRecords =
    !listconcat(MMIXGenericLoweringRecords, MMIXTargetIntrinsicRecords,
                MMIXFunctionInlineAssemblyRecords,
                MMIXModuleInlineAssemblyRecords, MMIXMCOnlyRecords);

assert !eq(!size(MMIXClassifiedCodeGenRecords), 256),
       "MMIX CodeGen disposition classifications changed";
assert !eq(!size(!listremove(MMIXInventoryRecords,
                            MMIXClassifiedCodeGenRecords)), 0),
       "MMIX architectural instruction has no CodeGen disposition";
assert !eq(!size(!listremove(MMIXClassifiedCodeGenRecords,
                            MMIXInventoryRecords)), 0),
       "nonarchitectural instruction entered a CodeGen disposition";
assert !eq(!size(!listremove(MMIXInventoryRecords,
                            MMIXClassifiedAccessRecords)), 0),
       "MMIX architectural instruction has no reviewed access interface";
