#ifdef _WIN32

#pragma once

#include <windows.h>

#include "../IDolphinProcess.h"

namespace DolphinComm
{
class WindowsDolphinProcess : public IDolphinProcess
{
public:
  WindowsDolphinProcess() = default;
  bool findPID() override;
  bool obtainEmuRAMInformations() override;
  bool readFromRAM(u32 offset, char* buffer, size_t size, bool withBSwap) override;
  bool writeToRAM(u32 offset, const char* buffer, size_t size, bool withBSwap) override;

private:
  HANDLE m_hDolphin;
};
}  // namespace DolphinComm
#endif
