#pragma once

#include <cstddef>
#include <string>

#include "CommonTypes.h"

namespace Common
{
const u32 MEM1_SIZE = 0x1800000;
const u32 MEM1_START = 0x80000000;
const u32 MEM1_END = 0x81800000;

const u32 MEM2_SIZE = 0x4000000;
const u32 MEM2_START = 0x90000000;
const u32 MEM2_END = 0x94000000;

enum class MemType
{
  type_byte = 0,
  type_halfword,
  type_word,
  type_float,
  type_double,
  type_string,
  type_byteArray,
  type_num
};

enum class MemBase
{
  base_decimal = 0,
  base_hexadecimal,
  base_octal,
  base_binary,
  base_none // Placeholder when the base doesn't matter (ie. string)
};

enum class StrWidth
{
  utf_8 = 0,
  utf_16,
  utf_32
};

enum class MemOperationReturnCode
{
  invalidInput,
  operationFailed,
  inputTooLong,
  invalidPointer,
  OK
};

size_t getSizeForType(const MemType type, const size_t length, const Common::StrWidth stringWidth);
size_t getStringCharWidth(const Common::StrWidth stringWidth);
bool shouldBeBSwappedForType(const MemType type);
int getNbrBytesAlignementForType(const MemType type);
char* formatStringToMemory(MemOperationReturnCode& returnCode, size_t& actualLength,
                           const std::string inputString, const MemBase base, const MemType type,
                           const size_t length, const StrWidth stringWidth = StrWidth::utf_8);
std::string formatMemoryToString(const char* memory, const MemType type, const size_t length,
                                 const MemBase base, const bool isUnsigned,
                                 const bool withBSwap = false,
                                 const StrWidth stringWidth = StrWidth::utf_8);
template <StrWidth Width>
std::string toUTF8String(const char* buf, int len);
template <>
std::string toUTF8String<StrWidth::utf_8>(const char* buf, int len);
template <>
std::string toUTF8String<StrWidth::utf_16>(const char* buf, int len);
template <>
std::string toUTF8String<StrWidth::utf_32>(const char* buf, int len);

template <StrWidth Width>
std::string convertFromUTF8(const char* buf, int len);
template <>
std::string convertFromUTF8<StrWidth::utf_8>(const char* buf, int len);
template <>
std::string convertFromUTF8<StrWidth::utf_16>(const char* buf, int len);
template <>
std::string convertFromUTF8<StrWidth::utf_32>(const char* buf, int len);
} // namespace Common
