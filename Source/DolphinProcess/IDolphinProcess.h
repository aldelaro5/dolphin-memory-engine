// Interface to perform operations in Dolphin's process
#pragma once

#include <cstddef>

#include "../Common/CommonTypes.h"
#include "../Common/CommonUtils.h"

namespace DolphinComm
{
class IDolphinProcess
{
public:
  IDolphinProcess() = default;
  virtual ~IDolphinProcess() = default;

  void reset()
  {
    m_PID = -1;
    m_emuRAMAddressStart = 0;
    m_emuARAMAdressStart = 0;
    m_MEM2AddressStart = 0;
    m_ARAMAccessible = false;
    m_MEM2Present = false;
    m_ARAMSize = 0;
    m_MEM1Size = 0;
    m_MEM2Size = 0;
  }

  IDolphinProcess(const IDolphinProcess&) = delete;
  IDolphinProcess(IDolphinProcess&&) = delete;
  IDolphinProcess& operator=(const IDolphinProcess&) = delete;
  IDolphinProcess& operator=(IDolphinProcess&&) = delete;

  virtual bool findPID() = 0;
  virtual bool obtainEmuRAMInformations() = 0;
  virtual bool readFromRAM(u32 offset, char* buffer, size_t size, bool withBSwap) = 0;
  virtual bool writeToRAM(u32 offset, const char* buffer, size_t size, bool withBSwap) = 0;

  int getPID() const { return m_PID; };
  u64 getEmuRAMAddressStart() const { return m_emuRAMAddressStart; };
  bool isMEM2Present() const { return m_MEM2Present; };
  bool isARAMAccessible() const { return m_ARAMAccessible; };
  u64 getARAMAddressStart() const { return m_emuARAMAdressStart; };
  u64 getMEM2AddressStart() const { return m_MEM2AddressStart; };

  u32 getARAMSize() const { return m_ARAMSize; }
  u32 getMEM1Size() const { return m_MEM1Size; }
  u32 getMEM2Size() const { return m_MEM2Size; }

protected:
  int m_PID = -1;
  u64 m_emuRAMAddressStart = 0;
  u64 m_emuARAMAdressStart = 0;
  u64 m_MEM2AddressStart = 0;
  bool m_ARAMAccessible = false;
  bool m_MEM2Present = false;
  u32 m_ARAMSize = 0;
  u32 m_MEM1Size = 0;
  u32 m_MEM2Size = 0;
};

constexpr u32 SYSTEM_INFO_OFFSET{0x00000018};

struct SystemInfo
{
  u8 discMagicWordWii[4];
  u8 discMagicWordGC[4];
  u8 bootCode[4];
  u8 version[4];
  u32 physicalMemorySize;

  bool isNull() const
  {
    return !(discMagicWordWii[0] || discMagicWordWii[1] || discMagicWordWii[2] ||
             discMagicWordWii[3] || discMagicWordGC[0] || discMagicWordGC[1] ||
             discMagicWordGC[2] || discMagicWordGC[3] || bootCode[0] || bootCode[1] ||
             bootCode[2] || bootCode[3] || version[0] || version[1] || version[2] || version[3]);
  }

  bool isDiscMagicWordWiiKnown() const
  {
    // Only known value is 0x5D1C9EA3 (Nintendo Wii Disc).
    return discMagicWordWii[0] == 0x5D && discMagicWordWii[1] == 0x1C &&
           discMagicWordWii[2] == 0x9E && discMagicWordWii[3] == 0xA3;
  }

  bool isDiscMagicWordGCKnown() const
  {
    // Only known value is 0xC2339F3D (Nintendo GameCube Disc).
    return discMagicWordGC[0] == 0xC2 && discMagicWordGC[1] == 0x33 && discMagicWordGC[2] == 0x9F &&
           discMagicWordGC[3] == 0x3D;
  }

  bool isBootCodeKnown() const
  {
    // Only known values are:
    // - 0x0D15EA5E (normal boot)
    // - 0xE5207C22 (booted from jtag)
    return (bootCode[0] == 0x0D && bootCode[1] == 0x15 && bootCode[2] == 0xEA &&
            bootCode[3] == 0x5E) ||
           (bootCode[0] == 0xE5 && bootCode[1] == 0x20 && bootCode[2] == 0x7C &&
            bootCode[3] == 0x22);
  }

  u32 getPhysicalMemorySize() const { return Common::bSwap32(physicalMemorySize); }
};

constexpr u32 DOLPHIN_OS_GLOBALS_OFFSET{0x00000084};

struct DolphinOSGlobals
{
  u8 padding[60];
  u8 currentOSContextPhysicalAddress[4];
  u8 previousOSInterruptMask[4];
  u8 currentOSInterruptMask[4];
  u8 tvMode[4];
  u32 aramSize;
  u8 currentOSContext[4];
  u8 defaultOSThread[4];
  u8 activeThreadQueueHeadThread[4];
  u8 activeThreadQueueTailThread[4];
  u8 currentOSThread[4];
  u8 debugMonitorSize[4];
  u8 debugMonitorLocation[4];
  u32 simulatedMemorySize;
  u8 dvdBi2Location[4];
  u8 busClockSpeed[4];
  u8 cpuClockSpeed[4];

  u32 getARAMSize() const { return Common::bSwap32(aramSize); }
  u32 getSimulatedMemorySize() const { return Common::bSwap32(simulatedMemorySize); }
};

constexpr u32 WII_SPECIFIC_INFO_OFFSET{0x00003100};

struct WiiSpecificInfo
{
  u32 physicalMEM1Size;
  u32 simulatedMEM1Size;
  u8 unknown[16];
  u32 physicalMEM2Size;
  u32 simulatedMEM2Size;

  u32 getPhysicalMEM1Size() const { return Common::bSwap32(physicalMEM1Size); }
  u32 getSimulatedMEM1Size() const { return Common::bSwap32(simulatedMEM1Size); }
  u32 getPhysicalMEM2Size() const { return Common::bSwap32(physicalMEM2Size); }
  u32 getSimulatedMEM2Size() const { return Common::bSwap32(simulatedMEM2Size); }
};
}  // namespace DolphinComm
