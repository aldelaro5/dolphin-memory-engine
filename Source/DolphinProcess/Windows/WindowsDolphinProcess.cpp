#ifdef _WIN32

#include "WindowsDolphinProcess.h"
#include "../../Common/CommonUtils.h"
#include "../../Common/MemoryCommon.h"

#include <Psapi.h>
#ifdef UNICODE
#include <codecvt>
#endif
#include <cstdlib>
#include <string>
#include <tlhelp32.h>

namespace
{
#ifdef UNICODE
std::wstring utf8_to_wstring(const std::string& str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.from_bytes(str);
}
#endif
}  // namespace

namespace DolphinComm
{

bool WindowsDolphinProcess::findPID()
{
  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  static const char* const s_dolphinProcessName{std::getenv("DME_DOLPHIN_PROCESS_NAME")};

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

  m_PID = -1;
  if (Process32First(snapshot, &entry) == TRUE)
  {
    do
    {
#ifdef UNICODE
      const std::wstring exeFile{entry.szExeFile};
      const bool match{s_dolphinProcessName ?
                           (exeFile == utf8_to_wstring(s_dolphinProcessName) ||
                            exeFile == utf8_to_wstring(s_dolphinProcessName) + L".exe") :
                           (exeFile == L"Dolphin.exe" || exeFile == L"DolphinQt2.exe" ||
                            exeFile == L"DolphinWx.exe")};
#else
      const std::string exeFile{entry.szExeFile};
      const bool match{s_dolphinProcessName ?
                           (exeFile == s_dolphinProcessName ||
                            exeFile == std::string(s_dolphinProcessName) + ".exe") :
                           (exeFile == "Dolphin.exe" || exeFile == "DolphinQt2.exe" ||
                            exeFile == "DolphinWx.exe")};
#endif
      if (match)
      {
        m_PID = entry.th32ProcessID;
        break;
      }
    } while (Process32Next(snapshot, &entry) == TRUE);
  }

  CloseHandle(snapshot);
  if (m_PID == -1)
    // Here, Dolphin doesn't appear to be running on the system
    return false;

  // Get the handle if Dolphin is running since it's required on Windows to read or write into the
  // RAM of the process and to query the RAM mapping information
  m_hDolphin = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ |
                               PROCESS_VM_WRITE,
                           FALSE, m_PID);
  return true;
}

bool WindowsDolphinProcess::obtainEmuRAMInformations()
{
  struct MemoryMapLine
  {
    const u64 baseAddress;
    const u64 size;
  };

  // In a first pass, attempt to parse data structures from the top of the memory region.
  std::vector<MemoryMapLine> memoryMapLines;
  {
    MEMORY_BASIC_INFORMATION info{};

    for (unsigned char* p{nullptr};
         VirtualQueryEx(m_hDolphin, p, &info, sizeof(info)) == sizeof(info); p += info.RegionSize)
    {
      PSAPI_WORKING_SET_EX_INFORMATION wsInfo{};
      wsInfo.VirtualAddress = info.BaseAddress;
      if (!QueryWorkingSetEx(m_hDolphin, &wsInfo, sizeof(PSAPI_WORKING_SET_EX_INFORMATION)) ||
          !wsInfo.VirtualAttributes.Valid)
        continue;

      memoryMapLines.push_back({Common::bit_cast<u64>(info.BaseAddress), info.RegionSize});
    }
  }

  SystemInfo systemInfo{};
  DolphinOSGlobals dolphinOSGlobals{};
  WiiSpecificInfo wiiSpecificInfo{};

  // In a first pass, attempt to parse data structures from the top of the memory region.
  for (const auto& [baseAddress, size] : memoryMapLines)
  {
    SIZE_T nread{};
    if (!ReadProcessMemory(m_hDolphin, reinterpret_cast<void*>(baseAddress + SYSTEM_INFO_OFFSET),
                           reinterpret_cast<char*>(&systemInfo), sizeof(systemInfo), &nread))
      continue;
    if (nread != sizeof(systemInfo))
      continue;
    if (!ReadProcessMemory(
            m_hDolphin, reinterpret_cast<void*>(baseAddress + DOLPHIN_OS_GLOBALS_OFFSET),
            reinterpret_cast<char*>(&dolphinOSGlobals), sizeof(dolphinOSGlobals), &nread))
      continue;
    if (nread != sizeof(dolphinOSGlobals))
      continue;
    if (!ReadProcessMemory(
            m_hDolphin, reinterpret_cast<void*>(baseAddress + WII_SPECIFIC_INFO_OFFSET),
            reinterpret_cast<char*>(&wiiSpecificInfo), sizeof(wiiSpecificInfo), &nread))
      continue;
    if (nread != sizeof(wiiSpecificInfo))
      continue;

    // GameCube or Triforce game.
    if (systemInfo.isDiscMagicWordGCKnown() && systemInfo.isBootCodeKnown())
    {
      m_emuRAMAddressStart = baseAddress;
      m_MEM1Size = dolphinOSGlobals.getSimulatedMemorySize();
      m_ARAMSize = dolphinOSGlobals.getARAMSize();
      break;
    }

    // Wii game.
    if (systemInfo.isDiscMagicWordWiiKnown() && systemInfo.isBootCodeKnown())
    {
      m_emuRAMAddressStart = baseAddress;
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
      m_emuRAMAddressStart = baseAddress;
      m_MEM1Size = wiiSpecificInfo.getSimulatedMEM1Size();
      m_MEM2Size = wiiSpecificInfo.getSimulatedMEM2Size();
      break;
    }
  }

  // Second loop to find either the ARAM address (GameCube or Triforce) or the MEM2 address (Wii).
  for (const auto& [baseAddress, size] : memoryMapLines)
  {
    if (m_ARAMSize && size == Common::NextPowerOf2(m_MEM1Size) &&
        baseAddress == m_emuRAMAddressStart + Common::NextPowerOf2(m_MEM1Size))
    {
      m_emuARAMAdressStart = baseAddress;
      m_ARAMAccessible = true;
      break;
    }

    if (m_MEM2Size && size == Common::NextPowerOf2(m_MEM2Size))
    {
      // In some cases, MEM2 can actually be before MEM1. Once MEM1 is found, regions of the
      // expected size that are too far away are ignored: there apparently are other non-MEM2
      // regions of 64 MiB.
      if (baseAddress >= m_emuRAMAddressStart + 0x10000000)
        continue;

      m_MEM2AddressStart = baseAddress;
      m_MEM2Present = true;
      break;
    }

    // TODO(CA): Ideally, we'd inspect the memory to try to determine whether it's pointing to the
    // expected data, as opposed to relying on these fragile size and offset checks.
  }

  return m_emuRAMAddressStart != 0;
}

bool WindowsDolphinProcess::readFromRAM(const u32 offset, char* buffer, const size_t size,
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

  SIZE_T nread = 0;
  const bool bResult{static_cast<bool>(
      ReadProcessMemory(m_hDolphin, reinterpret_cast<void*>(RAMAddress), buffer, size, &nread))};
  if (bResult && nread == size)
  {
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
  return false;
}

bool WindowsDolphinProcess::writeToRAM(const u32 offset, const char* buffer, const size_t size,
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

  SIZE_T nread = 0;
  char* bufferCopy = new char[size];
  std::memcpy(bufferCopy, buffer, size);
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

  const bool bResult{static_cast<bool>(WriteProcessMemory(
      m_hDolphin, reinterpret_cast<void*>(RAMAddress), bufferCopy, size, &nread))};
  delete[] bufferCopy;
  return (bResult && nread == size);
}
}  // namespace DolphinComm
#endif
