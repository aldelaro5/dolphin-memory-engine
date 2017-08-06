#include "DolphinAccessor.h"
#ifdef linux
#include "Linux/LinuxDolphinProcess.h"
#elif _WIN32
#include "Windows/WindowsDolphinProcess.h"
#endif

#include "../Common/MemoryCommon.h"

namespace DolphinComm
{
IDolphinProcess* DolphinAccessor::m_instance = nullptr;
DolphinAccessor::DolphinStatus DolphinAccessor::m_status = DolphinStatus::unHooked;
bool DolphinAccessor::m_mem2Enabled = false;

void DolphinAccessor::hook()
{
  if (m_instance == nullptr)
  {
#ifdef __linux__
    m_instance = new LinuxDolphinProcess();
#elif _WIN32
    m_instance = new WindowsDolphinProcess();
#endif
  }

  if (!m_instance->findPID())
  {
    m_status = DolphinStatus::notRunning;
  }
  else if (!m_instance->findEmuRAMStartAddress())
  {
    m_status = DolphinStatus::noEmu;
  }
  else
  {
    m_status = DolphinStatus::hooked;
  }
}

void DolphinAccessor::unHook()
{
  if (m_instance != nullptr)
    delete m_instance;
  m_instance = nullptr;
  m_status = DolphinStatus::unHooked;
}

DolphinAccessor::DolphinStatus DolphinAccessor::getStatus()
{
  return m_status;
}

bool DolphinAccessor::readFromRAM(const u32 offset, char* buffer, const size_t size,
                                  const bool withBSwap)
{
  return m_instance->readFromRAM(offset, buffer, size, withBSwap);
}

bool DolphinAccessor::writeToRAM(const u32 offset, const char* buffer, const size_t size,
                                 const bool withBSwap)
{
  return m_instance->writeToRAM(offset, buffer, size, withBSwap);
}

int DolphinAccessor::getPID()
{
  return m_instance->getPID();
}

u64 DolphinAccessor::getEmuRAMAddressStart()
{
  return m_instance->getEmuRAMAddressStart();
}

void DolphinAccessor::enableMem2(const bool doEnable)
{
  m_mem2Enabled = doEnable;
}

bool DolphinAccessor::isMem2Enabled()
{
  return m_mem2Enabled;
}

void DolphinAccessor::autoDetectMem2()
{
  if (getStatus() == DolphinStatus::hooked)
  {
    char gameIdFirstChar = ' ';
    if (readFromRAM(0, &gameIdFirstChar, 1, false))
    {
      switch (gameIdFirstChar)
      {
      case 'G': // Gamecube disc
      case 'P': // Promotional games, guaranteed to be Gamecube
        m_mem2Enabled = false;
        break;

      case 'R': // Wii disc, can be prototypes, but unlikely
      case 'S': // Later Wii disc
        m_mem2Enabled = true;
        break;

      // If there's no ID, likely to be a Wiiware, but this isn't guaranteed, could also be D, but
      // this one is present on both, so let's just leave MEM2 enabled for these by default, the
      // user can be the judge here, these are extremely unlikely cases anyway
      default:
        m_mem2Enabled = true;
        break;
      }
    }
    else
    {
      unHook();
    }
  }
}

bool DolphinAccessor::isValidConsoleAddress(const u32 address)
{
  bool isMem1Address = address >= Common::MEM1_START && address < Common::MEM1_END;
  if (m_mem2Enabled)
  {
    return isMem1Address || (address >= Common::MEM2_START && address < Common::MEM2_END);
  }
  return isMem1Address;
}
}
