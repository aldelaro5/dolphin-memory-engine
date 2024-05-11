#ifdef _WIN32

#include "WindowsDolphinProcess.h"
#include "../../Common/CommonUtils.h"
#include "../../Common/MemoryCommon.h"

#include <Psapi.h>
#ifdef UNICODE
#include <codecvt>
#endif
#include <cassert>
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
  MEMORY_BASIC_INFORMATION info;
  bool MEM1Found = false;
  for (unsigned char* p = nullptr;
       VirtualQueryEx(m_hDolphin, p, &info, sizeof(info)) == sizeof(info); p += info.RegionSize)
  {
    // Check region size so that we know it's MEM2
    if (!m_MEM2Present && info.RegionSize == Common::GetMEM2Size())
    {
      u64 regionBaseAddress = 0;
      std::memcpy(&regionBaseAddress, &(info.BaseAddress), sizeof(info.BaseAddress));
      if (MEM1Found && regionBaseAddress > m_emuRAMAddressStart + 0x10000000)
      {
        // In some cases MEM2 could actually be before MEM1. Once we find MEM1, ignore regions of
        // this size that are too far away. There apparently are other non-MEM2 regions of 64 MiB.
        break;
      }
      // View the comment for MEM1.
      PSAPI_WORKING_SET_EX_INFORMATION wsInfo;
      wsInfo.VirtualAddress = info.BaseAddress;
      if (QueryWorkingSetEx(m_hDolphin, &wsInfo, sizeof(PSAPI_WORKING_SET_EX_INFORMATION)))
      {
        if (wsInfo.VirtualAttributes.Valid)
        {
          std::memcpy(&m_MEM2AddressStart, &(regionBaseAddress), sizeof(regionBaseAddress));
          m_MEM2Present = true;
        }
      }
    }
    else if (info.RegionSize == Common::GetMEM1Size() && info.Type == MEM_MAPPED)
    {
      // Here, it's likely the right page, but it can happen that multiple pages with these criteria
      // exists and have nothing to do with the emulated memory. Only the right page has valid
      // working set information so an additional check is required that it is backed by physical
      // memory.
      PSAPI_WORKING_SET_EX_INFORMATION wsInfo;
      wsInfo.VirtualAddress = info.BaseAddress;
      if (QueryWorkingSetEx(m_hDolphin, &wsInfo, sizeof(PSAPI_WORKING_SET_EX_INFORMATION)))
      {
        if (wsInfo.VirtualAttributes.Valid)
        {
          if (!MEM1Found)
          {
            std::memcpy(&m_emuRAMAddressStart, &(info.BaseAddress), sizeof(info.BaseAddress));
            MEM1Found = true;
          }
          else
          {
            u64 aramCandidate = 0;
            std::memcpy(&aramCandidate, &(info.BaseAddress), sizeof(info.BaseAddress));
            if (aramCandidate == m_emuRAMAddressStart + Common::GetMEM1Size())
            {
              m_emuARAMAdressStart = aramCandidate;
              m_ARAMAccessible = true;
            }
          }
        }
      }
    }
  }

  if (m_MEM2Present)
  {
    m_emuARAMAdressStart = 0;
    m_ARAMAccessible = false;
  }

  if (m_emuRAMAddressStart == 0)
  {
    // Here, Dolphin is running, but the emulation hasn't started
    return false;
  }
  return true;
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
        assert(0 && "Unexpected type size");
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
      assert(0 && "Unexpected type size");
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
