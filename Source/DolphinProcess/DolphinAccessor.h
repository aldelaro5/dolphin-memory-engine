// Wrapper around IDolphinProcess
#pragma once

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"
#include "IDolphinProcess.h"

namespace DolphinComm
{
class DolphinAccessor
{
public:
  enum class DolphinStatus
  {
    hooked,
    notRunning,
    noEmu,
    unHooked
  };

  static void init();
  static void free();
  static void hook();
  static void unHook();
  static bool readFromRAM(const u32 offset, char* buffer, const size_t size, const bool withBSwap);
  static bool writeToRAM(const u32 offset, const char* buffer, const size_t size,
                         const bool withBSwap);
  static int getPID();
  static u64 getEmuRAMAddressStart();
  static DolphinStatus getStatus();
  static bool isARAMAccessible();
  static u64 getARAMAddressStart();
  static bool isMEM2Present();
  static size_t getRAMTotalSize();
  static Common::MemOperationReturnCode readEntireRAM(char* buffer);
  static std::string getFormattedValueFromMemory(const u32 ramIndex, Common::MemType memType,
                                                 size_t memSize, Common::MemBase memBase,
                                                 bool memIsUnsigned);
  static bool isValidConsoleAddress(const u32 address);

private:
  static IDolphinProcess* m_instance;
  static DolphinStatus m_status;
};
}  // namespace DolphinComm
