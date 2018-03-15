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

bool DolphinAccessor::isMEM2Present()
{
  return m_instance->isMEM2Present();
}

bool DolphinAccessor::isValidConsoleAddress(const u32 address)
{
  if (getStatus() != DolphinStatus::hooked)
    return false;

  bool isMem1Address = address >= Common::MEM1_START && address < Common::MEM1_END;
  if (isMEM2Present())
    return isMem1Address || (address >= Common::MEM2_START && address < Common::MEM2_END);
  return isMem1Address;
}

Common::MemOperationReturnCode DolphinAccessor::updateRAMCache()
{
  delete[] m_updatedRAMCache;
  m_updatedRAMCache = nullptr;

  // MEM2, if enabled, is read right after MEM1 in the cache so both regions are contigous
  if (isMEM2Present())
  {
    m_updatedRAMCache = new char[Common::MEM1_SIZE + Common::MEM2_SIZE];
    // Read Wii extra RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(Common::dolphinAddrToOffset(Common::MEM2_START),
                                                   m_updatedRAMCache + Common::MEM1_SIZE,
                                                   Common::MEM2_SIZE, false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else
  {
    m_updatedRAMCache = new char[Common::MEM1_SIZE];
  }

  // Read GameCube and Wii basic RAM
  if (!DolphinComm::DolphinAccessor::readFromRAM(0, m_updatedRAMCache, Common::MEM1_SIZE, false))
    return Common::MemOperationReturnCode::operationFailed;
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
    u32 offset = Common::dolphinAddrToOffset(consoleAddress);
    u32 ramIndex = 0;
    if (offset >= Common::MEM1_SIZE)
      ramIndex = offset - (Common::MEM2_START - Common::MEM1_END);
    else
      ramIndex = offset;
    std::memcpy(dest, m_updatedRAMCache + ramIndex, byteCount);
  }
}
} // namespace DolphinComm
