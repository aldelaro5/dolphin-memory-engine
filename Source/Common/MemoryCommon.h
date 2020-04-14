#pragma once

#include <cstddef>
#include <string>

#include "CommonTypes.h"

namespace Common
{
u32 GetMEM1SizeReal();
u32 GetMEM2SizeReal();
u32 GetMEM1Size();
u32 GetMEM2Size();
u32 GetMEM1End();
u32 GetMEM2End();
constexpr u32 MEM1_START = 0x80000000;
constexpr u32 MEM2_START = 0x90000000;
constexpr u32 ARAM_SIZE = 0x1000000;
// Dolphin maps 32 mb for the fakeVMem which is what ends up being the speedhack, but in reality
// the ARAM is actually 16 mb. We need the fake size to do process address calculation
constexpr u32 ARAM_FAKESIZE = 0x2000000;
constexpr u32 ARAM_START = 0x7E000000;
constexpr u32 ARAM_END = 0x7F000000;

void UpdateMemoryValues();

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

enum class MemOperationReturnCode
{
  invalidInput,
  operationFailed,
  inputTooLong,
  invalidPointer,
  OK
};

size_t getSizeForType(const MemType type, const size_t length);
bool shouldBeBSwappedForType(const MemType type);
int getNbrBytesAlignmentForType(const MemType type);
char* formatStringToMemory(MemOperationReturnCode& returnCode, size_t& actualLength,
                           const std::string inputString, const MemBase base, const MemType type,
                           const size_t length);
std::string formatMemoryToString(const char* memory, const MemType type, const size_t length,
                                 const MemBase base, const bool isUnsigned,
                                 const bool withBSwap = false);
} // namespace Common
