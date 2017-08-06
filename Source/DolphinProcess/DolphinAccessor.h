// Wrapper around IDolphinProcess
#pragma once

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

  static void hook();
  static void unHook();
  static bool readFromRAM(const u32 offset, char* buffer, const size_t size, const bool withBSwap);
  static bool writeToRAM(const u32 offset, const char* buffer, const size_t size,
                         const bool withBSwap);
  static int getPID();
  static u64 getEmuRAMAddressStart();
  static DolphinStatus getStatus();
  static void enableMem2(const bool doEnable);
  static bool isMem2Enabled();
  static void autoDetectMem2();
  static bool isValidConsoleAddress(const u32 address);

private:
  static IDolphinProcess* m_instance;
  static DolphinStatus m_status;
  static bool m_mem2Enabled;
};
}