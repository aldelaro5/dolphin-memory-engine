#ifdef __linux__

#include "LinuxDolphinProcess.h"
#include "../../Common/CommonUtils.h"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/uio.h>

namespace DolphinComm
{
bool LinuxDolphinProcess::obtainEmuRAMInformations()
{
  std::ifstream theMapsFile("/proc/" + std::to_string(m_PID) + "/maps");
  std::string line;
  bool MEM1Found = false;
  while (getline(theMapsFile, line))
  {
    if (line.length() > 73)
    {
      if (line.substr(73, 19) == "/dev/shm/dolphinmem" ||
          line.substr(73, 20) == "/dev/shm/dolphin-emu")
      {
        u64 firstAddress = 0;
        u64 SecondAddress = 0;
        std::string firstAddressStr("0x" + line.substr(0, 12));
        std::string secondAddressStr("0x" + line.substr(13, 12));

        firstAddress = std::stoul(firstAddressStr, nullptr, 16);
        SecondAddress = std::stoul(secondAddressStr, nullptr, 16);

        if (MEM1Found)
        {
          if (firstAddress == m_emuRAMAddressStart + 0x10000000)
          {
            m_MEM2Present = true;
            break;
          }
          else if (firstAddress > m_emuRAMAddressStart + 0x10000000)
          {
            m_MEM2Present = false;
            break;
          }
          continue;
        }

        u64 test = SecondAddress - firstAddress;

        if (SecondAddress - firstAddress == 0x2000000)
        {
          m_emuRAMAddressStart = firstAddress;
          MEM1Found = true;
        }
      }
    }
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
  u64 RAMAddress = m_emuRAMAddressStart + offset;

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
  u64 RAMAddress = m_emuRAMAddressStart + offset;
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
