// Gamecube Power PC Disassembler (written by ModSault)
// This was made to get the exact same output as the Dolphin Disassembler
// The reason Dolphin's Disassembler wasn't used was because it wasn't under the MIT licence

#include "PowerPCDisassembler.h"
#include <sstream>
#include <string>
#include "../../Common/CommonTypes.h"

namespace Common
{

// grabs 'sizeInBits' starting at 'startBit' from 'binary'. 0 = leftmost bit, 31 = rightmost bit
u32 PowerPCDisassembler::ValueAtBits(const u32 binary, const u32 startBit, const u32 sizeInBits)
{
  return (binary >> (32 - (startBit + sizeInBits))) & ((1 << sizeInBits) - 1);
}
// MA - Mnemonic Add
// adds a period or the letter 'o' to the mnemonic if needed
std::string PowerPCDisassembler::MA(const std::string& mnemonic, const u32 rc,
                                    const u32 add_letter_o)
{
  std::string toReturn = mnemonic;
  if (add_letter_o)
    toReturn += "o";
  if (rc)
    toReturn += ".";
  return toReturn;
}
// TS - To String
// changes an integer to a string in decimal or hex format. Handles padding too.
std::string PowerPCDisassembler::TS(const u32 value, const u8 mode, const u8 size)
{
  std::stringstream ss;

  if (mode == 10)
    ss << std::dec << static_cast<s32>(value);
  else if (mode == 16)
    ss << std::hex << std::uppercase << value;

  std::string toReturn = ss.str();
  if (size != 0 && toReturn.size() < size)
  {
    toReturn = std::string(size - toReturn.size(), '0') + toReturn;
  }
  return toReturn;
}

// Turns a number to proper alias. 'str_prefix' is added to the beginning of the string. r1 turns to
// sp and r2 turns to rtoc.
std::string PowerPCDisassembler::Alias(const u32 number, const std::string& str_prefix)
{
  if (str_prefix == "r")
  {
    if (number == 1)
      return "sp";
    if (number == 2)
      return "rtoc";
  }
  if (str_prefix != "")
  {
    return str_prefix + TS(number);
  }
  switch (number)
  {
  case 1:
    return "XER";
  case 8:
    return "LR";
  case 9:
    return "CTR";
  case 18:
    return "DSISR";
  case 19:
    return "DAR";
  case 22:
    return "DEC";
  case 25:
    return "SDR1";
  case 26:
    return "SRR0";
  case 27:
    return "SRR1";
  case 268:
    return "TBLr";
  case 269:
    return "TBUr";
  case 272:
    return "SPRG0";
  case 273:
    return "SPRG1";
  case 274:
    return "SPRG2";
  case 275:
    return "SPRG3";
  case 282:
    return "EAR";
  case 284:
    return "TBLw";
  case 285:
    return "TBUw";
  case 287:
    return "PVR";
  case 528:
    return "IBAT0U";
  case 529:
    return "IBAT0L";
  case 530:
    return "IBAT1U";
  case 531:
    return "IBAT1L";
  case 532:
    return "IBAT2U";
  case 533:
    return "IBAT2L";
  case 534:
    return "IBAT3U";
  case 535:
    return "IBAT3L";
  case 536:
    return "DBAT0U";
  case 537:
    return "DBAT0L";
  case 538:
    return "DBAT1U";
  case 539:
    return "DBAT1L";
  case 540:
    return "DBAT2U";
  case 541:
    return "DBAT2L";
  case 542:
    return "DBAT3U";
  case 543:
    return "DBAT3L";
  case 560:
    return "IBAT4U";
  case 561:
    return "IBAT4L";
  case 562:
    return "IBAT5U";
  case 563:
    return "IBAT5L";
  case 564:
    return "IBAT6U";
  case 565:
    return "IBAT6L";
  case 566:
    return "IBAT7U";
  case 567:
    return "IBAT7L";
  case 568:
    return "DBAT4U";
  case 569:
    return "DBAT4L";
  case 570:
    return "DBAT5U";
  case 571:
    return "DBAT5L";
  case 572:
    return "DBAT6U";
  case 573:
    return "DBAT6L";
  case 574:
    return "DBAT7U";
  case 575:
    return "DBAT7L";
  case 912:
    return "GQR0";
  case 913:
    return "GQR1";
  case 914:
    return "GQR2";
  case 915:
    return "GQR3";
  case 916:
    return "GQR4";
  case 917:
    return "GQR5";
  case 918:
    return "GQR6";
  case 919:
    return "GQR7";
  case 920:
    return "HID2";
  case 921:
    return "WPAR";
  case 922:
    return "DMAU";
  case 923:
    return "DMAL";
  case 924:
    return "ECID_U";
  case 925:
    return "ECID_M";
  case 926:
    return "ECID_L";
  case 936:
    return "UMMCR0";
  case 937:
    return "UPMC1";
  case 938:
    return "UPMC2";
  case 939:
    return "USIA";
  case 940:
    return "UMMCR1";
  case 941:
    return "UPMC3";
  case 942:
    return "UPMC4";
  case 943:
    return "USDA";
  case 952:
    return "MMCR0";
  case 953:
    return "PMC1";
  case 954:
    return "PMC2";
  case 955:
    return "SIA";
  case 956:
    return "MMCR1";
  case 957:
    return "PMC3";
  case 958:
    return "PMC4";
  case 959:
    return "SDA";
  case 1008:
    return "HID0";
  case 1009:
    return "HID1";
  case 1010:
    return "IABR";
  case 1011:
    return "HID4";
  case 1012:
    return "TDCL";
  case 1013:
    return "DABR";
  case 1017:
    return "L2CR";
  case 1018:
    return "TDCH";
  case 1019:
    return "ICTC";
  case 1020:
    return "THRM1";
  case 1021:
    return "THRM2";
  case 1022:
    return "THRM3";
  default:
    return TS(number);
  }
}
// for trap instructions, this gets a string depending on the 'to' value
std::string PowerPCDisassembler::TrapTOStr(const u32 to)
{
  switch (to)
  {
  case 1:
    return "lgt";
  case 2:
    return "llt";
  case 4:
    return "eq";
  case 5:
    return "lge";
  case 6:
    return "lle";
  case 8:
    return "gt";
  case 12:
    return "ge";
  case 16:
    return "lt";
  case 20:
    return "le";
  case 24:
    return "ne";
  default:
    return "";
  }
}

// returns illegal instruction
std::string PowerPCDisassembler::ILL(const u32 binary)
{
  return "(ill) 0x" + TS(binary, 16, 8);
}

// handles 'trapi' statements
std::string PowerPCDisassembler::TXI(const u32 binary, const std::string& type)
{
  u32 to = ValueAtBits(binary, 6, 5);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 si = ValueAtBits(binary, 16, 16);
  if (si >= 0x00008000)
    si += 0xFFFF0000;

  std::string pseudo = TrapTOStr(to);
  std::string toReturn = "t" + type + pseudo + "i ";

  if (pseudo == "")
  {
    toReturn += TS(to) + ", ";
  }
  return toReturn + Alias(ra, "r") + ", " + TS(si);
}

// handles 'trap' statements
std::string PowerPCDisassembler::TX(const u32 binary, const std::string& type)
{
  u32 to = ValueAtBits(binary, 6, 5);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 rb = ValueAtBits(binary, 16, 5);

  std::string pseudo = TrapTOStr(to);
  std::string toReturn = "t" + type + pseudo + " ";

  return toReturn + Alias(ra, "r") + ", " + Alias(rb, "r");
}

// handles all for opcode 4
std::string PowerPCDisassembler::opcode4Handler(const u32 binary)
{
  u32 d = ValueAtBits(binary, 6, 5);
  u32 a = ValueAtBits(binary, 11, 5);
  u32 b = ValueAtBits(binary, 16, 5);
  u32 c = ValueAtBits(binary, 21, 5);

  u32 w = ValueAtBits(binary, 21, 1);
  u32 i = ValueAtBits(binary, 22, 3);

  u32 ext_5 = ValueAtBits(binary, 26, 5);
  u32 ext_6 = ValueAtBits(binary, 25, 6);
  u32 ext_10 = ValueAtBits(binary, 21, 10);

  switch (ext_5)
  {
  case 0:
    switch (ext_10)
    {
    case 0:
      if (d >> 2 != 0)
      {
        return "ps_cmpu0 " + Alias(d >> 2, "cr") + ", " + Alias(a, "p") + ", " + Alias(b, "p");
      }
      return "ps_cmpu0 " + Alias(a, "p") + ", " + Alias(b, "p");
    case 32:
      if (d >> 2 != 0)
      {
        return "ps_cmpo0 " + Alias(d >> 2, "cr") + ", " + Alias(a, "p") + ", " + Alias(b, "p");
      }
      return "ps_cmpo0 " + Alias(a, "p") + ", " + Alias(b, "p");
    case 64:
      if (d >> 2 != 0)
      {
        return "ps_cmpu1 " + Alias(d >> 2, "cr") + ", " + Alias(a, "p") + ", " + Alias(b, "p");
      }
      return "ps_cmpu1 " + Alias(a, "p") + ", " + Alias(b, "p");
    case 96:
      if (d >> 2 != 0)
      {
        return "ps_cmpo1 " + Alias(d >> 2, "cr") + ", " + Alias(a, "p") + ", " + Alias(b, "p");
      }
      return "ps_cmpo1 " + Alias(a, "p") + ", " + Alias(b, "p");
    default:
      break;
    }
    return "ps_0 ---";
  case 1:
    return "ps_1 ---";
  case 2:
    return "ps_2 ---";
  case 3:
    return "ps_3 ---";
  case 4:
    return "ps_4 ---";
  case 5:
    return "ps_5 ---";
  case 6:
    if (ext_6 == 6)
      return "psq_lx " + Alias(d, "p") + ", r" + TS(a) + ", r" + TS(b) + ", " + TS(w) + ", " +
             Alias(i, "qr");
    if (ext_6 == 38)
      return "psq_lux " + Alias(d, "p") + ", r" + TS(a) + ", r" + TS(b) + ", " + TS(w) + ", " +
             Alias(i, "qr");
    return "ps_6 ---";
  case 7:
    if (ext_6 == 7)
      return "psq_stx " + Alias(d, "p") + ", r" + TS(a) + ", r" + TS(b) + ", " + TS(w) + ", " +
             Alias(i, "qr");
    if (ext_6 == 39)
      return "psq_stux " + Alias(d, "p") + ", r" + TS(a) + ", r" + TS(b) + ", " + TS(w) + ", " +
             Alias(i, "qr");
    return "ps_7 ---";
  case 8:
    switch (ext_10)
    {
    case 40:
      return "ps_neg " + Alias(d, "p") + ", -" + Alias(b, "p");
    case 72:
      return "ps_mr " + Alias(d, "p") + ", " + Alias(b, "p");
    case 136:
      return "ps_nabs " + Alias(d, "p") + ", -|" + Alias(b, "p") + "|";
    case 264:
      return "ps_abs " + Alias(d, "p") + ", |" + Alias(b, "p") + "|";
    default:
      break;
    }
    return "ps_8 ---";
  case 9:
    return "ps_9 ---";
  case 10:
    return "ps_sum0 " + Alias(d, "p") + ", 0=" + Alias(a, "p") + "+" + Alias(b, "p") +
           ", 1=" + Alias(c, "p");
  case 11:
    return "ps_sum1 " + Alias(d, "p") + ", 0=" + Alias(c, "p") + ", 1=" + Alias(a, "p") + "+" +
           Alias(b, "p");
  case 12:
    return "ps_muls0 " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p") + "[0]";
  case 13:
    return "ps_muls1 " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p") + "[1]";
  case 14:
    return "ps_madds0 " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p") + "[0]+" +
           Alias(b, "p");
  case 15:
    return "ps_madds1 " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p") + "[1]+" +
           Alias(b, "p");
  case 16:
    switch (ext_10)
    {
    case 528:
      return "ps_merge00 " + Alias(d, "p") + ", " + Alias(a, "p") + "[0], " + Alias(b, "p") + "[0]";
    case 560:
      return "ps_merge01 " + Alias(d, "p") + ", " + Alias(a, "p") + "[0], " + Alias(b, "p") + "[1]";
    case 592:
      return "ps_merge10 " + Alias(d, "p") + ", " + Alias(a, "p") + "[1], " + Alias(b, "p") + "[0]";
    case 624:
      return "ps_merge11 " + Alias(d, "p") + ", " + Alias(a, "p") + "[1], " + Alias(b, "p") + "[1]";
    default:
      break;
    }
    return "ps_16 ---";
  case 17:
    return "ps_17 ---";
  case 18:
    return "ps_div " + Alias(d, "p") + ", " + Alias(a, "p") + "/" + Alias(b, "p");
  case 19:
    return "ps_19 ---";
  case 20:
    return "ps_sub " + Alias(d, "p") + ", " + Alias(a, "p") + "-" + Alias(b, "p");
  case 21:
    return "ps_add " + Alias(d, "p") + ", " + Alias(a, "p") + "+" + Alias(b, "p");
  case 22:
    if (ext_10 == 1014)
    {
      if (binary % 4)
        return ILL(binary);
      if (d != 0)
        return ILL(binary);
      return "dcbz_l " + Alias(a, "r") + ", " + Alias(b, "r");
    }
    return "ps_22 ---";
  case 23:
    return "ps_sel " + Alias(d, "p") + ", " + Alias(a, "p") + ">=0?" + Alias(c, "p") + ":" +
           Alias(b, "p");
  case 24:
    return "ps_res " + Alias(d, "p") + ", (1/" + Alias(b, "p") + ")";
  case 25:
    return "ps_mul " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p");
  case 26:
    return "ps_rsqrte " + Alias(d, "p") + ", " + Alias(b, "p");
  case 27:
    return "ps_27 ---";
  case 28:
    return "ps_msub " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p") + "-" +
           Alias(b, "p");
  case 29:
    return "ps_madd " + Alias(d, "p") + ", " + Alias(a, "p") + "*" + Alias(c, "p") + "+" +
           Alias(b, "p");
  case 30:
    return "ps_nmsub " + Alias(d, "p") + ", -(" + Alias(a, "p") + "*" + Alias(c, "p") + "-" +
           Alias(b, "p") + ")";
  case 31:
    return "ps_nmadd " + Alias(d, "p") + ", -(" + Alias(a, "p") + "*" + Alias(c, "p") + "+" +
           Alias(b, "p") + ")";
  default:
    ILL(binary);
  }
  return ILL(binary);
}

// handles math operations with immediate values
std::string PowerPCDisassembler::MathImmediate(const u32 binary, std::string mnemonic,
                                               const bool consider_neg, const bool hex)
{
  u32 rt = ValueAtBits(binary, 6, 5);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 si = ValueAtBits(binary, 16, 16);
  if (consider_neg && si > 0x7FFF)
  {
    si += 0xFFFF0000;
  }

  if (mnemonic == "addi" && ra == 0)
  {
    return "li " + Alias(rt, "r") + ", " + TS(si);
  }
  if (mnemonic == "addis" && ra == 0)
  {
    return "lis " + Alias(rt, "r") + ", " + "0x" + TS(si & 0xFFFF, 16, 4);
  }
  if (mnemonic == "ori" && rt == 0 && ra == 0 && si == 0)
  {
    return "nop";
  }
  if (mnemonic.find("add") != std::string::npos && si > 0x7FFF)
  {
    mnemonic.replace(mnemonic.find("add"), 3, "sub");
    si = 0xFFFFFFFF - si + 1;
  }

  if (hex)
    return mnemonic + " " + Alias(ra, "r") + ", " + Alias(rt, "r") + ", 0x" + TS(si, 16, 4);
  return mnemonic + " " + Alias(rt, "r") + ", " + Alias(ra, "r") + ", " + TS(si);
}
// handles memory operations with immediate values
std::string PowerPCDisassembler::MemImmediate(const u32 binary, const std::string& mnemonic,
                                              const std::string& prefix)
{
  u32 rt = ValueAtBits(binary, 6, 5);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 si = ValueAtBits(binary, 16, 16);

  std::string offset = "0x" + TS(si, 16, 4);
  if (si == 0)
    offset = "0";
  if (si >= 0x8000)
    offset = "-0x" + TS(0x8000 - si + 0x8000, 16, 4);

  return mnemonic + " " + Alias(rt, prefix) + ", " + offset + " (" + Alias(ra, "r") + ")";
}
// handles compares with immediate values
std::string PowerPCDisassembler::CompareImmediate(const u32 binary, std::string mnemonic,
                                                  const bool consider_negative)
{
  u32 crd = ValueAtBits(binary, 6, 3);
  u32 expected_zero = ValueAtBits(binary, 9, 1);
  u32 l = ValueAtBits(binary, 10, 1);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 uimm = ValueAtBits(binary, 16, 16);
  if (consider_negative && uimm > 0x7FFF)
    uimm += 0xFFFF0000;

  if (expected_zero)
    return ILL(binary);

  if (l == 0)
  {
    mnemonic += "w";
  }
  else
  {
    mnemonic += "d";
  }

  std::string toReturn = mnemonic + "i ";
  if (crd != 0)
  {
    toReturn += Alias(crd, "cr") + ", ";
  }
  toReturn += Alias(ra, "r") + ", " + TS(uimm);
  return toReturn;
}

// handles all for opcode 16
std::string PowerPCDisassembler::Opcode16Handler(u32 binary, u32 current_instruction_address)
{
  static const char* opcode_16_bo_4[] = {"ge", "le", "ne", "ns"};
  static const char* opcode_16_bo_12[] = {"lt", "gt", "eq", "so"};

  u32 bo = ValueAtBits(binary, 6, 5);
  u32 bi = ValueAtBits(binary, 11, 5);
  u32 bd = ValueAtBits(binary, 16, 14) << 2;
  u32 aa = ValueAtBits(binary, 30, 1);
  u32 lk = ValueAtBits(binary, 31, 1);
  bool show_bi = true;
  bool hide_crf = false;
  bool do_signs = true;

  std::string mnemonic = "b";
  switch (bo)
  {
  case 0:
  case 1:
    mnemonic += "dnzf";
    break;
  case 2:
  case 3:
    mnemonic += "dzf";
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    show_bi = false;
    mnemonic += opcode_16_bo_4[bi & 3];
    break;
  case 8:
  case 9:
    mnemonic += "dnzt";
    break;
  case 10:
  case 11:
    mnemonic += "dzt";
    break;
  case 12:
  case 13:
  case 14:
  case 15:
    show_bi = false;
    mnemonic += opcode_16_bo_12[bi & 3];
    break;
  case 16:
  case 17:
  case 24:
  case 25:
    mnemonic += "dnz";
    show_bi = false;
    hide_crf = true;
    break;
  case 18:
  case 19:
  case 26:
  case 27:
    mnemonic += "dz";
    show_bi = false;
    hide_crf = true;
    break;
  default:
    mnemonic += "c";
    do_signs = false;
    break;
  }
  if (lk)
    mnemonic += "l";
  if (aa)
    mnemonic += "a";

  if (do_signs)
  {
    if ((bo & 0x1) ^ (bd >= 0x8000))
      mnemonic += "+";
    else
      mnemonic += "-";
  }
  else
  {
    mnemonic += " " + TS(bo) + ",";
  }

  std::string offset;
  if (aa)
  {
    if (bd >= 0x8000)
      bd += 0xFFFF0000;
    offset = "->0x" + TS(bd, 16, 8);
  }
  else
  {
    if (current_instruction_address == 0)
    {
      offset = "0x" + TS(bd, 16);
      if (bd >= 0x8000)
        offset = "-0x" + TS(0x8000 - (bd - 0x8000), 16);
    }
    else
    {
      if (bd >= 0x8000)
        bd += 0xFFFF0000;
      offset = "->0x" + TS(current_instruction_address + bd, 16, 8);
    }
  }

  std::string str_crf = "";
  if ((bi >> 2) && !hide_crf)
    str_crf = Alias(bi >> 2, "cr");

  if (show_bi)
    return mnemonic + " " + TS(bi) + " " + offset;
  return mnemonic + " " + str_crf + " " + offset;
}

// handles all for opcode 18
std::string PowerPCDisassembler::Opcode18Handler(const u32 binary,
                                                 const u32 current_instruction_address)
{
  u32 amt = ValueAtBits(binary, 6, 24) << 2;
  u32 aa = ValueAtBits(binary, 30, 1);
  u32 lk = ValueAtBits(binary, 31, 1);
  bool is_negative = false;

  if (amt >= 0x2000000)
  {
    is_negative = true;
  }

  std::string mnemonic = "b";
  if (lk)
    mnemonic += "l";
  if (aa)
    mnemonic += "a";

  if (aa)
  {
    if (is_negative)
      amt += 0xFC000000;
    return mnemonic + " ->0x" + TS(amt, 16, 8);
  }
  else
  {
    if (current_instruction_address == 0)
    {
      if (!is_negative)
        return mnemonic + " 0x" + TS(amt, 16);
      return mnemonic + " -0x" + TS(0x2000000 - (amt - 0x2000000), 16);
    }
    else
    {
      if (is_negative)
        amt += 0xFC000000;
      return mnemonic + " ->0x" + TS(current_instruction_address + amt, 16, 8);
    }
  }
}

// handles all for opcode 18 extended opcode 16 and extended opcode 528
std::string PowerPCDisassembler::Opcode19HandlerExt16(const u32 binary, const std::string& toAdd)
{
  static const char* opcode_19_ext16_bo_4[] = {"bge", "ble", "bne", "bns"};
  static const char* opcode_19_ext16_bo_12[] = {"blt", "bgt", "beq", "bso"};

  u32 bo = ValueAtBits(binary, 6, 5);
  u32 bi = ValueAtBits(binary, 11, 5);
  u32 lk = ValueAtBits(binary, 31, 1);

  bool show_bi = true;
  bool hide_crf = false;
  bool do_signs = true;

  std::string mnemonic;
  switch (bo)
  {
  case 0:
  case 1:
    mnemonic = "bdnzf";
    break;
  case 2:
  case 3:
    mnemonic += "bdzf";
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    mnemonic = opcode_19_ext16_bo_4[bi & 3];
    show_bi = false;
    break;
  case 8:
  case 9:
    mnemonic = "bdnzt";
    break;
  case 10:
  case 11:
    mnemonic = "bdzt";
    break;
  case 12:
  case 13:
  case 14:
  case 15:
    mnemonic = opcode_19_ext16_bo_12[bi & 3];
    show_bi = false;
    break;
  case 16:
  case 17:
  case 24:
  case 25:
    mnemonic += "bdnz";
    show_bi = false;
    hide_crf = true;
    break;
  case 18:
  case 19:
  case 26:
  case 27:
    mnemonic += "bdz";
    show_bi = false;
    hide_crf = true;
    break;
  default:
    mnemonic += "b";
    show_bi = false;
    do_signs = false;
    hide_crf = true;
    break;
  }
  mnemonic += toAdd;
  if (lk)
    mnemonic += "l";

  if (do_signs)
  {
    if (bo & 1)
      mnemonic += "+";
    else
      mnemonic += "-";
  }

  if (show_bi)
    return mnemonic + " " + TS(bi);
  if (!hide_crf && (bi >> 2) != 0)
    return mnemonic + " " + Alias(bi >> 2, "cr");
  return mnemonic;
}

// handles all for opcode 19
std::string PowerPCDisassembler::Opcode19Handler(const u32 binary)
{
  u32 d = ValueAtBits(binary, 6, 5);
  u32 a = ValueAtBits(binary, 11, 5);
  u32 b = ValueAtBits(binary, 16, 5);

  u32 ext_10 = ValueAtBits(binary, 21, 10);

  if (ext_10 != 16 && ext_10 != 528 && ((binary & 1) == 1))
    return ILL(binary);

  switch (ext_10)
  {
  case 0:
    if ((b == 0) && ((d & 3) == 0) && ((a & 3) == 0))
      return "mcrf " + Alias(d >> 2, "cr") + ", " + Alias(a >> 2, "cr");
    return ILL(binary);
  case 16:
    return Opcode19HandlerExt16(binary, "lr");
  case 33:
    if (a == b)
      return "crnot " + TS(d) + ", " + TS(a);
    return "crnor " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 50:
    if (binary == 0x4c000064)
      return "rfi";
    return ILL(binary);
  case 129:
    return "crandc " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 150:
    if (binary == 0x4c00012c)
      return "isync";
    return ILL(binary);
  case 193:
    if (a == b)
      return "crclr " + TS(d) + ", " + TS(a);
    return "crxor " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 225:
    return "crnand " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 257:
    return "crand " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 289:
    if (a == b)
      return "crset " + TS(d) + ", " + TS(a);
    return "creqv " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 417:
    return "crorc " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 449:
    if (a == b)
      return "crmove " + TS(d) + ", " + TS(a);
    return "cror " + TS(d) + ", " + TS(a) + ", " + TS(b);
  case 528:
    return Opcode19HandlerExt16(binary, "ctr");
  default:
    return ILL(binary);
  }
}

// handles "RLW" instructions
std::string PowerPCDisassembler::RLWInstruction(const u32 binary, const std::string& mnemonic)
{
  u32 rs = ValueAtBits(binary, 6, 5);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 sh = ValueAtBits(binary, 16, 5);
  u32 mb = ValueAtBits(binary, 21, 5);
  u32 me = ValueAtBits(binary, 26, 5);

  std::string period = (binary & 1) ? ". " : " ";
  if (mnemonic != "rlwnm")
    return mnemonic + period + Alias(ra, "r") + ", " + Alias(rs, "r") + ", " + TS(sh) + ", " +
           TS(mb) + ", " + TS(me);
  return mnemonic + period + Alias(ra, "r") + ", " + Alias(rs, "r") + ", r" + TS(sh) + ", " +
         TS(mb) + ", " + TS(me);
}

// handles all for opcode 31. This took ages to do
std::string PowerPCDisassembler::Opcode31Handler(const u32 binary)
{
  u32 d = ValueAtBits(binary, 6, 5);
  u32 a = ValueAtBits(binary, 11, 5);
  u32 b = ValueAtBits(binary, 16, 5);

  u32 ext_10 = ValueAtBits(binary, 21, 10);

  u32 cmp_type = ValueAtBits(binary, 10, 1);
  u32 add_letter_o = ValueAtBits(binary, 21, 1);
  u32 rc = ValueAtBits(binary, 31, 1);
  u32 mtcrf_amt = ValueAtBits(binary, 12, 8);
  u32 mtsr_amt = ValueAtBits(binary, 12, 4);
  std::string mnemonic;

  switch (ext_10)
  {
  case 412:
    return MA("orc", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 373:
    if (rc)
      return ILL(binary);
    return "lwaux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 20:
    if (rc)
      return ILL(binary);
    return "lwarx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 534:
    if (rc)
      return ILL(binary);
    return "lwbrx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 597:
    if (rc)
      return ILL(binary);
    return "lswi " + Alias(d, "r") + ", " + Alias(a, "r") + "," + TS(b);
  case 279:
    if (rc)
      return ILL(binary);
    return "lhzx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 311:
    if (rc)
      return ILL(binary);
    return "lhzux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 790:
    if (rc)
      return ILL(binary);
    return "lhbrx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 343:
    if (rc)
      return ILL(binary);
    return "lhax " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 375:
    if (rc)
      return ILL(binary);
    return "lhaux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 535:
    return "lfsx " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 567:
    return "lfsux " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 599:
    return "lfdx " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 631:
    return "lfdux " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 87:
    if (rc)
      return ILL(binary);
    return "lbzx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 119:
    if (rc)
      return ILL(binary);
    return "lbzux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 21:
    if (rc)
      return ILL(binary);
    return "ldx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 53:
    if (rc)
      return ILL(binary);
    return "ldux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 23:
    if (rc)
      return ILL(binary);
    return "lwzx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 55:
    if (rc)
      return ILL(binary);
    return "lwzux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 84:
    if (rc)
      return ILL(binary);
    return "ldarx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 341:
    if (rc)
      return ILL(binary);
    return "lwax " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 533:
    if (rc)
      return ILL(binary);
    return "lswx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 489:
  case 1001:
    return MA("divd", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 491:
  case 1003:
    return MA("divw", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 457:
  case 969:
    return MA("divdu", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 459:
  case 971:
    return MA("divwu", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 246:
    if (rc || d)
      return ILL(binary);
    return "dcbtst " + Alias(a, "r") + ", " + Alias(b, "r");
  case 278:
    if (rc || d)
      return ILL(binary);
    return "dcbt " + Alias(a, "r") + ", " + Alias(b, "r");
  case 470:
    if (rc || d)
      return ILL(binary);
    return "dcbi " + Alias(a, "r") + ", " + Alias(b, "r");
  case 1014:
    if (rc || d)
      return ILL(binary);
    return "dcbz " + Alias(a, "r") + ", " + Alias(b, "r");
  case 54:
    if (rc || d)
      return ILL(binary);
    return "dcbst " + Alias(a, "r") + ", " + Alias(b, "r");
  case 86:
    if (rc || d)
      return ILL(binary);
    return "dcbf " + Alias(a, "r") + ", " + Alias(b, "r");
  case 9:
    return MA("mulhdu", rc) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 11:
    return MA("mulhwu", rc) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 73:
    return MA("mulhd", rc) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 75:
    return MA("mulhw", rc) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 233:
  case 745:
    return MA("mulld", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 235:
  case 747:
    return MA("mullw", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 19:
    if (rc || a || b)
      return ILL(binary);
    return "mfcr " + Alias(d, "r");
  case 444:
    if (d == b)
      mnemonic = "mr";
    else
      mnemonic = "or";

    if (d == b)
      return MA(mnemonic, rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
    return MA(mnemonic, rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
    ;
  case 83:
    if (rc || a || b)
      return ILL(binary);
    return "mfmsr " + Alias(d, "r");
  case 144:
    if (rc || ((binary >> 11) & 1) || ((binary >> 20) & 1))
      return ILL(binary);
    if (mtcrf_amt == 255)
      return "mtcr " + Alias(d, "r");
    return "mtcrf 0x" + TS(mtcrf_amt, 16, 2) + "," + Alias(d, "r");
  case 146:
    if (rc || a || b)
      return ILL(binary);
    return "mtmsr " + Alias(d, "r");
  case 210:
    if (rc || (a >> 4) || b)
      return ILL(binary);
    return "mtsr " + TS(mtsr_amt) + ", " + Alias(d, "r");
  case 242:
    if (rc || a)
      return ILL(binary);
    return "mtsrin " + Alias(d, "r") + ", " + Alias(b, "r");
  case 339:
    if (rc)
      return ILL(binary);
    if (((b << 5) | a) == 1)
      return "mfxer " + Alias(d, "r");
    if (((b << 5) | a) == 8)
      return "mflr " + Alias(d, "r");
    if (((b << 5) | a) == 9)
      return "mfctr " + Alias(d, "r");
    return "mfspr " + Alias(d, "r") + ", " + Alias((b << 5) | a);
  case 467:
    if (rc)
      return ILL(binary);
    if (((b << 5) | a) == 1)
      return "mtxer " + Alias(d, "r");
    if (((b << 5) | a) == 8)
      return "mtlr " + Alias(d, "r");
    if (((b << 5) | a) == 9)
      return "mtctr " + Alias(d, "r");
    return "mtspr " + Alias((b << 5) | a) + ", " + Alias(d, "r");
  case 512:
    if (rc || a || b || (d & 3))
      return ILL(binary);
    return "mcrxr " + Alias(d >> 2, "cr");
  case 595:
    if (rc || b || (a >> 4))
      return ILL(binary);
    return "mfsr " + Alias(d, "r") + ", " + TS(a & 0xF);
  case 659:
    if (rc || a)
      return ILL(binary);
    return "mfsrin " + Alias(d, "r") + ", " + Alias(b, "r");
  case 371:
    if (rc)
      return ILL(binary);
    if (((b << 5) | a) == 268)
      return "mftbl " + Alias(d, "r");
    if (((b << 5) | a) == 269)
      return "mftbu " + Alias(d, "r");
    return "mftb " + Alias(d, "r") + "," + TS((b << 5) | a);
  case 476:
    return MA("nand", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 124:
    if (d == b)
      mnemonic = "not";
    else
      mnemonic = "nor";

    if (d == b)
      return MA(mnemonic, rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
    return MA(mnemonic, rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 104:
  case 616:
    if (b)
      return ILL(binary);
    return MA("neg", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r");
  case 0:
    if (rc || (d & 2))
      return ILL(binary);
    mnemonic = "cmpw ";
    if (cmp_type)
    {
      mnemonic = "cmpd ";
    }
    if (d >> 2)
      return mnemonic + Alias(d >> 2, "cr") + "," + Alias(a, "r") + ", " + Alias(b, "r");
    return mnemonic + Alias(a, "r") + ", " + Alias(b, "r");
  case 32:
    if (rc || (d & 2))
      return ILL(binary);
    mnemonic = "cmplw ";
    if (cmp_type)
    {
      mnemonic = "cmpld ";
    }
    if (d >> 2)
      return mnemonic + Alias(d >> 2, "cr") + "," + Alias(a, "r") + ", " + Alias(b, "r");
    return mnemonic + Alias(a, "r") + ", " + Alias(b, "r");
  case 26:
    if (b)
      return ILL(binary);
    return MA("cntlzw", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
  case 58:
    if (b)
      return ILL(binary);
    return MA("cntlzd", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
  case 266:
  case 778:
    return MA("add", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 10:
  case 522:
    return MA("addc", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 138:
  case 650:
    return MA("adde", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 234:
  case 746:
    if (b)
      return ILL(binary);
    return MA("addme", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r");
  case 202:
  case 714:
    if (b)
      return ILL(binary);
    return MA("addze", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r");
  case 28:
    return MA("and", rc, add_letter_o) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " +
           Alias(b, "r");
  case 60:
    return MA("andc", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 40:
  case 552:
    return MA("sub", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(b, "r") + ", " +
           Alias(a, "r");
  case 8:
  case 520:
    return MA("subc", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(b, "r") + ", " +
           Alias(a, "r");
  case 136:
  case 648:
    return MA("subfe", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " +
           Alias(b, "r");
  case 232:
  case 744:
    if (b)
      return ILL(binary);
    return MA("subfme", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r");
  case 200:
  case 712:
    if (b)
      return ILL(binary);
    return MA("subfze", rc, add_letter_o) + " " + Alias(d, "r") + ", " + Alias(a, "r");
  case 498:
    if (rc || d || a || b)
      return ILL(binary);
    return "slbia";
  case 598:
    if (rc || d || a || b)
      return ILL(binary);
    return "sync";
  case 149:
    if (rc)
      return ILL(binary);
    return "stdx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 181:
    if (rc)
      return ILL(binary);
    return "stdux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 183:
    if (rc)
      return ILL(binary);
    return "stwux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 151:
    if (rc)
      return ILL(binary);
    return "stwx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 439:
    if (rc)
      return ILL(binary);
    return "sthux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 407:
    if (rc)
      return ILL(binary);
    return "sthx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 247:
    if (rc)
      return ILL(binary);
    return "stbux " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 215:
    if (rc)
      return ILL(binary);
    return "stbx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 150:
    if (!rc)
      return ILL(binary);
    return "stwcx. " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 662:
    if (rc)
      return ILL(binary);
    return "stwbrx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 214:
    if (!rc)
      return ILL(binary);
    return "stdcx. " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 661:
    if (rc)
      return ILL(binary);
    return "stswx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 663:
    return "stfsx " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 695:
    return "stfsux " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 725:
    if (rc)
      return ILL(binary);
    return "stswi " + Alias(d, "r") + ", " + Alias(a, "r") + "," + TS(b);
  case 727:
    return "stfdx " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 759:
    return "stfdux " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 918:
    if (rc)
      return ILL(binary);
    return "sthbrx " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 983:
    return "stfiwx " + Alias(d, "f") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 24:
    return MA("slw", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 27:
    return MA("sld", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 539:
    return MA("srd", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 536:
    return MA("srw", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 824:
    return MA("srawi", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + "," + TS(b);
  case 434:
    if (rc || d || a)
      return ILL(binary);
    return "slbie " + Alias(b, "r");
  case 792:
    return MA("sraw", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 794:
    return MA("srad", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 68:
    if (TrapTOStr(d) == "" && d != 31)
      return ILL(binary);
    if (d == 31)
      return "td " + TS(d) + ",0,0";
    return TX(binary, "d");
  case 4:
    if (d == 31 && !rc)
      return "trap";
    if (TrapTOStr(d) == "" || rc)
      return ILL(binary);
    return TX(binary, "w");
  case 306:
    if (rc | d | a)
      return ILL(binary);
    return "tlbie " + Alias(b, "r");
  case 566:
    if (rc | d | a | b)
      return ILL(binary);
    return "tlbsync";
  case 370:
    if (rc | d | a | b)
      return ILL(binary);
    return "tlbia";
  case 316:
    return MA("xor", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 954:
    if (b)
      return ILL(binary);
    return MA("extsb", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
  case 922:
    if (b)
      return ILL(binary);
    return MA("extsh", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
  case 986:
    if (b)
      return ILL(binary);
    return MA("extsw", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r");
  case 284:
    return MA("eqv", rc) + " " + Alias(a, "r") + ", " + Alias(d, "r") + ", " + Alias(b, "r");
  case 310:
    if (rc)
      return ILL(binary);
    return MA("eciwx", rc) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 438:
    if (rc)
      return ILL(binary);
    return MA("ecowx", rc) + " " + Alias(d, "r") + ", " + Alias(a, "r") + ", " + Alias(b, "r");
  case 854:
    if (rc | d | a | b)
      return ILL(binary);
    return "eieio";
  case 982:
    if (rc | d)
      return ILL(binary);
    return "icbi " + Alias(a, "r") + ", " + Alias(b, "r");
  default:
    return ILL(binary);
  }
}

// handles psq instructions
std::string PowerPCDisassembler::PsqInstruction(const u32 binary, const std::string& mnemonic)
{
  u32 frd = ValueAtBits(binary, 6, 5);
  u32 ra = ValueAtBits(binary, 11, 5);
  u32 w = ValueAtBits(binary, 16, 1);
  u32 i = ValueAtBits(binary, 17, 3);
  u32 d = ValueAtBits(binary, 20, 12);

  std::string offset = "0x" + TS(d, 16, 4);
  if (d == 0)
    offset = "0";
  if (d >= 0x800)
    offset = "-0x" + TS(0x800 - d + 0x800, 16, 4);

  return mnemonic + " " + Alias(frd, "p") + ", " + offset + "(r" + TS(ra) + "), " + TS(w) + ", " +
         Alias(i, "qr");
}

// handles all for opcode 59
std::string PowerPCDisassembler::Opcode59Handler(const u32 binary)
{
  u32 d = ValueAtBits(binary, 6, 5);
  u32 a = ValueAtBits(binary, 11, 5);
  u32 b = ValueAtBits(binary, 16, 5);
  u32 c = ValueAtBits(binary, 21, 5);
  u32 ext_5 = ValueAtBits(binary, 26, 5);
  u32 rc = ValueAtBits(binary, 31, 1);
  std::string mnemonic;

  switch (ext_5)
  {
  case 18:
    if (c)
      return ILL(binary);
    return MA("fdivs", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
  case 20:
    if (c)
      return ILL(binary);
    return MA("fsubs", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
  case 21:
    if (c)
      return ILL(binary);
    return MA("fadds", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
  case 22:
    if (a || c)
      return ILL(binary);
    return MA("fsqrts", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 24:
    if (a || c)
      return ILL(binary);
    return MA("fres", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 25:
    if (b)
      return ILL(binary);
    return MA("fmuls", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f");
  case 28:
    return MA("fmsubs", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 29:
    return MA("fmadds", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 30:
    return MA("fnmsubs", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 31:
    return MA("fnmadds", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  default:
    return ILL(binary);
  }
}

// handles all for opcode 63
std::string PowerPCDisassembler::Opcode63Handler(const u32 binary)
{
  u32 d = ValueAtBits(binary, 6, 5);
  u32 a = ValueAtBits(binary, 11, 5);
  u32 b = ValueAtBits(binary, 16, 5);
  u32 c = ValueAtBits(binary, 21, 5);
  u32 ext_5 = ValueAtBits(binary, 26, 5);
  u32 ext_10 = ValueAtBits(binary, 21, 10);
  u32 rc = ValueAtBits(binary, 31, 1);
  std::string mnemonic;

  switch (ext_5)
  {
  case 0:
    switch (ext_10)
    {
    case 0:
      if ((d & 3) || rc)
        return ILL(binary);
      return "fcmpu " + Alias(d >> 2, "cr") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
    case 32:
      if ((d & 3) || rc)
        return ILL(binary);
      return "fcmpo " + Alias(d >> 2, "cr") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
    case 64:
      if ((d & 3) || (a & 3) || b || rc)
        return ILL(binary);
      return "mcrfs " + Alias(d >> 2, "cr") + ", " + Alias(a >> 2, "cr");
    default:
      return ILL(binary);
    }
  case 6:
    switch (ext_10)
    {
    case 38:
      if (a || b)
        return ILL(binary);
      return MA("mtfsb1", rc) + " " + TS(d);
    case 70:
      if (a || b)
        return ILL(binary);
      return MA("mtfsb0", rc) + " " + TS(d);
    case 134:
      if ((a & 0xF) || (b & 1) || (d & 3))
        return ILL(binary);
      return MA("mtfsfi", rc) + " " + Alias(d >> 2, "cr") + "," + TS(b >> 1);
    default:
      return ILL(binary);
    }
  case 7:
    switch (ext_10)
    {
    case 583:
      if (a || b)
        return ILL(binary);
      return MA("mffs", rc) + " " + Alias(d, "r");
    case 711:
      if ((d >> 4) || (a & 1))
        return ILL(binary);
      return MA("mtfsf", rc) + " 0x" + TS(ValueAtBits(binary, 7, 8), 16) + ", " + Alias(b, "f");
    default:
      return ILL(binary);
    }
  case 8:
    switch (ext_10)
    {
    case 72:
      return MA("fmr", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
    case 40:
      return MA("fneg", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
    case 264:
      return MA("fabs", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
    case 136:
      return MA("fnabs", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
    default:
      return ILL(binary);
    }
  case 12:
    if (ext_10 != 12 || a)
      return ILL(binary);
    return MA("frsp", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 14:
    if (ext_10 != 14 || a)
      return ILL(binary);
    return MA("fctiw", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 15:
    if (ext_10 != 15 || a)
      return ILL(binary);
    return MA("fctiwz", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 18:
    if (c)
      return ILL(binary);
    return MA("fdiv", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
  case 20:
    if (c)
      return ILL(binary);
    return MA("fsub", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
  case 21:
    if (c)
      return ILL(binary);
    return MA("fadd", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(b, "f");
  case 22:
    if (a || c)
      return ILL(binary);
    return MA("fsqrt", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 23:
    return MA("fsel", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 25:
    if (b)
      return ILL(binary);
    return MA("fmul", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f");
  case 26:
    if (a || c)
      return ILL(binary);
    return MA("frsqrte", rc) + " " + Alias(d, "f") + ", " + Alias(b, "f");
  case 28:
    return MA("fmsub", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 29:
    return MA("fmadd", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 30:
    return MA("fnmsub", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  case 31:
    return MA("fnmadd", rc) + " " + Alias(d, "f") + ", " + Alias(a, "f") + ", " + Alias(c, "f") +
           ", " + Alias(b, "f");
  default:
    return ILL(binary);
  }
}

std::string PowerPCDisassembler::PPCDisassemble(u32 binary, u32 current_instruction_address)
{
  u32 opcode = ValueAtBits(binary, 0, 6);

  switch (opcode)
  {
  case 0:
  case 1:
  case 5:
  case 6:
  case 9:
  case 22:
  case 30:
    if (binary == 0)
      return " ---";
    return ILL(binary);
  case 2:
    return TXI(binary, "d");
  case 3:
    return TXI(binary, "w");
  case 4:
    return opcode4Handler(binary);
  case 7:
    return MathImmediate(binary, "mulli");
  case 8:
    return MathImmediate(binary, "subfic");
  case 10:
    return CompareImmediate(binary, "cmpl");
  case 11:
    return CompareImmediate(binary, "cmp", true);
  case 12:
    return MathImmediate(binary, "addic");
  case 13:
    return MathImmediate(binary, "addic.");
  case 14:
    return MathImmediate(binary, "addi");
  case 15:
    return MathImmediate(binary, "addis");
  case 16:
    return Opcode16Handler(binary, current_instruction_address);
  case 17:
    if (binary == 0x44000002)
      return "sc";
    return ILL(binary);
  case 18:
    return Opcode18Handler(binary, current_instruction_address);
  case 19:
    return Opcode19Handler(binary);
  case 20:
    return RLWInstruction(binary, "rlwimi");
  case 21:
    return RLWInstruction(binary, "rlwinm");
  case 23:
    return RLWInstruction(binary, "rlwnm");
  case 24:
    return MathImmediate(binary, "ori", false, true);
  case 25:
    return MathImmediate(binary, "oris", false, true);
  case 26:
    return MathImmediate(binary, "xori", false, true);
  case 27:
    return MathImmediate(binary, "xoris", false, true);
  case 28:
    return MathImmediate(binary, "andi.", false, true);
  case 29:
    return MathImmediate(binary, "andis.", false, true);
  case 31:
    return Opcode31Handler(binary);
  case 32:
    return MemImmediate(binary, "lwz");
  case 33:
    return MemImmediate(binary, "lwzu");
  case 34:
    return MemImmediate(binary, "lbz");
  case 35:
    return MemImmediate(binary, "lbzu");
  case 36:
    return MemImmediate(binary, "stw");
  case 37:
    return MemImmediate(binary, "stwu");
  case 38:
    return MemImmediate(binary, "stb");
  case 39:
    return MemImmediate(binary, "stbu");
  case 40:
    return MemImmediate(binary, "lhz");
  case 41:
    return MemImmediate(binary, "lhzu");
  case 42:
    return MemImmediate(binary, "lha");
  case 43:
    return MemImmediate(binary, "lhau");
  case 44:
    return MemImmediate(binary, "sth");
  case 45:
    return MemImmediate(binary, "sthu");
  case 46:
    return MemImmediate(binary, "lmw");
  case 47:
    return MemImmediate(binary, "stmw");
  case 48:
    return MemImmediate(binary, "lfs", "f");
  case 49:
    return MemImmediate(binary, "lfsu", "f");
  case 50:
    return MemImmediate(binary, "lfd", "f");
  case 51:
    return MemImmediate(binary, "lfdu", "f");
  case 52:
    return MemImmediate(binary, "stfs", "f");
  case 53:
    return MemImmediate(binary, "stfsu", "f");
  case 54:
    return MemImmediate(binary, "stfd", "f");
  case 55:
    return MemImmediate(binary, "stfdu", "f");
  case 56:
    return PsqInstruction(binary, "psq_l");
  case 57:
    return PsqInstruction(binary, "psq_lu");
  case 58:
    if ((binary & 0x3) == 0)
      return MemImmediate(binary & 0xFFFFFFFC, "ld");
    if ((binary & 0x3) == 1)
      return MemImmediate(binary & 0xFFFFFFFC, "ldu");
    if ((binary & 0x3) == 2)
      return MemImmediate(binary & 0xFFFFFFFC, "lwa");
    return ILL(binary);
  case 59:
    return Opcode59Handler(binary);
  case 60:
    return PsqInstruction(binary, "psq_st");
  case 61:
    return PsqInstruction(binary, "psq_stu");
  case 62:
    if ((binary & 0x3) == 0)
      return MemImmediate(binary & 0xFFFFFFFC, "std");
    if ((binary & 0x3) == 1)
      return MemImmediate(binary & 0xFFFFFFFC, "stdu");
    return ILL(binary);
  case 63:
    return Opcode63Handler(binary);
  default:
    return "ERROR. IMPOSSIBLE OPCODE REACHED";
  }
}

}  // namespace Common