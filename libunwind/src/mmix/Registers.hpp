//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//
//  Models register sets for supported processors.

#ifndef LIBUNWIND_MMIX_REGISTERS_HPP
#define LIBUNWIND_MMIX_REGISTERS_HPP

#include "Context.h"

namespace libunwind {
class _LIBUNWIND_HIDDEN Registers_mmix;
extern "C" void __libunwind_Registers_mmix_jumpto(Registers_mmix *);

class _LIBUNWIND_HIDDEN Registers_mmix {
public:
  typedef uint64_t reg_t;
  typedef uint64_t link_reg_t;
  typedef const link_reg_t &link_hardened_reg_arg_t;

  Registers_mmix() : _registers{} {}
  explicit Registers_mmix(const void *registers) {
    static_assert(check_fit<Registers_mmix, unw_context_t>::does_fit,
                  "MMIX registers do not fit into unw_context_t");
    memcpy(&_registers, registers, sizeof(_registers));
  }

  static constexpr int lastDwarfRegNum() {
    return _LIBUNWIND_MMIX_HIGHEST_DWARF_REGISTER;
  }
  static int getArch() { return REGISTERS_MMIX; }

  bool validRegister(int num) const { return registerSlot(num) != nullptr; }
  uint64_t getRegister(int num) const {
    if (const uint64_t *slot = registerSlot(num))
      return *slot;
    _LIBUNWIND_ABORT("unsupported MMIX register");
  }
  void setRegister(int num, uint64_t value) {
    if (const uint64_t *slot = registerSlot(num))
      *const_cast<uint64_t *>(slot) = value;
    else
      _LIBUNWIND_ABORT("unsupported MMIX register");
  }

  uint64_t getSP() const { return _registers.gpr[254]; }
  void setSP(uint64_t value) { _registers.gpr[254] = value; }
  uint64_t getIP() const { return _registers.ip; }
  void setIP(uint64_t value) { _registers.ip = value; }
  uint64_t getRegisterWindow() const { return _registers.ro; }
  void setRegisterWindow(uint64_t value) { _registers.ro = value; }

  // Floating values share the GPR file; there is no separate FP/vector bank.
  bool validFloatRegister(int) const { return false; }
  bool validVectorRegister(int) const { return false; }
  double getFloatRegister(int) const {
    _LIBUNWIND_ABORT("MMIX has no separate floating-point registers");
  }
  void setFloatRegister(int, double) {
    _LIBUNWIND_ABORT("MMIX has no separate floating-point registers");
  }
  v128 getVectorRegister(int) const {
    _LIBUNWIND_ABORT("MMIX has no vector registers");
  }
  void setVectorRegister(int, v128) {
    _LIBUNWIND_ABORT("MMIX has no vector registers");
  }
  void jumpto() { __libunwind_Registers_mmix_jumpto(this); }

