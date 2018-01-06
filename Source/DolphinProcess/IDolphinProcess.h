// Interface to perform operations in Dolphin's process
#pragma once

#include <cstddef>

#include "../Common/CommonTypes.h"

namespace DolphinComm
{
class IDolphinProcess
{
public:
  virtual ~IDolphinProcess()
  {
  }
  virtual bool findPID() = 0;
  virtual bool obtainEmuRAMInformations() = 0;
  virtual bool readFromRAM(const u32 offset, char* buffer, const size_t size,
                           const bool withBSwap) = 0;
  virtual bool writeToRAM(const u32 offset, const char* buffer, const size_t size,
                          const bool withBSwap) = 0;

  int getPID() const
  {
    return m_PID;
  };
  u64 getEmuRAMAddressStart() const
  {
    return m_emuRAMAddressStart;
  };
  u64 isMEM2Present() const
  {
    return m_MEM2Present;
  };

protected:
  int m_PID = -1;
  u64 m_emuRAMAddressStart = 0;
  bool m_MEM2Present = false;
};
} // namespace DolphinComm
