// -------------------------------------------------------------------------------------------
//
//                   Gamecube Power PC Assembler (written by ModSault)
//
//  How it works:
//    - PPCAssemble takes a string of a PowerPC instruction and returns a big endian 32-bit
//        integer representing the assembled instruction.
//
//    - It first sets the whole string to lowercase to avoid edge cases
//    - It then tokenizes the instruction into its parts (mnemonic and operands)
//          - i.e. "subi r0, sp, 0x10" becomes ["subi", "r0", "sp", "0x10", "", "", "", ""]
//          - array is always size 8 for simplicity. Its padded with empty strings if the
//            string isn't 8 tokens of length
//    - Each token is then converted to a number. Registers and aliases are converted to
//        their respective numbers (i.e. "sp" becomes 1, "lr" becomes 8, etc). Numbers in
//        decimal, hex (0x), or binary (0b) are converted to their respective values.
//          - i.e. "r0" becomes 0, "rtoc" becomes 2, "0x10" becomes 16, etc.
//          - i.e. ["subi", "r0", "sp", "0x10", "", "", "", ""]
//                becomes [0, 0, 1, 16, 0, 0, 0, 0]
//    - Based on the mnemonic (first token), the instruction is assembled using the
//        numbers from the previous step. One function (MI) handles all instruction types.
//        It requires an array of values and number of bits they take up in the instruction.
//          - i.e. "subi r0, sp, 0x10" becomes:
//             opcode (6 bits) | rA (5 bits) | rS (5 bits) | SIMM (16 bits)
//             which is represented as:
//             MI({ {opcode, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16} })
//          - Instructions that end with a period, +, or -, have it removed and
//             a boolean set to true. So "addi." becomes "addi" with rc = 1.
//    - The assembled instruction is then returned as a big endian 32-bit integer.
//
//   -----------------------------------------------------------------------------------------

#include "PowerPCAssembler.h"
#include <string>
#include <vector>
#include "../../Common/CommonTypes.h"