  static const char *getRegisterName(int num) {
    static const char *const gprNames[] = {
        "$0",   "$1",   "$2",   "$3",   "$4",   "$5",   "$6",   "$7",   "$8",
        "$9",   "$10",  "$11",  "$12",  "$13",  "$14",  "$15",  "$16",  "$17",
        "$18",  "$19",  "$20",  "$21",  "$22",  "$23",  "$24",  "$25",  "$26",
        "$27",  "$28",  "$29",  "$30",  "$31",  "$32",  "$33",  "$34",  "$35",
        "$36",  "$37",  "$38",  "$39",  "$40",  "$41",  "$42",  "$43",  "$44",
        "$45",  "$46",  "$47",  "$48",  "$49",  "$50",  "$51",  "$52",  "$53",
        "$54",  "$55",  "$56",  "$57",  "$58",  "$59",  "$60",  "$61",  "$62",
        "$63",  "$64",  "$65",  "$66",  "$67",  "$68",  "$69",  "$70",  "$71",
        "$72",  "$73",  "$74",  "$75",  "$76",  "$77",  "$78",  "$79",  "$80",
        "$81",  "$82",  "$83",  "$84",  "$85",  "$86",  "$87",  "$88",  "$89",
        "$90",  "$91",  "$92",  "$93",  "$94",  "$95",  "$96",  "$97",  "$98",
        "$99",  "$100", "$101", "$102", "$103", "$104", "$105", "$106", "$107",
        "$108", "$109", "$110", "$111", "$112", "$113", "$114", "$115", "$116",
        "$117", "$118", "$119", "$120", "$121", "$122", "$123", "$124", "$125",
        "$126", "$127", "$128", "$129", "$130", "$131", "$132", "$133", "$134",
        "$135", "$136", "$137", "$138", "$139", "$140", "$141", "$142", "$143",
        "$144", "$145", "$146", "$147", "$148", "$149", "$150", "$151", "$152",
        "$153", "$154", "$155", "$156", "$157", "$158", "$159", "$160", "$161",
        "$162", "$163", "$164", "$165", "$166", "$167", "$168", "$169", "$170",
        "$171", "$172", "$173", "$174", "$175", "$176", "$177", "$178", "$179",
        "$180", "$181", "$182", "$183", "$184", "$185", "$186", "$187", "$188",
        "$189", "$190", "$191", "$192", "$193", "$194", "$195", "$196", "$197",
        "$198", "$199", "$200", "$201", "$202", "$203", "$204", "$205", "$206",
        "$207", "$208", "$209", "$210", "$211", "$212", "$213", "$214", "$215",
        "$216", "$217", "$218", "$219", "$220", "$221", "$222", "$223", "$224",
        "$225", "$226", "$227", "$228", "$229", "$230", "$231", "$232", "$233",
        "$234", "$235", "$236", "$237", "$238", "$239", "$240", "$241", "$242",
        "$243", "$244", "$245", "$246", "$247", "$248", "$249", "$250", "$251",
        "$252", "$253", "$254", "$255",
    };
    int index = gprIndex(num);
    if (index >= 0)
      return gprNames[index];
    switch (num) {
    case UNW_REG_IP:
    case UNW_MMIX_PC:
      return "pc";
    case UNW_REG_SP:
      return "$254";
    case UNW_MMIX_RD:
      return "rD";
    case UNW_MMIX_RE:
      return "rE";
    case UNW_MMIX_RH:
      return "rH";
    case UNW_MMIX_RJ:
      return "rJ";
    case UNW_MMIX_RR:
      return "rR";
    case UNW_MMIX_RO:
      return "rO";
    case UNW_MMIX_RS:
      return "rS";
    case UNW_MMIX_RG:
      return "rG";
    case UNW_MMIX_RL:
      return "rL";
    default:
      return "unknown register";
    }
  }

private:
  static int gprIndex(int num) {
    if (num >= UNW_MMIX_R224 && num <= UNW_MMIX_R255)
      return num + 224;
    if (num >= UNW_MMIX_R0 && num <= UNW_MMIX_R223)
      return num - 48;
    return -1;
  }

  const uint64_t *registerSlot(int num) const {
    int index = gprIndex(num);
    if (index >= 0)
      return &_registers.gpr[index];
    switch (num) {
    case UNW_REG_IP:
    case UNW_MMIX_PC:
      return &_registers.ip;
    case UNW_REG_SP:
      return &_registers.gpr[254];
    case UNW_MMIX_RD:
      return &_registers.rd;
    case UNW_MMIX_RE:
      return &_registers.re;
    case UNW_MMIX_RH:
      return &_registers.rh;
    case UNW_MMIX_RJ:
      return &_registers.rj;
    case UNW_MMIX_RR:
      return &_registers.rr;
    case UNW_MMIX_RO:
      return &_registers.ro;
    case UNW_MMIX_RS:
      return &_registers.rs;
    case UNW_MMIX_RG:
      return &_registers.rg;
    case UNW_MMIX_RL:
      return &_registers.rl;
    default:
      return nullptr;
    }
  }

  MMIXUnwindContext _registers;
};
static_assert(sizeof(Registers_mmix) == sizeof(MMIXUnwindContext),
              "MMIX register wrapper must preserve the assembly layout");
static_assert(alignof(Registers_mmix) == alignof(MMIXUnwindContext),
              "MMIX register wrapper must preserve context alignment");
} // namespace libunwind

#endif
