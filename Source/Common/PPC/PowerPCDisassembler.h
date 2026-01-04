#pragma once

#include <string>
#include <vector>
#include "../../Common/CommonTypes.h"

namespace Common
{
class PowerPCDisassembler final
{
public:
  static std::string PPCDisassemble(u32 binary, u32 current_instruction_address = 0);

private:
  PowerPCDisassembler() = delete;

  // helpers
  static u32 ValueAtBits(const u32 binary, const u32 startBit, const u32 sizeInBits);
  static std::string MA(const std::string& mnemonic, const u32 rc, const u32 add_letter_o = 0);
  static std::string TS(const u32 value, const u8 mode = 10, const u8 size = 0);
  static std::string Alias(const u32 number, const std::string& str_prefix = "");
  static std::string TrapTOStr(const u32 to);

  // returns disassembled instructions
  static std::string ILL(const u32 binary);
  static std::string TXI(const u32 binary, const std::string& type);
  static std::string TX(const u32 binary, const std::string& type);
  static std::string opcode4Handler(const u32 binary);
  static std::string MathImmediate(const u32 binary, std::string mnemonic,
                                   const bool consider_neg = true, const bool hex = false);
  static std::string MemImmediate(const u32 binary, const std::string& mnemonic,
                                  const std::string& prefix = "r");
  static std::string CompareImmediate(const u32 binary, std::string mnemonic,
                                      const bool consider_negative = false);
  static std::string Opcode16Handler(const u32 binary, const u32 current_instruction_address);
  static std::string Opcode18Handler(const u32 binary, const u32 current_instruction_address);
  static std::string Opcode19HandlerExt16(const u32 binary, const std::string& toAdd);
  static std::string Opcode19Handler(const u32 binary);
  static std::string RLWInstruction(const u32 binary, const std::string& mnemonic);
  static std::string Opcode31Handler(const u32 binary);
  static std::string PsqInstruction(const u32 binary, const std::string& mnemonic);
  static std::string Opcode59Handler(const u32 binary);
  static std::string Opcode63Handler(const u32 binary);
};
}  // namespace Common