namespace Common
{

// FNV-1a hash for strings in switch case statements. Fast, simple,
// and no need for an include statement
constexpr u32 FNV1A(const char* str, u32 hash = 0x811C9DC5)
{
  while (*str)
  {
    hash ^= static_cast<u32>(*str++);
    hash *= 16777619;
  }
  return hash;
}
// HsCp - hash string at compile time
constexpr u32 HsCp(const char* str)
{
  return FNV1A(str);
}
// HsRn - hash string at runtime
u32 PowerPCAssembler::HsRn(const std::string& str)
{
  return FNV1A(str.c_str());
}

// tokenizes string of the assemble instruction into its parts.
// i.e. "subi r0, sp, 0x10" becomes ["subi", "r0", "sp", "0x10", "", "", "", ""]
// -
// str assumed all lowercase and no leading or trailing spaces. All if statements
// are for edge cases for how PowerpcDisassembler formats instructions like ps_sel
std::vector<std::string> PowerPCAssembler::Tokenize(const std::string& user_str)
{
  const std::string delimiters = " ,\t\n+/*>:()\0";
  std::vector<std::string> tokens;
  size_t start = user_str.find_first_not_of(delimiters);
  size_t end = (size_t)(-1);

  while (start != std::string::npos)
  {
    end = user_str.find_first_of(delimiters, start);

    std::string toAdd = user_str.substr(start, end - start);
    if (tokens.size() == 0 && end != std::string::npos && end < user_str.size() &&
        user_str[end] == '-')
    {
      // add "-" to end of mnemonic if present
      toAdd += "-";
    }
    if (tokens.size() == 0 && end != std::string::npos && end < user_str.size() &&
        user_str[end] == '+')
    {
      // add "+" to end of mnemonic if present
      toAdd += "+";
    }
    if (toAdd.find("=") != std::string::npos)
    {
      // take right side of "="
      toAdd = toAdd.substr(toAdd.find("=") + 1);
    }
    if (toAdd.find("?") != std::string::npos)
    {
      // take right side of "?"
      toAdd = toAdd.substr(toAdd.find("?") + 1);
    }
    if (toAdd.find("[") != std::string::npos)
    {
      // take left side of "["
      toAdd = toAdd.substr(0, toAdd.find("["));
    }
    if (toAdd.find("-") != std::string::npos && toAdd.find("-") != 0 &&
        toAdd.find("-") != toAdd.size() - 1)
    {
      // if "-" in middle of delimiters, add both segments separately
      // inside this if statement only the left half is added, the right half is added later
      tokens.push_back(toAdd.substr(0, toAdd.find("-")));
      toAdd = toAdd.substr(toAdd.find("-") + 1);
    }
    // the below line handles case of "ps_res p0, (1/p0)", specifically the "1/" part
    bool isOneOverValue =
        (user_str.find("1/") != std::string::npos && user_str.find("1/") == start);
    if (!isOneOverValue && toAdd != "-" && toAdd.size() > 0)
    {
      // add if valid token
      tokens.push_back(toAdd);
    }

    start = user_str.find_first_not_of(delimiters, end);
  }

  if (tokens.size() < 8)
  {
    // prevents potential out of bounds errors by forcing its size to 8
    tokens.resize(8, "");
  }
  return tokens;
}

// this table was pulled from PowerpcDisassembler.cpp and converted to look for strings
// and return an int instead of the other way around.
u32 PowerPCAssembler::StringAliasToNumber(const std::string& token)
{
  switch (HsRn(token))
  {
  case HsCp("sp"):
    return 1;
  case HsCp("rtoc"):
    return 2;
  case HsCp("xer"):
    return 1;
  case HsCp("lr"):
    return 8;
  case HsCp("ctr"):
    return 9;
  case HsCp("dsisr"):
    return 18;
  case HsCp("dar"):
    return 19;
  case HsCp("dec"):
    return 22;
  case HsCp("sdr1"):
    return 25;
  case HsCp("srr0"):
    return 26;
  case HsCp("srr1"):
    return 27;
  case HsCp("tblr"):
    return 268;
  case HsCp("tbur"):
    return 269;
  case HsCp("sprg0"):
    return 272;
  case HsCp("sprg1"):
    return 273;
  case HsCp("sprg2"):
    return 274;
  case HsCp("sprg3"):
    return 275;
  case HsCp("ear"):
    return 282;
  case HsCp("tblw"):
    return 284;
  case HsCp("tbuw"):
    return 285;
  case HsCp("pvr"):
    return 287;
  case HsCp("ibat0u"):
    return 528;
  case HsCp("ibat0l"):
    return 529;
  case HsCp("ibat1u"):
    return 530;
  case HsCp("ibat1l"):
    return 531;
  case HsCp("ibat2u"):
    return 532;
  case HsCp("ibat2l"):
    return 533;
  case HsCp("ibat3u"):
    return 534;
  case HsCp("ibat3l"):
    return 535;
  case HsCp("dbat0u"):
    return 536;
  case HsCp("dbat0l"):
    return 537;
  case HsCp("dbat1u"):
    return 538;
  case HsCp("dbat1l"):
    return 539;
  case HsCp("dbat2u"):
    return 540;
  case HsCp("dbat2l"):
    return 541;
  case HsCp("dbat3u"):
    return 542;
  case HsCp("dbat3l"):
    return 543;
  case HsCp("ibat4u"):
    return 560;
  case HsCp("ibat4l"):
    return 561;
  case HsCp("ibat5u"):
    return 562;
  case HsCp("ibat5l"):
    return 563;
  case HsCp("ibat6u"):
    return 564;
  case HsCp("ibat6l"):
    return 565;
  case HsCp("ibat7u"):
    return 566;
  case HsCp("ibat7l"):
    return 567;
  case HsCp("dbat4u"):
    return 568;
  case HsCp("dbat4l"):
    return 569;
  case HsCp("dbat5u"):
    return 570;
  case HsCp("dbat5l"):
    return 571;
  case HsCp("dbat6u"):
    return 572;
  case HsCp("dbat6l"):
    return 573;
  case HsCp("dbat7u"):
    return 574;
  case HsCp("dbat7l"):
    return 575;
  case HsCp("gqr0"):
    return 912;
  case HsCp("gqr1"):
    return 913;
  case HsCp("gqr2"):
    return 914;
  case HsCp("gqr3"):
    return 915;
  case HsCp("gqr4"):
    return 916;
  case HsCp("gqr5"):
    return 917;
  case HsCp("gqr6"):
    return 918;
  case HsCp("gqr7"):
    return 919;
  case HsCp("hid2"):
    return 920;
  case HsCp("wpar"):
    return 921;
  case HsCp("dmau"):
    return 922;
  case HsCp("dmal"):
    return 923;
  case HsCp("ecid_u"):
    return 924;
  case HsCp("ecid_m"):
    return 925;
  case HsCp("ecid_l"):
    return 926;
  case HsCp("ummcr0"):
    return 936;
  case HsCp("upmc1"):
    return 937;
  case HsCp("upmc2"):
    return 938;
  case HsCp("usia"):
    return 939;
  case HsCp("ummcr1"):
    return 940;
  case HsCp("upmc3"):
    return 941;
  case HsCp("upmc4"):
    return 942;
  case HsCp("usda"):
    return 943;
  case HsCp("mmcr0"):
    return 952;
  case HsCp("pmc1"):
    return 953;
  case HsCp("pmc2"):
    return 954;
  case HsCp("sia"):
    return 955;
  case HsCp("mmcr1"):
    return 956;
  case HsCp("pmc3"):
    return 957;
  case HsCp("pmc4"):
    return 958;
  case HsCp("sda"):
    return 959;
  case HsCp("hid0"):
    return 1008;
  case HsCp("hid1"):
    return 1009;
  case HsCp("iabr"):
    return 1010;
  case HsCp("hid4"):
    return 1011;
  case HsCp("tdcl"):
    return 1012;
  case HsCp("dabr"):
    return 1013;
  case HsCp("l2cr"):
    return 1017;
  case HsCp("tdch"):
    return 1018;
  case HsCp("ictc"):
    return 1019;
  case HsCp("thrm1"):
    return 1020;
  case HsCp("thrm2"):
    return 1021;
  case HsCp("thrm3"):
    return 1022;
  default:
    return 0;
  }
}

// converts a string to a value. So "r0" becomes 0, "sp" becomes 1, "0x10" becomes 16, etc.
// -
// str assumed all lowercase and no leading or trailing spaces
u32 PowerPCAssembler::StringToNumber(const std::string& token, bool consider_alias)
{
  // check if token (aka str) is a valid alias
  if (consider_alias && StringAliasToNumber(token))
    return StringAliasToNumber(token);

  unsigned char mode = 10;
  size_t startIndex = 0;
  bool isNegative = false;
  if (token[startIndex] == '-')
  {
    startIndex += 1;
    isNegative = true;
  }
  if (token.find("0x") == startIndex)
  {
    startIndex += 2;
    mode = 16;
  }
  else if (token.find("0b") == startIndex)
  {
    startIndex += 2;
    mode = 2;
  }

  u32 toReturn = 0;
  std::string str_to_look_at = token.substr(startIndex);
  for (size_t i = 0; i < str_to_look_at.length(); ++i)
  {
    const char c = str_to_look_at[i];
    if (mode == 2)
    {
      if (c == '0' || c == '1')
      {
        toReturn = (toReturn * mode) + (c - '0');
      }
    }
    else
    {
      if (c >= '0' && c <= '9')
      {
        toReturn = (toReturn * mode) + (c - '0');
      }
      else if (mode == 16 && c >= 'a' && c <= 'f')
      {
        toReturn = (toReturn * mode) + (c - 'a' + 10);
      }
      else
      {
        isNegative = false;
      }
    }
  }
  // flip sign of returned value using (doing it like this avoids warning on unsigned int)
  if (isNegative)
  {
    toReturn = 0xFFFFFFFF - toReturn + 1;
  }
  return toReturn;
}

// mi - Make Instruction
// Takes a list of the above struct and makes a powerpc instruction out of it.
u32 PowerPCAssembler::MI(std::initializer_list<Common::PowerPCAssembler::MIStruct> all_parts)
{
  u32 toReturn = 0;
  u8 currentBit = 32;
  for (size_t i = 0; i < all_parts.size(); ++i)
  {
    const Common::PowerPCAssembler::MIStruct part = *(all_parts.begin() + i);
    currentBit -= part.sizeInBits;
    toReturn |= (part.value & ((1 << part.sizeInBits) - 1)) << currentBit;
  }
  return toReturn;
}

// simplify branches statements. Certain characters in the mnemonic affect the value of certain
// bit fields. These functions return the proper values based on the mnemonic string.
u32 PowerPCAssembler::EndX(const std::string& mnemonic, const char check_char)
{
  if (mnemonic.size() < 1)
    return false;
  return (mnemonic[mnemonic.size() - 1] == check_char);
}
u32 PowerPCAssembler::EndA(const std::string& mnemonic)
{
  return EndX(mnemonic, 'a');
}
u32 PowerPCAssembler::EndD(const std::string& mnemonic)
{
  return EndX(mnemonic, 'd');
}
u32 PowerPCAssembler::EndO(const std::string& mnemonic)
{
  return EndX(mnemonic, 'o');
}
u32 PowerPCAssembler::EndDI(const std::string& mnemonic)
{
  if (mnemonic.size() < 2)
    return false;
  return (mnemonic[mnemonic.size() - 2] == 'd' && mnemonic[mnemonic.size() - 1] == 'i');
}
u32 PowerPCAssembler::EndL(const std::string& mnemonic)
{
  if (mnemonic.size() < 2)
    return false;
  return ((mnemonic[mnemonic.size() - 2] == 'l' && mnemonic[mnemonic.size() - 1] == 'a') ||
          mnemonic[mnemonic.size() - 1] == 'l');
}
u32 PowerPCAssembler::BOAmt(const std::string& mnemonic)
{
  if (mnemonic.find("bdnzf") != std::string::npos)
    return 0;
  if (mnemonic.find("bdzf") != std::string::npos)
    return 2;
  if (mnemonic.find("bge") != std::string::npos)
    return 4;
  if (mnemonic.find("ble") != std::string::npos)
    return 4;
  if (mnemonic.find("bne") != std::string::npos)
    return 4;
  if (mnemonic.find("bns") != std::string::npos)
    return 4;
  if (mnemonic.find("bdnzt") != std::string::npos)
    return 8;
  if (mnemonic.find("bdzt") != std::string::npos)
    return 10;
  if (mnemonic.find("blt") != std::string::npos)
    return 12;
  if (mnemonic.find("bgt") != std::string::npos)
    return 12;
  if (mnemonic.find("beq") != std::string::npos)
    return 12;
  if (mnemonic.find("bso") != std::string::npos)
    return 12;
  if (mnemonic.find("bdnz") != std::string::npos)
    return 16;
  if (mnemonic.find("bdz") != std::string::npos)
    return 18;
  return 0xFFFFFFFF;
}
u32 PowerPCAssembler::BIAmt(const std::string& mnemonic)
{
  if (mnemonic.find("bge") != std::string::npos)
    return 0;
  if (mnemonic.find("ble") != std::string::npos)
    return 1;
  if (mnemonic.find("bne") != std::string::npos)
    return 2;
  if (mnemonic.find("bns") != std::string::npos)
    return 3;
  if (mnemonic.find("bdnzt") != std::string::npos)
    return 0;
  if (mnemonic.find("bdzt") != std::string::npos)
    return 0;
  if (mnemonic.find("blt") != std::string::npos)
    return 0;
  if (mnemonic.find("bgt") != std::string::npos)
    return 1;
  if (mnemonic.find("beq") != std::string::npos)
    return 2;
  if (mnemonic.find("bso") != std::string::npos)
    return 3;
  if (mnemonic.find("bdnz") != std::string::npos)
    return 0;
  if (mnemonic.find("bdz") != std::string::npos)
    return 0;
  return 0xFFFFFFFF;
}
u32 PowerPCAssembler::TOAmt(const std::string& mnemonic)
{
  if (mnemonic.find("lgt") != std::string::npos)
    return 1;
  if (mnemonic.find("llt") != std::string::npos)
    return 2;
  if (mnemonic.find("eq") != std::string::npos)
    return 4;
  if (mnemonic.find("lge") != std::string::npos)
    return 5;
  if (mnemonic.find("lle") != std::string::npos)
    return 6;
  if (mnemonic.find("gt") != std::string::npos)
    return 8;
  if (mnemonic.find("ge") != std::string::npos)
    return 12;
  if (mnemonic.find("lt") != std::string::npos)
    return 16;
  if (mnemonic.find("dl") != std::string::npos)
    return 20;
  if (mnemonic.find("ne") != std::string::npos)
    return 24;
  if (mnemonic.find("le") != std::string::npos)
    return 20;
  return 0xFFFFFFFF;
}

// Handles all instructions starting with the letter "a"
u32 PowerPCAssembler::InstructionA(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("addic"):
    return MI({{12 + rc, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("addi"):
    return MI({{14, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("addis"):
    return MI({{15, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("andi"):
    return MI({{28, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16}});
  case HsCp("andis"):
    return MI({{29, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16}});
  case HsCp("add"):
  case HsCp("addo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {266, 9}, {rc, 1}});
  case HsCp("addc"):
  case HsCp("addco"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {10, 9}, {rc, 1}});
  case HsCp("adde"):
  case HsCp("addeo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {138, 9}, {rc, 1}});
  case HsCp("addme"):
  case HsCp("addmeo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {EndO(t[0]), 1}, {234, 9}, {rc, 1}});
  case HsCp("addze"):
  case HsCp("addzeo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {EndO(t[0]), 1}, {202, 9}, {rc, 1}});
  case HsCp("and"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {28, 10}, {rc, 1}});
  case HsCp("andc"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {60, 10}, {rc, 1}});
  default:
    return 0;
  }
}

// Handles all instructions starting with the letter "b"
u32 PowerPCAssembler::InstructionB(const std::vector<std::string>& t, std::vector<u32>& n,
                                   u32 end_in_plus)
{
  switch (HsRn(t[0]))
  {
  case HsCp("bc"):
  case HsCp("bca"):
  case HsCp("bcl"):
  case HsCp("bcla"):
    return MI({{16, 6}, {n[1], 5}, {n[2], 5}, {n[3] >> 2, 14}, {EndA(t[0]), 1}, {EndL(t[0]), 1}});
  case HsCp("bdnzf"):
  case HsCp("bdnzfl"):
  case HsCp("bdnzfa"):
  case HsCp("bdnzfla"):
    end_in_plus = end_in_plus ^ (n[2] > 0x8000);
    return MI(
        {{16, 6}, {end_in_plus, 5}, {n[1], 5}, {n[2] >> 2, 14}, {EndA(t[0]), 1}, {EndL(t[0]), 1}});
  case HsCp("bdzf"):
  case HsCp("bdzfl"):
  case HsCp("bdzfa"):
  case HsCp("bdzfla"):
    end_in_plus = end_in_plus ^ (n[2] > 0x8000);
    return MI({{16, 6},
               {2 + end_in_plus, 5},
               {n[1], 5},
               {n[2] >> 2, 14},
               {EndA(t[0]), 1},
               {EndL(t[0]), 1}});

  case HsCp("bge"):
  case HsCp("bgel"):
  case HsCp("bgea"):
  case HsCp("bgela"):
  case HsCp("ble"):
  case HsCp("blel"):
  case HsCp("blea"):
  case HsCp("blela"):
  case HsCp("bne"):
  case HsCp("bnel"):
  case HsCp("bnea"):
  case HsCp("bnela"):
  case HsCp("bns"):
  case HsCp("bnsl"):
  case HsCp("bnsa"):
  case HsCp("bnsla"):
  case HsCp("blt"):
  case HsCp("bltl"):
  case HsCp("blta"):
  case HsCp("bltla"):
  case HsCp("bgt"):
  case HsCp("bgtl"):
  case HsCp("bgta"):
  case HsCp("bgtla"):
  case HsCp("beq"):
  case HsCp("beql"):
  case HsCp("beqa"):
  case HsCp("beqla"):
  case HsCp("bso"):
  case HsCp("bsol"):
  case HsCp("bsoa"):
  case HsCp("bsola"):
    if (t[2] == "")
    {
      // no crf specified, act like crf is 0
      n[2] = n[1];
      n[1] = 0;
    }
    end_in_plus = end_in_plus ^ (n[2] > 0x8000);
    return MI({{16, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(n[1] << 2) + BIAmt(t[0]), 5},
               {n[2] >> 2, 14},
               {EndA(t[0]), 1},
               {EndL(t[0]), 1}});
  case HsCp("bdnzt"):
  case HsCp("bdnztl"):
  case HsCp("bdnzta"):
  case HsCp("bdnztla"):
  case HsCp("bdzt"):
  case HsCp("bdztl"):
  case HsCp("bdzta"):
  case HsCp("bdztla"):
  case HsCp("bdnz"):
  case HsCp("bdnzl"):
  case HsCp("bdnza"):
  case HsCp("bdnzla"):
  case HsCp("bdz"):
  case HsCp("bdzl"):
  case HsCp("bdza"):
  case HsCp("bdzla"):
    if (t[2] == "")
    {
      // no crf specified, act like crf is 0
      n[2] = n[1];
      n[1] = BIAmt(t[0]);
    }
    end_in_plus = end_in_plus ^ (n[2] > 0x8000);
    return MI({{16, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(n[1]), 5},
               {n[2] >> 2, 14},
               {EndA(t[0]), 1},
               {EndL(t[0]), 1}});

  case HsCp("b"):
  case HsCp("ba"):
  case HsCp("bl"):
  case HsCp("bla"):
    return MI({{18, 6}, {n[1] >> 2, 24}, {EndA(t[0]), 1}, {EndL(t[0]), 1}});

  case HsCp("bclr"):
  case HsCp("bclrl"):
  case HsCp("bdnzflr"):
  case HsCp("bdnzflrl"):
    return MI({{19, 6}, {end_in_plus, 5}, {n[1], 5}, {0, 5}, {16, 10}, {EndL(t[0]), 1}});

  case HsCp("bcctr"):
  case HsCp("bdnzfctr"):
  case HsCp("bdnzfctrl"):
    return MI({{19, 6}, {end_in_plus, 5}, {n[1], 5}, {0, 5}, {528, 10}, {EndL(t[0]), 1}});
  case HsCp("bcctrl"):
    return MI({{19, 6}, {n[1] + end_in_plus, 5}, {n[2], 5}, {0, 5}, {528, 10}, {EndL(t[0]), 1}});

  case HsCp("bnsectr"):
  case HsCp("bnsectrl"):
  case HsCp("bsocctr"):
  case HsCp("bsocctrl"):
    return MI({{19, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(0) + BIAmt(t[0]), 5},
               {0, 5},
               {528, 10},
               {EndL(t[0]), 1}});
  case HsCp("bdnztctr"):
  case HsCp("bdnztctrl"):
  case HsCp("bdzfctr"):
  case HsCp("bdzfctrl"):
  case HsCp("bdztctr"):
  case HsCp("bdztctrl"):
    return MI({{19, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(n[1]) + BIAmt(t[0]), 5},
               {0, 5},
               {528, 10},
               {EndL(t[0]), 1}});
  case HsCp("bcfctr"):
    return MI({{19, 6},
               {n[1] + end_in_plus, 5},
               {(n[2]) + BIAmt(t[0]), 5},
               {0, 5},
               {528, 10},
               {EndL(t[0]), 1}});
  case HsCp("blectr"):
  case HsCp("blectrl"):
  case HsCp("bnectr"):
  case HsCp("bnectrl"):
  case HsCp("bnsctr"):
  case HsCp("bnsctrl"):
  case HsCp("bgectr"):
  case HsCp("bgectrl"):
  case HsCp("bsoctr"):
  case HsCp("bsoctrl"):
  case HsCp("bltctr"):
  case HsCp("bltctrl"):
  case HsCp("bgtctr"):
  case HsCp("bgtctrl"):
  case HsCp("beqctr"):
  case HsCp("beqctrl"):
  case HsCp("bdnzctr"):
  case HsCp("bdnzctrl"):
  case HsCp("bdzctr"):
  case HsCp("bdzctrl"):
  case HsCp("bctr"):
  case HsCp("bctrl"):
    return MI({{19, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(n[1] << 2) + BIAmt(t[0]), 5},
               {0, 5},
               {528, 10},
               {EndL(t[0]), 1}});

  case HsCp("bdzflr"):
  case HsCp("bdzflrl"):
  case HsCp("bdnztlr"):
  case HsCp("bdnztlrl"):
  case HsCp("bdztlr"):
  case HsCp("bdztlrl"):
    return MI({{19, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(n[1]) + BIAmt(t[0]), 5},
               {0, 5},
               {16, 10},
               {EndL(t[0]), 1}});
  case HsCp("bnelr"):
  case HsCp("bnelrl"):
  case HsCp("bgelr"):
  case HsCp("bgelrl"):
  case HsCp("blelr"):
  case HsCp("blelrl"):
  case HsCp("bnslr"):
  case HsCp("bnslrl"):
  case HsCp("bltlr"):
  case HsCp("bltlrl"):
  case HsCp("bgtlr"):
  case HsCp("bgtlrl"):
  case HsCp("beqlr"):
  case HsCp("beqlrl"):
  case HsCp("bsolr"):
  case HsCp("bsolrl"):
  case HsCp("bdnzlr"):
  case HsCp("bdnzlrl"):
  case HsCp("bdzlr"):
  case HsCp("bdzlrl"):
  case HsCp("blrl"):
    return MI({{19, 6},
               {BOAmt(t[0]) + end_in_plus, 5},
               {(n[1] << 2) + BIAmt(t[0]), 5},
               {0, 5},
               {16, 10},
               {EndL(t[0]), 1}});
  case HsCp("blr"):
    // more familiar looking format instead of the above (aka 0x4FFF0020) or 0x4FFE4820
    return 0x4E800020;
  default:
    return 0;
  }
}

// Handles all instructions starting with the letter "c"
u32 PowerPCAssembler::InstructionC(const std::vector<std::string>& t, std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("cmpli"):
    if (t[4] == "")
    {
      // no crf specified, act like crf is 0
      n[4] = n[3];
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{10, 6}, {n[1], 3}, {0, 1}, {n[2], 1}, {n[3], 5}, {n[4], 16}});
  case HsCp("cmpldi"):
  case HsCp("cmplwi"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{10, 6}, {n[1], 3}, {0, 1}, {EndDI(t[0]), 1}, {n[2], 5}, {n[3], 16}});
  case HsCp("cmpi"):
    if (t[4] == "")
    {
      // no crf specified, act like crf is 0
      n[4] = n[3];
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{11, 6}, {n[1], 3}, {0, 1}, {n[2], 1}, {n[3], 5}, {n[4], 16}});
  case HsCp("cmpdi"):
  case HsCp("cmpwi"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{11, 6}, {n[1], 3}, {0, 1}, {EndDI(t[0]), 1}, {n[2], 5}, {n[3], 16}});
  case HsCp("crnot"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[2], 5}, {33, 10}, {0, 1}});
  case HsCp("crnor"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {33, 10}, {0, 1}});
  case HsCp("crandc"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {129, 10}, {0, 1}});
  case HsCp("crclr"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[2], 5}, {193, 10}, {0, 1}});
  case HsCp("crxor"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {193, 10}, {0, 1}});
  case HsCp("crnand"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {225, 10}, {0, 1}});
  case HsCp("crand"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {257, 10}, {0, 1}});
  case HsCp("crse"):
    return MI({{19, 6}, {n[1], 5}, {n[1], 5}, {n[1], 5}, {289, 10}, {0, 1}});
  case HsCp("crset"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[2], 5}, {289, 10}, {0, 1}});
  case HsCp("creqv"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {289, 10}, {0, 1}});
  case HsCp("crorc"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {417, 10}, {0, 1}});
  case HsCp("crmove"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[2], 5}, {449, 10}, {0, 1}});
  case HsCp("cror"):
    return MI({{19, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {449, 10}, {0, 1}});
  case HsCp("cmp"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{31, 6}, {n[1], 3}, {0, 1}, {n[2], 1}, {n[3], 5}, {n[4], 5}, {0, 10}, {0, 1}});
  case HsCp("cmpd"):
  case HsCp("cmpw"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{31, 6}, {n[1], 3}, {0, 1}, {EndD(t[0]), 1}, {n[2], 5}, {n[3], 5}, {0, 10}, {0, 1}});
  case HsCp("cmpl"):
    if (t[4] == "")
    {
      // no crf specified, act like crf is 0
      n[4] = n[3];
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{31, 6}, {n[1], 3}, {0, 1}, {n[2], 1}, {n[3], 5}, {n[4], 5}, {32, 10}, {0, 1}});
  case HsCp("cmpld"):
  case HsCp("cmplw"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI(
        {{31, 6}, {n[1], 3}, {0, 1}, {EndD(t[0]), 1}, {n[2], 5}, {n[3], 5}, {32, 10}, {0, 1}});
  case HsCp("cntlzw"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {0, 5}, {26, 10}, {rc, 1}});
  case HsCp("cntlzd"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {0, 5}, {58, 10}, {rc, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "d"
u32 PowerPCAssembler::InstructionD(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("dcbz_l"):
    return MI({{4, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {1014, 10}, {rc, 1}});
  case HsCp("divd"):
  case HsCp("divdo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {489, 9}, {rc, 1}});
  case HsCp("divw"):
  case HsCp("divwo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {491, 9}, {rc, 1}});
  case HsCp("divdu"):
  case HsCp("divduo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {457, 9}, {rc, 1}});
  case HsCp("divwu"):
  case HsCp("divwuo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {459, 9}, {rc, 1}});
  case HsCp("dcbtst"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {246, 10}, {0, 1}});
  case HsCp("dcbt"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {278, 10}, {0, 1}});
  case HsCp("dcbi"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {470, 10}, {0, 1}});
  case HsCp("dcbz"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {1014, 10}, {0, 1}});
  case HsCp("dcbst"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {54, 10}, {0, 1}});
  case HsCp("dcbf"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {86, 10}, {0, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "e"
u32 PowerPCAssembler::InstructionE(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("extsb"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {0, 5}, {954, 10}, {rc, 1}});
  case HsCp("extsh"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {0, 5}, {922, 10}, {rc, 1}});
  case HsCp("extsw"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {0, 5}, {986, 10}, {rc, 1}});
  case HsCp("eqv"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {284, 10}, {rc, 1}});
  case HsCp("eciwx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {310, 10}, {0, 1}});
  case HsCp("ecowx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {438, 10}, {0, 1}});
  case HsCp("eieio"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {0, 5}, {854, 10}, {0, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "f"
u32 PowerPCAssembler::InstructionF(const std::vector<std::string>& t, std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("fadds"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {21, 5}, {rc, 1}});
  case HsCp("fsubs"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {20, 5}, {rc, 1}});
  case HsCp("fdivs"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {18, 5}, {rc, 1}});
  case HsCp("fmuls"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {n[3], 5}, {25, 5}, {rc, 1}});
  case HsCp("fmsubs"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {28, 5}, {rc, 1}});
  case HsCp("fnmadds"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {31, 5}, {rc, 1}});
  case HsCp("fnmsubs"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {30, 5}, {rc, 1}});
  case HsCp("fsqrts"):
    return MI({{59, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {0, 5}, {22, 5}, {rc, 1}});
  case HsCp("fres"):
    return MI({{59, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {0, 5}, {24, 5}, {rc, 1}});
  case HsCp("fmadds"):
    return MI({{59, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {29, 5}, {rc, 1}});
  case HsCp("fcmpu"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{63, 6}, {n[1], 3}, {0, 2}, {n[2], 5}, {n[3], 5}, {0, 10}, {0, 1}});
  case HsCp("fcmpo"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{63, 6}, {n[1], 3}, {0, 2}, {n[2], 5}, {n[3], 5}, {32, 10}, {0, 1}});
  case HsCp("fmr"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {72, 10}, {rc, 1}});
  case HsCp("fneg"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {40, 10}, {rc, 1}});
  case HsCp("fabs"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {264, 10}, {rc, 1}});
  case HsCp("fnabs"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {136, 10}, {rc, 1}});
  case HsCp("fadd"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {21, 5}, {rc, 1}});
  case HsCp("fsub"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {20, 5}, {rc, 1}});
  case HsCp("fdiv"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {18, 5}, {rc, 1}});
  case HsCp("fmul"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {n[3], 5}, {25, 5}, {rc, 1}});
  case HsCp("fmadd"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {29, 5}, {rc, 1}});
  case HsCp("fmsub"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {28, 5}, {rc, 1}});
  case HsCp("fnmadd"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {31, 5}, {rc, 1}});
  case HsCp("fnmsub"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {30, 5}, {rc, 1}});
  case HsCp("frsp"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {12, 10}, {rc, 1}});
  case HsCp("fctiw"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {14, 10}, {rc, 1}});
  case HsCp("fctiwz"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {15, 10}, {rc, 1}});
  case HsCp("fsqrt"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {0, 5}, {22, 5}, {rc, 1}});
  case HsCp("frsqrte"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {0, 5}, {26, 5}, {rc, 1}});
  case HsCp("fsel"):
    return MI({{63, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {23, 5}, {rc, 1}});
  case HsCp("fctid"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {814, 10}, {rc, 1}});
  case HsCp("fctidz"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {815, 10}, {rc, 1}});
  case HsCp("fcfid"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {846, 10}, {rc, 1}});

  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "i"
