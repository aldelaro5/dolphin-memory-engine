#ifdef _WIN32

#pragma once

#include <windows.h>

#include "../IDolphinProcess.h"

namespace DolphinComm
{
class WindowsDolphinProcess : public IDolphinProcess
{
public:
  WindowsDolphinProcess()
  {
  }
  bool findPID() override;
  bool obtainEmuRAMInformations() override;
  bool readFromRAM(const u32 offset, char* buffer, const size_t size,
                   const bool withBSwap) override;
  bool writeToRAM(const u32 offset, const char* buffer, const size_t size,
                  const bool withBSwap) override;

private:
  HANDLE m_hDolphin;
};
} // namespace DolphinComm
#endif
