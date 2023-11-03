#ifdef __linux__

#include "LinuxDolphinProcess.h"
#include "../../Common/CommonUtils.h"
#include "../../Common/MemoryCommon.h"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/uio.h>
#include <vector>

namespace DolphinComm
{
bool LinuxDolphinProcess::obtainEmuRAMInformations()
{
  std::ifstream theMapsFile("/proc/" + std::to_string(m_PID) + "/maps");
  std::string line;
  bool MEM1Found = false;
  while (getline(theMapsFile, line))
  {
    std::vector<std::string> lineData;
    std::string token;
    std::stringstream ss(line);
    while (getline(ss, token, ' '))
    {
      if (!token.empty())
        lineData.push_back(token);
    }

    if (lineData.size() < 3)
      continue;

    bool foundDevShmDolphin = false;
    for (auto str : lineData)
    {
      if (str.substr(0, 19) == "/dev/shm/dolphinmem" || str.substr(0, 20) == "/dev/shm/dolphin-emu")
      {
        foundDevShmDolphin = true;
        break;
      }
    }

    if (!foundDevShmDolphin)
      continue;

    u32 offset = 0;
    std::string offsetStr("0x" + lineData[2]);
    offset = std::stoul(offsetStr, nullptr, 16);
    if (offset != 0 && offset != Common::GetMEM1Size() + 0x40000)
      continue;

    u64 firstAddress = 0;
    u64 SecondAddress = 0;
    int indexDash = lineData[0].find('-');
    std::string firstAddressStr("0x" + lineData[0].substr(0, indexDash));
    std::string secondAddressStr("0x" + lineData[0].substr(indexDash + 1));

    firstAddress = std::stoul(firstAddressStr, nullptr, 16);
    SecondAddress = std::stoul(secondAddressStr, nullptr, 16);

    if (SecondAddress - firstAddress == Common::GetMEM2Size() &&
        offset == Common::GetMEM1Size() + 0x40000)
    {
      m_MEM2AddressStart = firstAddress;
      m_MEM2Present = true;
      if (MEM1Found)
        break;
    }

    if (SecondAddress - firstAddress == Common::GetMEM1Size())
    {
      if (offset == 0x0)
      {
        m_emuRAMAddressStart = firstAddress;
        MEM1Found = true;
      }
      else if (offset == Common::GetMEM1Size() + 0x40000)
      {
        m_emuARAMAdressStart = firstAddress;
        m_ARAMAccessible = true;
      }
    }
  }

  // On Wii, there is no concept of speedhack so act as if we couldn't find it
  if (m_MEM2Present)
  {
    m_emuARAMAdressStart = 0;
    m_ARAMAccessible = false;
  }

  if (m_emuRAMAddressStart != 0)
    return true;

  // Here, Dolphin appears to be running, but the emulator isn't started
  return false;
}

bool LinuxDolphinProcess::findPID()
{
  DIR* directoryPointer = opendir("/proc/");
  if (directoryPointer == nullptr)
    return false;

  struct dirent* directoryEntry = nullptr;
  while (m_PID == -1 && (directoryEntry = readdir(directoryPointer)))
  {
    std::istringstream conversionStream(directoryEntry->d_name);
    int aPID = 0;
    if (!(conversionStream >> aPID))
      continue;
    std::ifstream aCmdLineFile;
    std::string line;
    aCmdLineFile.open("/proc/" + std::string(directoryEntry->d_name) + "/comm");
    getline(aCmdLineFile, line);
    if (line == "dolphin-emu" || line == "dolphin-emu-qt2" || line == "dolphin-emu-wx")
      m_PID = aPID;

    aCmdLineFile.close();
  }
  closedir(directoryPointer);

  if (m_PID == -1)
    // Here, Dolphin apparently isn't running on the system
    return false;
  return true;
}

bool LinuxDolphinProcess::readFromRAM(const u32 offset, char* buffer, const size_t size,
                                      const bool withBSwap)
{
  struct iovec local;
  struct iovec remote;
  size_t nread;
  u64 RAMAddress = 0;
  if (m_ARAMAccessible)
  {
    if (offset >= Common::ARAM_FAKESIZE)
      RAMAddress = m_emuRAMAddressStart + offset - Common::ARAM_FAKESIZE;
    else
      RAMAddress = m_emuARAMAdressStart + offset;
  }
  else if (offset >= (Common::MEM2_START - Common::MEM1_START))
  {
    RAMAddress = m_MEM2AddressStart + offset - (Common::MEM2_START - Common::MEM1_START);
  }
  else
  {
    RAMAddress = m_emuRAMAddressStart + offset;
  }

  local.iov_base = buffer;
  local.iov_len = size;
  remote.iov_base = (void*)RAMAddress;
  remote.iov_len = size;

  nread = process_vm_readv(m_PID, &local, 1, &remote, 1, 0);
  if (nread != size)
    return false;

  if (withBSwap)
  {
    switch (size)
    {
    case 2:
    {
      u16 halfword = 0;
      std::memcpy(&halfword, buffer, sizeof(u16));
      halfword = Common::bSwap16(halfword);
      std::memcpy(buffer, &halfword, sizeof(u16));
      break;
    }
    case 4:
    {
      u32 word = 0;
      std::memcpy(&word, buffer, sizeof(u32));
      word = Common::bSwap32(word);
      std::memcpy(buffer, &word, sizeof(u32));
      break;
    }
    case 8:
    {
      u64 doubleword = 0;
      std::memcpy(&doubleword, buffer, sizeof(u64));
      doubleword = Common::bSwap64(doubleword);
      std::memcpy(buffer, &doubleword, sizeof(u64));
      break;
    }
    }
  }

  return true;
}

bool LinuxDolphinProcess::writeToRAM(const u32 offset, const char* buffer, const size_t size,
                                     const bool withBSwap)
{
  struct iovec local;
  struct iovec remote;
  size_t nwrote;
  u64 RAMAddress = 0;
  if (m_ARAMAccessible)
  {
    if (offset >= Common::ARAM_FAKESIZE)
      RAMAddress = m_emuRAMAddressStart + offset - Common::ARAM_FAKESIZE;
    else
      RAMAddress = m_emuARAMAdressStart + offset;
  }
  else if (offset >= (Common::MEM2_START - Common::MEM1_START))
  {
    RAMAddress = m_MEM2AddressStart + offset - (Common::MEM2_START - Common::MEM1_START);
  }
  else
  {
    RAMAddress = m_emuRAMAddressStart + offset;
  }

  char* bufferCopy = new char[size];
  std::memcpy(bufferCopy, buffer, size);

  local.iov_base = bufferCopy;
  local.iov_len = size;
  remote.iov_base = (void*)RAMAddress;
  remote.iov_len = size;

  if (withBSwap)
  {
    switch (size)
    {
    case 2:
    {
      u16 halfword = 0;
      std::memcpy(&halfword, bufferCopy, sizeof(u16));
      halfword = Common::bSwap16(halfword);
      std::memcpy(bufferCopy, &halfword, sizeof(u16));
      break;
    }
    case 4:
    {
      u32 word = 0;
      std::memcpy(&word, bufferCopy, sizeof(u32));
      word = Common::bSwap32(word);
      std::memcpy(bufferCopy, &word, sizeof(u32));
      break;
    }
    case 8:
    {
      u64 doubleword = 0;
      std::memcpy(&doubleword, bufferCopy, sizeof(u64));
      doubleword = Common::bSwap64(doubleword);
      std::memcpy(bufferCopy, &doubleword, sizeof(u64));
      break;
    }
    }
  }

  nwrote = process_vm_writev(m_PID, &local, 1, &remote, 1, 0);
  delete[] bufferCopy;
  if (nwrote != size)
    return false;

  return true;
}
} // namespace DolphinComm
#endif
