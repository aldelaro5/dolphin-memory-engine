#ifdef __linux__

#include "LinuxDolphinProcess.h"
#include "../../Common/CommonUtils.h"
#include "../../Common/MemoryCommon.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/uio.h>
#include <vector>

namespace
{
bool readFromRAM(const pid_t pid, const u64 ramAddress, const u32 offset, char* const buffer,
                 const size_t size)
{
  iovec local{};
  local.iov_base = buffer;
  local.iov_len = size;

  iovec remote{};
  remote.iov_base = reinterpret_cast<void*>(ramAddress + offset);
  remote.iov_len = size;

  const ssize_t nread{process_vm_readv(pid, &local, 1, &remote, 1, 0)};
  if (nread == -1)
  {
    // A more specific error type should be available in `errno` (if ever interested).
    return false;
  }

  if (static_cast<size_t>(nread) != size)
    return false;

  return true;
}
}  // namespace

namespace DolphinComm
{
bool LinuxDolphinProcess::obtainEmuRAMInformations()
{
  struct MemoryMapLine
  {
    const std::string line;
    const u64 firstAddress;
    const u64 secondAddress;
    const u32 offset;
  };

  // Parse memory map of the Dolphin process.
  std::vector<MemoryMapLine> memoryMapLines;
  {
    std::ifstream mapsFile("/proc/" + std::to_string(m_PID) + "/maps");

    std::string line;
    while (getline(mapsFile, line))
    {
      if (line.find("/dev/shm/dolphin-emu") == std::string::npos &&
          line.find("/dev/shm/dolphinmem") == std::string::npos)
        continue;

      const std::vector<std::string> lineParts{Common::splitBySpace(line)};
      if (lineParts.size() < 3)
        continue;

      const std::size_t indexDash{lineParts[0].find('-')};
      const std::string firstAddressStr{"0x" + lineParts[0].substr(0, indexDash)};
      const std::string secondAddressStr{"0x" + lineParts[0].substr(indexDash + 1)};
      const u64 firstAddress{std::stoul(firstAddressStr, nullptr, 16)};
      const u64 secondAddress{std::stoul(secondAddressStr, nullptr, 16)};
      const std::string offsetStr("0x" + lineParts[2]);
      const u32 offset{static_cast<u32>(std::stoul(offsetStr, nullptr, 16))};
      memoryMapLines.push_back({line, firstAddress, secondAddress, offset});
    }
  }

  SystemInfo systemInfo{};
  DolphinOSGlobals dolphinOSGlobals{};
  WiiSpecificInfo wiiSpecificInfo{};

  // In a first pass, attempt to parse data structures from the top of the memory region.
  for (const auto& [line, firstAddress, secondAddress, offset] : memoryMapLines)
  {
    if (offset)
      continue;

    if (!::readFromRAM(m_PID, firstAddress, SYSTEM_INFO_OFFSET,
                       reinterpret_cast<char*>(&systemInfo), sizeof(systemInfo)) ||
        !::readFromRAM(m_PID, firstAddress, DOLPHIN_OS_GLOBALS_OFFSET,
                       reinterpret_cast<char*>(&dolphinOSGlobals), sizeof(dolphinOSGlobals)) ||
        !::readFromRAM(m_PID, firstAddress, WII_SPECIFIC_INFO_OFFSET,
                       reinterpret_cast<char*>(&wiiSpecificInfo), sizeof(wiiSpecificInfo)))
      continue;

    // GameCube or Triforce game.
    if (systemInfo.isDiscMagicWordGCKnown() && systemInfo.isBootCodeKnown())
    {
      m_emuRAMAddressStart = firstAddress;
      m_MEM1Size = dolphinOSGlobals.getSimulatedMemorySize();
      m_ARAMSize = dolphinOSGlobals.getARAMSize();
      break;
    }

    // Wii game.
    if (systemInfo.isDiscMagicWordWiiKnown() && systemInfo.isBootCodeKnown())
    {
      m_emuRAMAddressStart = firstAddress;
      m_MEM1Size = wiiSpecificInfo.getSimulatedMEM1Size();
      m_MEM2Size = wiiSpecificInfo.getSimulatedMEM2Size();
      break;
    }

    // WAD game.
    // NOTE: All magic words happen to be null when a WAD game is running. One consistency check
    // that can be performed is compare the physical and simulated memory in the two structures
    // where they are written: if they match and have a reasonable size, it is likely a WAD game.
    if (systemInfo.isNull() &&
        Common::isInBounds(0x01800000U, systemInfo.getPhysicalMemorySize(), 0x08000000U) &&
        systemInfo.getPhysicalMemorySize() == wiiSpecificInfo.getPhysicalMEM1Size() &&
        Common::isInBounds(0x01800000U, dolphinOSGlobals.getSimulatedMemorySize(), 0x08000000U) &&
        dolphinOSGlobals.getSimulatedMemorySize() == wiiSpecificInfo.getSimulatedMEM1Size())
    {
      m_emuRAMAddressStart = firstAddress;
      m_MEM1Size = wiiSpecificInfo.getSimulatedMEM1Size();
      m_MEM2Size = wiiSpecificInfo.getSimulatedMEM2Size();
      break;
    }
  }

  // Second loop to find either the ARAM address (GameCube or Triforce) or the MEM2 address (Wii).
  for (const auto& [line, firstAddress, secondAddress, offset] : memoryMapLines)
  {
    const u64 size{secondAddress - firstAddress};

    if (m_ARAMSize && size == Common::NextPowerOf2(m_MEM1Size) &&
        (offset & 0x00040000) == 0x00040000)
    {
      m_emuARAMAdressStart = firstAddress;
      m_ARAMAccessible = true;
      break;
    }

    if (m_MEM2Size && size == Common::NextPowerOf2(m_MEM2Size) &&
        (offset & 0x00040000) == 0x00040000)
    {
      m_MEM2AddressStart = firstAddress;
      m_MEM2Present = true;
      break;
    }

    // TODO(CA): Ideally, we'd inspect the memory to try to determine whether it's pointing to the
    // expected data, as opposed to relying on these fragile size and offset checks.
  }

  return m_emuRAMAddressStart != 0;
}

bool LinuxDolphinProcess::findPID()
{
  DIR* directoryPointer = opendir("/proc/");
  if (directoryPointer == nullptr)
    return false;

  static const char* const s_dolphinProcessName{std::getenv("DME_DOLPHIN_PROCESS_NAME")};

  m_PID = -1;
  struct dirent* directoryEntry = nullptr;
  while (m_PID == -1 && (directoryEntry = readdir(directoryPointer)))
  {
    const char* const name{static_cast<const char*>(directoryEntry->d_name)};
    std::istringstream conversionStream(name);
    int aPID = 0;
    if (!(conversionStream >> aPID))
      continue;
    std::ifstream aCmdLineFile;
    std::string line;
    aCmdLineFile.open("/proc/" + std::string(name) + "/comm");
    getline(aCmdLineFile, line);

    const bool match{s_dolphinProcessName ?
                         line == s_dolphinProcessName :
                         (line == "dolphin-emu" || line == "dolphin-emu-qt2" ||
                          line == "dolphin-emu-wx" || line == ".dolphin-emu-wr")};
    if (match)
      m_PID = aPID;

    aCmdLineFile.close();
  }
  closedir(directoryPointer);

  const bool running{m_PID != -1};
  return running;
}

bool LinuxDolphinProcess::readFromRAM(const u32 offset, char* buffer, const size_t size,
                                      const bool withBSwap)
{
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

  if (!::readFromRAM(m_PID, RAMAddress, 0, buffer, size))
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
    default:
      break;
    }
  }

  return true;
}

bool LinuxDolphinProcess::writeToRAM(const u32 offset, const char* buffer, const size_t size,
                                     const bool withBSwap)
{
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

  iovec local{};
  local.iov_base = bufferCopy;
  local.iov_len = size;

  iovec remote{};
  remote.iov_base = reinterpret_cast<void*>(RAMAddress);
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
    default:
      break;
    }
  }

  const ssize_t nwrote{process_vm_writev(m_PID, &local, 1, &remote, 1, 0)};
  delete[] bufferCopy;

  if (nwrote == -1)
  {
    // A more specific error type should be available in `errno` (if ever interested).
    return false;
  }

  return static_cast<size_t>(nwrote) == size;
}
}  // namespace DolphinComm
#endif
