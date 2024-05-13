#ifdef __APPLE__

#pragma once

#include <mach/mach.h>
#include "../IDolphinProcess.h"

namespace DolphinComm
{
class MacDolphinProcess : public IDolphinProcess
{
public:
  MacDolphinProcess() = default;
  bool findPID() override;
  bool obtainEmuRAMInformations() override;
  bool readFromRAM(u32 offset, char* buffer, size_t size, bool withBSwap) override;
  bool writeToRAM(u32 offset, const char* buffer, size_t size, bool withBSwap) override;

private:
  task_t m_task;
  task_t m_currentTask;
};
}  // namespace DolphinComm
#endif
