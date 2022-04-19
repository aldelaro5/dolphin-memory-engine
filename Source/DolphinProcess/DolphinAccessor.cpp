#include "DolphinAccessor.h"
#ifdef linux
#include "Linux/LinuxDolphinProcess.h"
#elif _WIN32
#include "Windows/WindowsDolphinProcess.h"
#endif

#include <cstring>

#include "../Common/CommonUtils.h"
#include "../Common/MemoryCommon.h"

namespace DolphinComm
{
IDolphinProcess* DolphinAccessor::m_instance = nullptr;
DolphinAccessor::DolphinStatus DolphinAccessor::m_status = DolphinStatus::unHooked;
char* DolphinAccessor::m_updatedRAMCache = nullptr;

void DolphinAccessor::init()
{
  if (m_instance == nullptr)
  {
#ifdef __linux__
    m_instance = new LinuxDolphinProcess();
#elif _WIN32
    m_instance = new WindowsDolphinProcess();
#endif
  }
}

void DolphinAccessor::free()
{
  delete m_instance;
  delete[] m_updatedRAMCache;
}

void DolphinAccessor::hook()
{
  init();
  if (!m_instance->findPID())
  {
    m_status = DolphinStatus::notRunning;
  }
  else if (!m_instance->obtainEmuRAMInformations())
  {
    m_status = DolphinStatus::noEmu;
  }
  else
  {
    m_status = DolphinStatus::hooked;
    updateRAMCache();
  }
}

void DolphinAccessor::unHook()
{
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

bool DolphinAccessor::isARAMAccessible()
{
  return m_instance->isARAMAccessible();
}

u64 DolphinAccessor::getARAMAddressStart()
{
  return m_instance->getARAMAddressStart();
}

bool DolphinAccessor::isMEM2Present()
{
  return m_instance->isMEM2Present();
}

bool DolphinAccessor::isValidConsoleAddress(const u32 address)
{
  if (getStatus() != DolphinStatus::hooked)
    return false;

  if (address >= Common::MEM1_START && address < Common::MEM1_END)
    return true;

  if (isMEM2Present() && (address >= Common::MEM2_START && address < Common::MEM2_END))
    return true;

  if (isARAMAccessible && (address >= Common::ARAM_START && address < Common::ARAM_END))
    return true;

  return false;
}

char* DolphinAccessor::getRAMCache()
{
  return m_updatedRAMCache;
}

size_t DolphinAccessor::getRAMCacheSize()
{
  if (isMEM2Present())
  {
    return Common::MEM1_SIZE + Common::MEM2_SIZE;
  }
  else if (isARAMAccessible())
  {
    return Common::MEM1_SIZE + Common::ARAM_SIZE;
  }
  else
  {
    return Common::MEM1_SIZE;
  }
}

Common::MemOperationReturnCode DolphinAccessor::updateRAMCache()
{
  delete[] m_updatedRAMCache;
  m_updatedRAMCache = nullptr;

  // MEM2, if enabled, is read right after MEM1 in the cache so both regions are contigous
  if (isMEM2Present())
  {
    m_updatedRAMCache = new char[Common::MEM1_SIZE + Common::MEM2_SIZE];

    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, false), m_updatedRAMCache,
            Common::MEM1_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;

    // Read Wii extra RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM2_START, false),
            m_updatedRAMCache + Common::MEM1_SIZE, Common::MEM2_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else if (isARAMAccessible())
  {
    m_updatedRAMCache = new char[Common::ARAM_SIZE + Common::MEM1_SIZE];
    // read ARAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::ARAM_START, true), m_updatedRAMCache,
            Common::ARAM_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;

    // Read GameCube and Wii basic RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, true),
            m_updatedRAMCache + Common::ARAM_SIZE, Common::MEM1_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else
  {
    m_updatedRAMCache = new char[Common::MEM1_SIZE];
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, false), m_updatedRAMCache,
            Common::MEM1_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;
  }

  return Common::MemOperationReturnCode::OK;
}

std::string DolphinAccessor::getFormattedValueFromCache(const u32 ramIndex, Common::MemType memType,
                                                        size_t memSize, Common::MemBase memBase,
                                                        bool memIsUnsigned)
{
  return Common::formatMemoryToString(&m_updatedRAMCache[ramIndex], memType, memSize, memBase,
                                      memIsUnsigned, Common::shouldBeBSwappedForType(memType));
}

void DolphinAccessor::copyRawMemoryFromCache(char* dest, const u32 consoleAddress,
                                             const size_t byteCount)
{
  if (isValidConsoleAddress(consoleAddress) &&
      isValidConsoleAddress((consoleAddress + static_cast<u32>(byteCount)) - 1))
  {
    bool aramAccessible = isARAMAccessible();
    u32 offset = Common::dolphinAddrToOffset(consoleAddress, isARAMAccessible());
    u32 cacheIndex = Common::offsetToCacheIndex(offset, aramAccessible);
    std::memcpy(dest, m_updatedRAMCache + cacheIndex, byteCount);
  }
}
} // namespace DolphinComm
