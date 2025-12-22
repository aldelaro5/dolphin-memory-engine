#pragma once

#include <string>
#include <vector>

#include "../../Common/CommonTypes.h"

namespace Common
{
class PowerPCAssembler final
{
public:
  static u32 PPCAssemble(const std::string& instruction);

private:
  PowerPCAssembler() = delete;
  static u32 HsRn(const std::string& str);
  static std::vector<std::string> Tokenize(const std::string& user_str);
  static u32 StringAliasToNumber(const std::string& token);
  static u32 StringToNumber(const std::string& token, bool consider_alias = true);

  // MIStruct - Make Instruction Struct. Contains a value and size in bits for how much of the
  // instruction it takes up.
  struct MIStruct
  {
    u32 value;
    u8 sizeInBits;
  };
  static u32 MI(std::initializer_list<MIStruct> all_parts);

  static u32 EndX(const std::string& mnemonic, const char check_char);
  static u32 EndA(const std::string& mnemonic);
  static u32 EndD(const std::string& mnemonic);
  static u32 EndO(const std::string& mnemonic);
  static u32 EndDI(const std::string& mnemonic);
  static u32 EndL(const std::string& mnemonic);
  static u32 BOAmt(const std::string& mnemonic);
  static u32 BIAmt(const std::string& mnemonic);
  static u32 TOAmt(const std::string& mnemonic);

  static u32 InstructionA(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionB(const std::vector<std::string>& t, std::vector<u32>& n, u32 end_in_plus);
  static u32 InstructionC(const std::vector<std::string>& t, std::vector<u32>& n, const u32 rc);
  static u32 InstructionD(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionE(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionF(const std::vector<std::string>& t, std::vector<u32>& n, const u32 rc);
  static u32 InstructionI(const std::vector<std::string>& t, const std::vector<u32>& n);
  static u32 InstructionL(const std::vector<std::string>& t, const std::vector<u32>& n);
  static u32 InstructionM(const std::vector<std::string>& t, std::vector<u32>& n, const u32 rc);
  static u32 InstructionN(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionO(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionP(const std::vector<std::string>& t, std::vector<u32>& n, const u32 rc);
  static u32 InstructionR(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionS(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionT(const std::vector<std::string>& t, const std::vector<u32>& n);
  static u32 InstructionX(const std::vector<std::string>& t, const std::vector<u32>& n,
                          const u32 rc);
  static u32 InstructionPeriod(const std::vector<std::string>& t);
};
}  // namespace Common