#include "DolphinAccessor.h"
#ifdef __linux__
#include "Linux/LinuxDolphinProcess.h"
#elif _WIN32
#include "Windows/WindowsDolphinProcess.h"
#elif __APPLE__
#include "Mac/MacDolphinProcess.h"
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
  Common::UpdateMemoryValues();
  if (m_instance == nullptr)
  {
#ifdef __linux__
    m_instance = new LinuxDolphinProcess();
#elif _WIN32
    m_instance = new WindowsDolphinProcess();
#elif __APPLE__
    m_instance = new MacDolphinProcess();
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
  return m_instance ? m_instance->readFromRAM(offset, buffer, size, withBSwap) : false;
}

bool DolphinAccessor::writeToRAM(const u32 offset, const char* buffer, const size_t size,
                                 const bool withBSwap)
{
  return m_instance ? m_instance->writeToRAM(offset, buffer, size, withBSwap) : false;
}

int DolphinAccessor::getPID()
{
  return m_instance ? m_instance->getPID() : -1;
}

u64 DolphinAccessor::getEmuRAMAddressStart()
{
  return m_instance ? m_instance->getEmuRAMAddressStart() : 0;
}

bool DolphinAccessor::isARAMAccessible()
{
  return m_instance ? m_instance->isARAMAccessible() : false;
}

u64 DolphinAccessor::getARAMAddressStart()
{
  return m_instance ? m_instance->getARAMAddressStart() : 0;
}

bool DolphinAccessor::isMEM2Present()
{
  return m_instance ? m_instance->isMEM2Present() : false;
}

bool DolphinAccessor::isValidConsoleAddress(const u32 address)
{
  if (getStatus() != DolphinStatus::hooked)
    return false;

  if (address >= Common::MEM1_START && address < Common::GetMEM1End())
    return true;

  if (isMEM2Present() && (address >= Common::MEM2_START && address < Common::GetMEM2End()))
    return true;

  if (isARAMAccessible() && (address >= Common::ARAM_START && address < Common::ARAM_END))
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
    return Common::GetMEM1SizeReal() + Common::GetMEM2SizeReal();
  }
  else if (isARAMAccessible())
  {
    return Common::GetMEM1SizeReal() + Common::ARAM_SIZE;
  }
  else
  {
    return Common::GetMEM1SizeReal();
  }
}

Common::MemOperationReturnCode DolphinAccessor::updateRAMCache()
{
  delete[] m_updatedRAMCache;
  m_updatedRAMCache = nullptr;

  // MEM2, if enabled, is read right after MEM1 in the cache so both regions are contigous
  if (isMEM2Present())
  {
    m_updatedRAMCache = new char[Common::GetMEM1SizeReal() + Common::GetMEM2SizeReal()];

    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, false), m_updatedRAMCache,
            Common::GetMEM1SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;

    // Read Wii extra RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM2_START, false),
            m_updatedRAMCache + Common::GetMEM1SizeReal(), Common::GetMEM2SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else if (isARAMAccessible())
  {
    m_updatedRAMCache = new char[Common::ARAM_SIZE + Common::GetMEM1SizeReal()];
    // read ARAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::ARAM_START, true), m_updatedRAMCache,
            Common::ARAM_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;

    // Read GameCube and Wii basic RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, true),
            m_updatedRAMCache + Common::ARAM_SIZE, Common::GetMEM1SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else
  {
    m_updatedRAMCache = new char[Common::GetMEM1SizeReal()];
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, false), m_updatedRAMCache,
            Common::GetMEM1SizeReal(), false))
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
