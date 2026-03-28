#ifdef __APPLE__

#include "MacDolphinProcess.h"
#include "../../Common/CommonUtils.h"
#include "../../Common/MemoryCommon.h"

#include <cstdlib>
#include <cstring>
#include <mach/mach_vm.h>
#include <memory>
#include <string_view>
#include <sys/sysctl.h>

namespace DolphinComm
{
bool MacDolphinProcess::findPID()
{
  static const int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

  size_t procSize = 0;
  if (sysctl((int*)mib, 4, NULL, &procSize, NULL, 0) == -1)
    return false;

  auto procs = std::make_unique<kinfo_proc[]>(procSize / sizeof(kinfo_proc));
  if (sysctl((int*)mib, 4, procs.get(), &procSize, NULL, 0) == -1)
    return false;

  static const char* const s_dolphinProcessName{std::getenv("DME_DOLPHIN_PROCESS_NAME")};

  m_PID = -1;
  for (size_t i{0}; i < procSize / sizeof(kinfo_proc); ++i)
  {
    const std::string_view name{procs[i].kp_proc.p_comm};
    const bool match{s_dolphinProcessName ? name == s_dolphinProcessName :
                                            (name == "Dolphin" || name == "dolphin-emu")};
    if (match)
    {
      m_PID = procs[i].kp_proc.p_pid;
    }
  }

  const bool running{m_PID != -1};
  return running;
}

bool MacDolphinProcess::obtainEmuRAMInformations()
{
  m_currentTask = current_task();
  kern_return_t error = task_for_pid(m_currentTask, m_PID, &m_task);
  if (error != KERN_SUCCESS)
    return false;

  struct MemoryMapLine
  {
    const mach_vm_address_t regionAddr;
    const mach_vm_size_t size;
    const vm_region_extended_info_data_t regInfo;
    const vm_region_basic_info_data_64_t basInfo;
  };

  // Parse memory map of the Dolphin process.
  std::vector<MemoryMapLine> memoryMapLines;
  {
    mach_vm_address_t regionAddr = 0;
    mach_vm_size_t size = 0;
    vm_region_extended_info_data_t regInfo;
    vm_region_basic_info_data_64_t basInfo;
    vm_region_top_info_data_t topInfo;
    mach_msg_type_number_t cnt = VM_REGION_EXTENDED_INFO_COUNT;
    mach_port_t obj;

    while (mach_vm_region(m_task, &regionAddr, &size, VM_REGION_EXTENDED_INFO, (int*)&regInfo, &cnt,
                          &obj) == KERN_SUCCESS)
    {
      cnt = VM_REGION_BASIC_INFO_COUNT_64;
      if (mach_vm_region(m_task, &regionAddr, &size, VM_REGION_BASIC_INFO_64, (int*)&basInfo, &cnt,
                         &obj) != KERN_SUCCESS)
        break;
      cnt = VM_REGION_TOP_INFO_COUNT;
      if (mach_vm_region(m_task, &regionAddr, &size, VM_REGION_TOP_INFO, (int*)&topInfo, &cnt,
                         &obj) != 0)
        break;

      memoryMapLines.push_back({regionAddr, size, regInfo, basInfo});

      regionAddr += size;
      cnt = VM_REGION_EXTENDED_INFO_COUNT;
    }
  }

  SystemInfo systemInfo{};
  DolphinOSGlobals dolphinOSGlobals{};
  WiiSpecificInfo wiiSpecificInfo{};

  // In a first pass, attempt to parse data structures from the top of the memory region.
  for (const auto& [regionAddr, size, regInfo, basInfo] : memoryMapLines)
  {
    if (basInfo.offset)
      continue;

    if (!(regInfo.share_mode == SM_TRUESHARED &&
          basInfo.max_protection == (VM_PROT_READ | VM_PROT_WRITE)))
      continue;

    vm_size_t nread{};
    if (vm_read_overwrite(m_task, regionAddr + SYSTEM_INFO_OFFSET, sizeof(systemInfo),
                          reinterpret_cast<vm_address_t>(&systemInfo), &nread) != KERN_SUCCESS)
      continue;
    if (nread != sizeof(systemInfo))
      continue;
    if (vm_read_overwrite(m_task, regionAddr + DOLPHIN_OS_GLOBALS_OFFSET, sizeof(dolphinOSGlobals),
                          reinterpret_cast<vm_address_t>(&dolphinOSGlobals),
                          &nread) != KERN_SUCCESS)
      continue;
    if (nread != sizeof(dolphinOSGlobals))
      continue;
    if (vm_read_overwrite(m_task, regionAddr + WII_SPECIFIC_INFO_OFFSET, sizeof(wiiSpecificInfo),
                          reinterpret_cast<vm_address_t>(&wiiSpecificInfo), &nread) != KERN_SUCCESS)
      continue;
    if (nread != sizeof(wiiSpecificInfo))
      continue;

    // GameCube or Triforce game.
    if (systemInfo.isDiscMagicWordGCKnown() && systemInfo.isBootCodeKnown())
    {
      m_emuRAMAddressStart = regionAddr;
      m_MEM1Size = dolphinOSGlobals.getSimulatedMemorySize();
      m_ARAMSize = dolphinOSGlobals.getARAMSize();
      break;
    }

    // Wii game.
    if (systemInfo.isDiscMagicWordWiiKnown() && systemInfo.isBootCodeKnown())
    {
      m_emuRAMAddressStart = regionAddr;
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
      m_emuRAMAddressStart = regionAddr;
      m_MEM1Size = wiiSpecificInfo.getSimulatedMEM1Size();
      m_MEM2Size = wiiSpecificInfo.getSimulatedMEM2Size();
      break;
    }
  }

  // Second loop to find either the ARAM address (GameCube or Triforce) or the MEM2 address (Wii).
  for (const auto& [regionAddr, size, regInfo, basInfo] : memoryMapLines)
  {
    (void)regInfo;

    if (m_ARAMSize && size == Common::NextPowerOf2(m_MEM1Size) &&
        (basInfo.offset & 0x00040000) == 0x00040000)
    {
      m_emuARAMAdressStart = regionAddr;
      m_ARAMAccessible = true;
      break;
    }

    if (m_MEM2Size && size == Common::NextPowerOf2(m_MEM2Size) &&
        (basInfo.offset & 0x00040000) == 0x00040000)
    {
      m_MEM2AddressStart = regionAddr;
      m_MEM2Present = true;
      break;
    }

    // TODO(CA): Ideally, we'd inspect the memory to try to determine whether it's pointing to the
    // expected data, as opposed to relying on these fragile size and offset checks.
  }

  return m_emuRAMAddressStart != 0;
}

bool MacDolphinProcess::readFromRAM(const u32 offset, char* buffer, size_t size,
                                    const bool withBSwap)
{
  vm_size_t nread;
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

  if (vm_read_overwrite(m_task, RAMAddress, size, reinterpret_cast<vm_address_t>(buffer), &nread) !=
      KERN_SUCCESS)
    return false;
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
    default:
      break;
    }
  }

  return true;
}

bool MacDolphinProcess::writeToRAM(const u32 offset, const char* buffer, const size_t size,
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

  if (vm_write(m_task, RAMAddress, reinterpret_cast<vm_offset_t>(bufferCopy),
               static_cast<mach_msg_type_number_t>(size)) != KERN_SUCCESS)
  {
    delete[] bufferCopy;
    return false;
  }

  delete[] bufferCopy;
  return true;
}
}  // namespace DolphinComm
#endif