u32 PowerPCAssembler::InstructionI(const std::vector<std::string>& t, const std::vector<u32>& n)
{
  switch (HsRn(t[0]))
  {
  case HsCp("ill"):
    // ensures an alias isn't used due to overlapping hashing
    return StringToNumber(t[1], false);
  case HsCp("isync"):
    return MI({{19, 6}, {0, 5}, {0, 5}, {0, 5}, {150, 10}, {0, 1}});
  case HsCp("icbi"):
    return MI({{31, 6}, {0, 5}, {n[1], 5}, {n[2], 5}, {982, 10}, {0, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "l"
u32 PowerPCAssembler::InstructionL(const std::vector<std::string>& t, const std::vector<u32>& n)
{
  switch (HsRn(t[0]))
  {
  case HsCp("li"):
    return MI({{14, 6}, {n[1], 5}, {0, 5}, {n[2], 16}});
  case HsCp("lis"):
    return MI({{15, 6}, {n[1], 5}, {0, 5}, {n[2], 16}});
  case HsCp("lwaux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {373, 10}, {0, 1}});
  case HsCp("lwarx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {20, 10}, {0, 1}});
  case HsCp("lwbrx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {534, 10}, {0, 1}});
  case HsCp("lswi"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {597, 10}, {0, 1}});
  case HsCp("lhzx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {279, 10}, {0, 1}});
  case HsCp("lhzux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {311, 10}, {0, 1}});
  case HsCp("lhbrx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {790, 10}, {0, 1}});
  case HsCp("lhax"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {343, 10}, {0, 1}});
  case HsCp("lhaux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {375, 10}, {0, 1}});
  case HsCp("lfsx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {535, 10}, {0, 1}});
  case HsCp("lfsux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {567, 10}, {0, 1}});
  case HsCp("lfdx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {599, 10}, {0, 1}});
  case HsCp("lfdux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {631, 10}, {0, 1}});
  case HsCp("lbzx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {87, 10}, {0, 1}});
  case HsCp("lbzux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {119, 10}, {0, 1}});
  case HsCp("ldx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {21, 10}, {0, 1}});
  case HsCp("ldux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {53, 10}, {0, 1}});
  case HsCp("lwzx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {23, 10}, {0, 1}});
  case HsCp("lwzux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {55, 10}, {0, 1}});
  case HsCp("ldarx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {84, 10}, {0, 1}});
  case HsCp("lwax"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {341, 10}, {0, 1}});
  case HsCp("lswx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {533, 10}, {0, 1}});
  case HsCp("lwz"):
    return MI({{32, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lwzu"):
    return MI({{33, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lbz"):
    return MI({{34, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lbzu"):
    return MI({{35, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lhz"):
    return MI({{40, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lhzu"):
    return MI({{41, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lha"):
    return MI({{42, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lhau"):
    return MI({{43, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lfs"):
    return MI({{48, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lfsu"):
    return MI({{49, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lfd"):
    return MI({{50, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lfdu"):
    return MI({{51, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("lmw"):
    return MI({{46, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("ld"):
    return MI({{58, 6}, {n[1], 5}, {n[3], 5}, {n[2] >> 2, 14}, {0, 2}});
  case HsCp("ldu"):
    return MI({{58, 6}, {n[1], 5}, {n[3], 5}, {n[2] >> 2, 14}, {1, 2}});
  case HsCp("lwa"):
    return MI({{58, 6}, {n[1], 5}, {n[3], 5}, {n[2] >> 2, 14}, {2, 2}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "m"
u32 PowerPCAssembler::InstructionM(const std::vector<std::string>& t, std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("mulli"):
    return MI({{7, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("mcrf"):
    return MI({{19, 6}, {n[1], 3}, {0, 2}, {n[2], 3}, {0, 2}, {0, 5}, {0, 10}, {0, 1}});
  case HsCp("mulhdu"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 1}, {9, 9}, {rc, 1}});
  case HsCp("mulhwu"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 1}, {11, 9}, {rc, 1}});
  case HsCp("mulhd"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 1}, {73, 9}, {rc, 1}});
  case HsCp("mulhw"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 1}, {75, 9}, {rc, 1}});
  case HsCp("mulld"):
  case HsCp("mulldo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {233, 9}, {rc, 1}});
  case HsCp("mullw"):
  case HsCp("mullwo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {235, 9}, {rc, 1}});
  case HsCp("mfcr"):
    return MI({{31, 6}, {n[1], 5}, {0, 1}, {0, 9}, {19, 10}, {0, 1}});
  case HsCp("mr"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[2], 5}, {444, 10}, {rc, 1}});
  case HsCp("mfmsr"):
    return MI({{31, 6}, {n[1], 5}, {0, 5}, {0, 5}, {83, 10}, {0, 1}});
  case HsCp("mtcrf"):
    return MI({{31, 6}, {n[2], 5}, {0, 1}, {n[1], 8}, {0, 1}, {144, 10}, {0, 1}});
  case HsCp("mtcr"):
    return MI({{31, 6}, {n[1], 5}, {0, 1}, {255, 8}, {0, 1}, {144, 10}, {0, 1}});
  case HsCp("mtmsr"):
    return MI({{31, 6}, {n[1], 5}, {0, 5}, {0, 5}, {146, 10}, {0, 1}});
  case HsCp("mtsr"):
    return MI({{31, 6}, {n[2], 5}, {0, 1}, {n[1], 4}, {0, 5}, {210, 10}, {0, 1}});
  case HsCp("mtsrin"):
    return MI({{31, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {242, 10}, {0, 1}});
  case HsCp("mfxer"):
    return MI({{31, 6}, {n[1], 5}, {1, 5}, {0, 5}, {339, 10}, {0, 1}});
  case HsCp("mflr"):
    return MI({{31, 6}, {n[1], 5}, {8, 5}, {0, 5}, {339, 10}, {0, 1}});
  case HsCp("mfctr"):
    return MI({{31, 6}, {n[1], 5}, {9, 5}, {0, 5}, {339, 10}, {0, 1}});
  case HsCp("mfspr"):
    return MI({{31, 6}, {n[1], 5}, {n[2] & 0x1F, 5}, {n[2] >> 5, 5}, {339, 10}, {0, 1}});
  case HsCp("mtxer"):
    return MI({{31, 6}, {n[1], 5}, {1, 5}, {0, 5}, {467, 10}, {0, 1}});
  case HsCp("mtlr"):
    return MI({{31, 6}, {n[1], 5}, {8, 5}, {0, 5}, {467, 10}, {0, 1}});
  case HsCp("mtctr"):
    return MI({{31, 6}, {n[1], 5}, {9, 5}, {0, 5}, {467, 10}, {0, 1}});
  case HsCp("mtspr"):
    return MI({{31, 6}, {n[2], 5}, {n[1] & 0x1F, 5}, {n[1] >> 5, 5}, {467, 10}, {0, 1}});
  case HsCp("mcrxr"):
    return MI({{31, 6}, {n[1], 3}, {0, 2}, {0, 5}, {0, 5}, {512, 10}, {0, 1}});
  case HsCp("mfsr"):
    return MI({{31, 6}, {n[1], 5}, {0, 1}, {n[2], 4}, {0, 5}, {595, 10}, {0, 1}});
  case HsCp("mfsrin"):
    return MI({{31, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {659, 10}, {0, 1}});
  case HsCp("mftb"):
    if (t[2] == "")
      n[2] = 268;
    return MI({{31, 6}, {n[1], 5}, {n[2] & 0x1F, 5}, {n[2] >> 5, 5}, {371, 10}, {0, 1}});
  case HsCp("mftbu"):
    return MI({{31, 6}, {n[1], 5}, {13, 5}, {8, 5}, {371, 10}, {0, 1}});
  case HsCp("mftbl"):  // undocumetned instruction. mftb???
    return MI({{31, 6}, {n[1], 5}, {803558, 21}});
  case HsCp("mtfsb1"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {0, 5}, {38, 10}, {rc, 1}});
  case HsCp("mtfsb0"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {0, 5}, {70, 10}, {rc, 1}});
  case HsCp("mcrfs"):
    return MI({{63, 6}, {n[1], 3}, {0, 2}, {n[2], 3}, {0, 2}, {0, 5}, {64, 10}, {0, 1}});
  case HsCp("mffs"):
    return MI({{63, 6}, {n[1], 5}, {0, 5}, {0, 5}, {583, 10}, {rc, 1}});
  case HsCp("mtfsfi"):
    return MI({{63, 6}, {n[1], 3}, {0, 2}, {0, 5}, {n[2], 4}, {0, 1}, {134, 10}, {rc, 1}});
  case HsCp("mtfsf"):
    return MI({{63, 6}, {0, 1}, {n[1], 8}, {0, 1}, {n[2], 5}, {711, 10}, {rc, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "n"
u32 PowerPCAssembler::InstructionN(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("nop"):
    return MI({{24, 6}, {0, 5}, {0, 5}, {0, 16}});
  case HsCp("nand"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {476, 10}, {rc, 1}});
  case HsCp("nor"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {124, 10}, {rc, 1}});
  case HsCp("not"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[2], 5}, {124, 10}, {rc, 1}});
  case HsCp("neg"):
  case HsCp("nego"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {EndO(t[0]), 1}, {104, 9}, {rc, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "o"
u32 PowerPCAssembler::InstructionO(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("ori"):
    return MI({{24, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16}});
  case HsCp("oris"):
    return MI({{25, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16}});
  case HsCp("or"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {444, 10}, {rc, 1}});
  case HsCp("orc"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {412, 10}, {rc, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "p"
u32 PowerPCAssembler::InstructionP(const std::vector<std::string>& t, std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("ps_cmpu0"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{4, 6}, {n[1], 3}, {0, 2}, {n[2], 5}, {n[3], 5}, {0, 10}, {0, 1}});
  case HsCp("ps_0"):
    return MI({{4, 6}, {0, 3}, {0, 2}, {0, 5}, {0, 5}, {128, 10}, {0, 1}});
  case HsCp("ps_1"):
  case HsCp("ps_2"):
  case HsCp("ps_3"):
  case HsCp("ps_4"):
  case HsCp("ps_5"):
  case HsCp("ps_8"):
  case HsCp("ps_9"):
  case HsCp("ps_16"):
  case HsCp("ps_17"):
  case HsCp("ps_19"):
  case HsCp("ps_22"):
  case HsCp("ps_27"):
    return MI({{4, 6}, {0, 3}, {0, 2}, {0, 5}, {0, 5}, {n[0], 10}, {0, 1}});
  case HsCp("ps_cmpo0"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{4, 6}, {n[1], 3}, {0, 2}, {n[2], 5}, {n[3], 5}, {32, 10}, {0, 1}});
  case HsCp("psq_lx"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {6, 6}, {0, 1}});
  case HsCp("psq_stx"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {7, 6}, {0, 1}});
  case HsCp("ps_sum0"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {n[4], 5}, {10, 5}, {rc, 1}});
  case HsCp("ps_sum1"):
    return MI({{4, 6}, {n[1], 5}, {n[3], 5}, {n[4], 5}, {n[2], 5}, {11, 5}, {rc, 1}});
  case HsCp("ps_muls0"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {n[3], 5}, {12, 5}, {rc, 1}});
  case HsCp("ps_muls1"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {n[3], 5}, {13, 5}, {rc, 1}});
  case HsCp("ps_madds0"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {14, 5}, {rc, 1}});
  case HsCp("ps_madds1"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {15, 5}, {rc, 1}});
  case HsCp("ps_div"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {18, 5}, {rc, 1}});
  case HsCp("ps_sub"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {20, 5}, {rc, 1}});
  case HsCp("ps_add"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {0, 5}, {21, 5}, {rc, 1}});
  case HsCp("ps_sel"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {23, 5}, {rc, 1}});
  case HsCp("ps_res"):
    return MI({{4, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {0, 5}, {24, 5}, {rc, 1}});
  case HsCp("ps_rsqrte"):
    return MI({{4, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {0, 5}, {26, 5}, {rc, 1}});
  case HsCp("ps_mul"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {n[3], 5}, {25, 5}, {rc, 1}});
  case HsCp("ps_msub"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {28, 5}, {rc, 1}});
  case HsCp("ps_madd"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {29, 5}, {rc, 1}});
  case HsCp("ps_merge00"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {528, 10}, {rc, 1}});
  case HsCp("ps_merge01"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {560, 10}, {rc, 1}});
  case HsCp("ps_merge10"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {592, 10}, {rc, 1}});
  case HsCp("ps_merge11"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {624, 10}, {rc, 1}});
  case HsCp("ps_mr"):
    return MI({{4, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {72, 10}, {rc, 1}});
  case HsCp("ps_nmsub"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {30, 5}, {rc, 1}});
  case HsCp("ps_nmadd"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[4], 5}, {n[3], 5}, {31, 5}, {rc, 1}});
  case HsCp("ps_neg"):
    return MI({{4, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {40, 10}, {rc, 1}});
  case HsCp("ps_nabs"):
    return MI({{4, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {136, 10}, {rc, 1}});
  case HsCp("psq_lux"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {38, 6}, {0, 1}});
  case HsCp("psq_stux"):
    return MI({{4, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {39, 6}, {0, 1}});
  case HsCp("ps_cmpu1"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{4, 6}, {n[1], 3}, {0, 2}, {n[2], 5}, {n[3], 5}, {64, 10}, {0, 1}});
  case HsCp("ps_cmpo1"):
    if (t[3] == "")
    {
      // no crf specified, act like crf is 0
      n[3] = n[2];
      n[2] = n[1];
      n[1] = 0;
    }
    return MI({{4, 6}, {n[1], 3}, {0, 2}, {n[2], 5}, {n[3], 5}, {96, 10}, {0, 1}});
  case HsCp("ps_abs"):
    return MI({{4, 6}, {n[1], 5}, {0, 5}, {n[2], 5}, {264, 10}, {rc, 1}});
  case HsCp("psq_l"):
    return MI({{56, 6}, {n[1], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {n[2], 12}});
  case HsCp("psq_lu"):
    return MI({{57, 6}, {n[1], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {n[2], 12}});
  case HsCp("psq_st"):
    return MI({{60, 6}, {n[1], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {n[2], 12}});
  case HsCp("psq_stu"):
    return MI({{61, 6}, {n[1], 5}, {n[3], 5}, {n[4], 1}, {n[5], 3}, {n[2], 12}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "r"
u32 PowerPCAssembler::InstructionR(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("rfi"):
    return MI({{19, 6}, {0, 5}, {0, 5}, {0, 5}, {50, 10}, {0, 1}});
  case HsCp("rotlwr"):
    return MI({{23, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {0, 5}, {31, 5}, {rc, 1}});
  case HsCp("rlwnm"):
    return MI({{23, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 5}, {n[5], 5}, {rc, 1}});
  case HsCp("rlwimi"):
    return MI({{20, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 5}, {n[5], 5}, {rc, 1}});
  case HsCp("rlwinm"):
    return MI({{21, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 5}, {n[5], 5}, {rc, 1}});
  case HsCp("rldicl"):
    return MI(
        {{30, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 6}, {0, 3}, {n[3] >> 5, 1}, {rc, 1}});
  case HsCp("rldicr"):
    return MI(
        {{30, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 6}, {1, 3}, {n[3] >> 5, 1}, {rc, 1}});
  case HsCp("rldic"):
    return MI(
        {{30, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 6}, {2, 3}, {n[3] >> 5, 1}, {rc, 1}});
  case HsCp("rldimi"):
    return MI(
        {{30, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 6}, {3, 3}, {n[3] >> 5, 1}, {rc, 1}});
  case HsCp("rldcr"):
    return MI({{30, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 6}, {9, 4}, {rc, 1}});
  case HsCp("rldcl"):
    return MI({{30, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {n[4], 6}, {8, 4}, {rc, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "s"
u32 PowerPCAssembler::InstructionS(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("subfic"):
    return MI({{8, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("subic"):
    return MI({{12 + rc, 6}, {n[1], 5}, {n[2], 5}, {-1 * n[3], 16}});
  case HsCp("subi"):
    return MI({{14, 6}, {n[1], 5}, {n[2], 5}, {-1 * n[3], 16}});
  case HsCp("subis"):
    return MI({{15, 6}, {n[1], 5}, {n[2], 5}, {-1 * n[3], 16}});
  case HsCp("sc"):
    return MI({{17, 6}, {0, 5}, {0, 5}, {0, 14}, {1, 1}, {0, 1}});
  case HsCp("sub"):
  case HsCp("subo"):
    return MI({{31, 6}, {n[1], 5}, {n[3], 5}, {n[2], 5}, {EndO(t[0]), 1}, {40, 9}, {rc, 1}});
  case HsCp("subf"):
  case HsCp("subfo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {40, 9}, {rc, 1}});
  case HsCp("subfc"):
  case HsCp("subfco"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {8, 9}, {rc, 1}});
  case HsCp("subc"):
  case HsCp("subco"):
    return MI({{31, 6}, {n[1], 5}, {n[3], 5}, {n[2], 5}, {EndO(t[0]), 1}, {8, 9}, {rc, 1}});
  case HsCp("subfe"):
  case HsCp("subfeo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {EndO(t[0]), 1}, {136, 9}, {rc, 1}});
  case HsCp("subfme"):
  case HsCp("subfmeo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {EndO(t[0]), 1}, {232, 9}, {rc, 1}});
  case HsCp("subfze"):
  case HsCp("subfzeo"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {0, 5}, {EndO(t[0]), 1}, {200, 9}, {rc, 1}});
  case HsCp("slbia"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {0, 5}, {498, 10}, {0, 1}});
  case HsCp("sync"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {0, 5}, {598, 10}, {0, 1}});
  case HsCp("stdx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {149, 10}, {0, 1}});
  case HsCp("stdux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {181, 10}, {0, 1}});
  case HsCp("stwux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {183, 10}, {0, 1}});
  case HsCp("stwx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {151, 10}, {0, 1}});
  case HsCp("sthux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {439, 10}, {0, 1}});
  case HsCp("sthx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {407, 10}, {0, 1}});
  case HsCp("stbux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {247, 10}, {0, 1}});
  case HsCp("stbx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {215, 10}, {0, 1}});
  case HsCp("stwcx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {150, 10}, {1, 1}});
  case HsCp("stwbrx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {662, 10}, {0, 1}});
  case HsCp("stdcx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {214, 10}, {rc, 1}});
  case HsCp("stswx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {661, 10}, {0, 1}});
  case HsCp("stfsx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {663, 10}, {0, 1}});
  case HsCp("stfsux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {695, 10}, {0, 1}});
  case HsCp("stswi"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {725, 10}, {0, 1}});
  case HsCp("stfdx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {727, 10}, {0, 1}});
  case HsCp("stfdux"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {759, 10}, {0, 1}});
  case HsCp("sthbrx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {918, 10}, {0, 1}});
  case HsCp("stfiwx"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {983, 10}, {0, 1}});
  case HsCp("slw"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {24, 10}, {rc, 1}});
  case HsCp("sld"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {27, 10}, {rc, 1}});
  case HsCp("srd"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {539, 10}, {rc, 1}});
  case HsCp("srw"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {536, 10}, {rc, 1}});
  case HsCp("sradi"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {413, 9}, {n[3] >> 5, 1}, {rc, 1}});
  case HsCp("srawi"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {824, 10}, {rc, 1}});
  case HsCp("slbie"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {n[1], 5}, {434, 10}, {0, 1}});
  case HsCp("sraw"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {792, 10}, {rc, 1}});
  case HsCp("srad"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {794, 10}, {rc, 1}});
  case HsCp("stb"):
    return MI({{38, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stbu"):
    return MI({{39, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("sth"):
    return MI({{44, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("sthu"):
    return MI({{45, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stw"):
    return MI({{36, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stwu"):
    return MI({{37, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stfs"):
    return MI({{52, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stfsu"):
    return MI({{53, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stfd"):
    return MI({{54, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stfdu"):
    return MI({{55, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("stmw"):
    return MI({{47, 6}, {n[1], 5}, {n[3], 5}, {n[2], 16}});
  case HsCp("std"):
    return MI({{62, 6}, {n[1], 5}, {n[3], 5}, {n[2] >> 2, 14}, {0, 2}});
  case HsCp("stdu"):
    return MI({{62, 6}, {n[1], 5}, {n[3], 5}, {n[2] >> 2, 14}, {1, 2}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "t"
u32 PowerPCAssembler::InstructionT(const std::vector<std::string>& t, const std::vector<u32>& n)
{
  switch (HsRn(t[0]))
  {
  case HsCp("td"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {68, 10}, {0, 1}});
  case HsCp("tdi"):
    return MI({{2, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("tdlgti"):
  case HsCp("tdllti"):
  case HsCp("tdeqi"):
  case HsCp("tdlgei"):
  case HsCp("tdllei"):
  case HsCp("tdgti"):
  case HsCp("tdgei"):
  case HsCp("tdlti"):
  case HsCp("tdlei"):
  case HsCp("tdnei"):
    return MI({{2, 6}, {TOAmt(t[0]), 5}, {n[1], 5}, {n[2], 16}});
  case HsCp("tdlgt"):
  case HsCp("tdllt"):
  case HsCp("tdeq"):
  case HsCp("tdlge"):
  case HsCp("tdlle"):
  case HsCp("tdgt"):
  case HsCp("tdge"):
  case HsCp("tdlt"):
  case HsCp("tdle"):
  case HsCp("tdne"):
    return MI({{31, 6}, {TOAmt(t[0]), 5}, {n[1], 5}, {n[2], 5}, {68, 10}, {0, 1}});
  case HsCp("tw"):
    return MI({{31, 6}, {n[1], 5}, {n[2], 5}, {n[3], 5}, {4, 10}, {0, 1}});
  case HsCp("trap"):
    return MI({{31, 6}, {31, 5}, {0, 5}, {0, 5}, {4, 10}, {0, 1}});
  case HsCp("twi"):
    return MI({{3, 6}, {n[1], 5}, {n[2], 5}, {n[3], 16}});
  case HsCp("twlgti"):
  case HsCp("twllti"):
  case HsCp("tweqi"):
  case HsCp("twlgei"):
  case HsCp("twllei"):
  case HsCp("twgti"):
  case HsCp("twgei"):
  case HsCp("twlti"):
  case HsCp("twlei"):
  case HsCp("twnei"):
    return MI({{3, 6}, {TOAmt(t[0]), 5}, {n[1], 5}, {n[2], 16}});
  case HsCp("twlgt"):
  case HsCp("twllt"):
  case HsCp("tweq"):
  case HsCp("twlge"):
  case HsCp("twlle"):
  case HsCp("twgt"):
  case HsCp("twge"):
  case HsCp("twlt"):
  case HsCp("twle"):
  case HsCp("twne"):
    return MI({{31, 6}, {TOAmt(t[0]), 5}, {n[1], 5}, {n[2], 5}, {4, 10}, {0, 1}});
  case HsCp("tlbie"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {n[1], 5}, {306, 10}, {0, 1}});
  case HsCp("tlbsync"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {0, 5}, {566, 10}, {0, 1}});
  case HsCp("tlbia"):
    return MI({{31, 6}, {0, 5}, {0, 5}, {0, 5}, {370, 10}, {0, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "x"
u32 PowerPCAssembler::InstructionX(const std::vector<std::string>& t, const std::vector<u32>& n,
                                   const u32 rc)
{
  switch (HsRn(t[0]))
  {
  case HsCp("xori"):
    return MI({{26, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16}});
  case HsCp("xoris"):
    return MI({{27, 6}, {n[2], 5}, {n[1], 5}, {n[3], 16}});
  case HsCp("xor"):
    return MI({{31, 6}, {n[2], 5}, {n[1], 5}, {n[3], 5}, {316, 10}, {rc, 1}});
  default:
    return 0;
  }
}
// Handles all instructions starting with the letter "."
u32 PowerPCAssembler::InstructionPeriod(const std::vector<std::string>& t)
{
  switch (HsRn(t[0]))
  {
  case HsCp(".word"):
  case HsCp(".float"):
  case HsCp(".long"):
    // ensures an alias isn't used due to overlapping hashing
    return StringToNumber(t[1], false);
  default:
    return 0;
  }
}

// Main assemble function. Takes a string of a PowerPC instruction and returns a big endian
// 32-bit integer
u32 PowerPCAssembler::PPCAssemble(const std::string& instruction,
                                  const u32 current_instruction_address)
{
  // if instruction is empty string, return 0
  if (instruction.empty() || instruction.size() == 0)
    return 0;

  std::string lowerCase = instruction;
  // set instruction to lowercase
  for (size_t i = 0; i < lowerCase.size(); ++i)
  {
    lowerCase[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowerCase[i])));
  }

  // tokenize instruction, get values as well
  std::vector<std::string> t = Tokenize(lowerCase);    // tokens variable. t for short
  std::vector<u32> n = std::vector<u32>(t.size(), 0);  // numbers variable. n for short
  for (size_t i = 0; i < t.size(); ++i)
  {
    n[i] = StringToNumber(t[i]);
    if (n[i] >= 0x80000000 && current_instruction_address != 0)
      n[i] = n[i] - current_instruction_address;
  }

  // if no mnemonic, return 0
  if (t[0] == "")
    return 0;

  // check if mnemonic ends with + or - or .
  // it then removes that character for easier processing when assembling
  u32 end_in_plus = t[0].find("+") == t[0].size() - 1;
  u32 end_in_minus = t[0].find("-") == t[0].size() - 1;
  u32 rc = t[0].find(".") != std::string::npos ? 1 : 0;
  if (rc)
    t[0] = t[0].substr(0, t[0].find("."));
  if (end_in_plus)
    t[0] = t[0].substr(0, t[0].find("+"));
  if (end_in_minus)
    t[0] = t[0].substr(0, t[0].find("-"));

  // assemble based on instruction's starting character
  switch (t[0][0])
  {
  case 'a':
    return InstructionA(t, n, rc);
  case 'b':
    return InstructionB(t, n, end_in_plus);
  case 'c':
    return InstructionC(t, n, rc);
  case 'd':
    return InstructionD(t, n, rc);
  case 'e':
    return InstructionE(t, n, rc);
  case 'f':
    return InstructionF(t, n, rc);
  case 'i':
    return InstructionI(t, n);
  case 'l':
    return InstructionL(t, n);
  case 'm':
    return InstructionM(t, n, rc);
  case 'n':
    return InstructionN(t, n, rc);
  case 'o':
    return InstructionO(t, n, rc);
  case 'p':
    return InstructionP(t, n, rc);
  case 'r':
    return InstructionR(t, n, rc);
  case 's':
    return InstructionS(t, n, rc);
  case 't':
    return InstructionT(t, n);
  case 'x':
    return InstructionX(t, n, rc);
  case '.':
    return InstructionPeriod(t);
  default:
    return 0;
  }
}
}  // namespace Common