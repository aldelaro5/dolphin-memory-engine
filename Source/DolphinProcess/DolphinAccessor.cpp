#include "DolphinAccessor.h"
#ifdef __linux__
#include "Linux/LinuxDolphinProcess.h"
#elif _WIN32
#include "Windows/WindowsDolphinProcess.h"
#elif __APPLE__
#include "Mac/MacDolphinProcess.h"
#endif

#include <cstring>
#include <memory>

#include "../Common/CommonUtils.h"
#include "../Common/MemoryCommon.h"

namespace DolphinComm
{
IDolphinProcess* DolphinAccessor::m_instance = nullptr;
DolphinAccessor::DolphinStatus DolphinAccessor::m_status = DolphinStatus::unHooked;

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

size_t DolphinAccessor::getRAMTotalSize()
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

Common::MemOperationReturnCode DolphinAccessor::readEntireRAM(char* buffer)
{
  // MEM2, if enabled, is read right after MEM1 in the buffer so both regions are contigous
  if (isMEM2Present())
  {
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, false), buffer,
            Common::GetMEM1SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;

    // Read Wii extra RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM2_START, false),
            buffer + Common::GetMEM1SizeReal(), Common::GetMEM2SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else if (isARAMAccessible())
  {
    // read ARAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::ARAM_START, true), buffer, Common::ARAM_SIZE,
            false))
      return Common::MemOperationReturnCode::operationFailed;

    // Read GameCube and Wii basic RAM
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, true), buffer + Common::ARAM_SIZE,
            Common::GetMEM1SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;
  }
  else
  {
    if (!DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(Common::MEM1_START, false), buffer,
            Common::GetMEM1SizeReal(), false))
      return Common::MemOperationReturnCode::operationFailed;
  }

  return Common::MemOperationReturnCode::OK;
}

std::string DolphinAccessor::getFormattedValueFromMemory(const u32 ramIndex,
                                                         Common::MemType memType, size_t memSize,
                                                         Common::MemBase memBase,
                                                         bool memIsUnsigned)
{
  std::unique_ptr<char[]> buffer(new char[memSize]);
  readFromRAM(ramIndex, buffer.get(), memSize, false);
  return Common::formatMemoryToString(buffer.get(), memType, memSize, memBase, memIsUnsigned,
                                      Common::shouldBeBSwappedForType(memType));
}
}  // namespace DolphinComm
