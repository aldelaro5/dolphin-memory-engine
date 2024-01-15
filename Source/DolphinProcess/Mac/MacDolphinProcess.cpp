#ifdef __APPLE__

#include "MacDolphinProcess.h"
#include "../../Common/CommonUtils.h"

#include <memory>
#include <sys/sysctl.h>

namespace DolphinComm
{
bool MacDolphinProcess::findPID()
{
  static const int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  size_t procSize = 0;
  if(sysctl((int*) mib, 4, NULL, &procSize, NULL, 0) == -1)
    return false;

  auto procs = std::make_unique<kinfo_proc[]>(procSize / sizeof(kinfo_proc));
  if(sysctl((int*) mib, 4, procs.get(), &procSize, NULL, 0) == -1)
    return false;

  for(int i = 0; i < procSize / sizeof(kinfo_proc); i++)
  {
    if(std::strcmp(procs[i].kp_proc.p_comm, "Dolphin") == 0 ||
       std::strcmp(procs[i].kp_proc.p_comm, "dolphin-emu") == 0)
    {
      m_PID = procs[i].kp_proc.p_pid;
    }
  }

  if (m_PID == -1)
    return false;
  return true;
}

bool MacDolphinProcess::obtainEmuRAMInformations()
{
  return false;
}

bool MacDolphinProcess::readFromRAM(const u32 offset, char* buffer, size_t size, const bool withBSwap)
{
  return false;
}

bool MacDolphinProcess::writeToRAM(const u32 offset, const char* buffer, const size_t size, const bool withBSwap)
{
  return false;
}
} // namespace DolphinComm
#endif
