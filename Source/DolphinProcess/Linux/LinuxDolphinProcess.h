#ifdef __linux__

#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include "../IDolphinProcess.h"

namespace DolphinComm
{
class LinuxDolphinProcess : public IDolphinProcess
{
public:
  LinuxDolphinProcess() = default;
  bool findPID() override;
  bool obtainEmuRAMInformations() override;
  bool readFromRAM(u32 offset, char* buffer, size_t size, bool withBSwap) override;
  bool writeToRAM(u32 offset, const char* buffer, size_t size, bool withBSwap) override;
};
}  // namespace DolphinComm
#endif
