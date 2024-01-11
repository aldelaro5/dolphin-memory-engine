#ifdef __APPLE__

#include "DarwinDolphinProcess.h"
#include "../../Common/CommonUtils.h"

#include <memory>
#include <sys/sysctl.h>
#include <mach/mach_vm.h>

namespace DolphinComm
{
bool DarwinDolphinProcess::findPID()
{
  static const int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  size_t procSize = 0;
  if(sysctl((int*) mib, 4, NULL, &procSize, NULL, 0) == -1)
    return false;

  auto procs = std::make_unique<kinfo_proc[]>(procSize / sizeof(kinfo_proc));
  if(sysctl((int*) mib, 4, procs.get(), &procSize, NULL, 0) == -1)
    return false;

  for(int i = 0; i < procSize / sizeof(kinfo_proc); i++)
  {
    if(std::strcmp(procs[i].kp_proc.p_comm, "Dolphin") == 0 ||
       std::strcmp(procs[i].kp_proc.p_comm, "dolphin-emu") == 0)
    {
      m_PID = procs[i].kp_proc.p_pid;
    }
  }

  if (m_PID == -1)
    return false;
  return true;
}

bool DarwinDolphinProcess::obtainEmuRAMInformations()
{
  m_currentTask = current_task();
  kern_return_t error = task_for_pid(m_currentTask, m_PID, &m_task);
  if(error != KERN_SUCCESS)
    return false;

  mach_vm_address_t regionAddr = 0;
  mach_vm_size_t size = 0;
  vm_region_extended_info_data_t regInfo;
  vm_region_basic_info_data_64_t basInfo;
  vm_region_top_info_data_t topInfo;
  mach_msg_type_number_t cnt = VM_REGION_EXTENDED_INFO_COUNT;
  mach_port_t obj;
  bool MEM1Found = false;
  unsigned int MEM1Obj = 0;
  while(mach_vm_region(m_task, &regionAddr, &size, VM_REGION_EXTENDED_INFO, (int*)&regInfo, &cnt, &obj) == KERN_SUCCESS)
  {
    cnt = VM_REGION_BASIC_INFO_COUNT_64;
    if(mach_vm_region(m_task, &regionAddr, &size, VM_REGION_BASIC_INFO_64, (int*)&basInfo, &cnt, &obj) != KERN_SUCCESS)
        break;
    cnt = VM_REGION_TOP_INFO_COUNT;
    if(mach_vm_region(m_task, &regionAddr, &size, VM_REGION_TOP_INFO, (int*)&topInfo, &cnt, &obj) != 0)
        break;

    if (MEM1Found && regionAddr == m_emuRAMAddressStart + 0x10000000)
    {
      m_MEM2Present = true;
      break;
    }
    
    // if these are true, then it is very likely the correct region, but we cannot guarantee
    if((!MEM1Found || (MEM1Found && MEM1Obj == topInfo.obj_id)) && size == 0x2000000 && regInfo.share_mode == SM_TRUESHARED &&
       basInfo.max_protection == (VM_PROT_READ | VM_PROT_WRITE))
    {
      m_emuRAMAddressStart = regionAddr;
      MEM1Found = true;
      MEM1Obj = topInfo.obj_id;
    }

    regionAddr += size;
    cnt = VM_REGION_EXTENDED_INFO_COUNT;
  }
  if (m_emuRAMAddressStart != 0)
    return true;

  // Here, Dolphin appears to be running, but the emulator isn't started
  return false;
}

bool DarwinDolphinProcess::readFromRAM(const u32 offset, char* buffer, size_t size, const bool withBSwap)
{
  vm_size_t nread;
  u64 RAMAddress = m_emuRAMAddressStart + offset;

  if(vm_read_overwrite(m_task, RAMAddress, size, reinterpret_cast<vm_address_t>(buffer), &nread) != KERN_SUCCESS)
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
    }
  }

  return true;
}

bool DarwinDolphinProcess::writeToRAM(const u32 offset, const char* buffer, const size_t size, const bool withBSwap)
{
  u64 RAMAddress = m_emuRAMAddressStart + offset;
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
    }
  }

  if(vm_write(m_task, RAMAddress, reinterpret_cast<vm_offset_t>(bufferCopy), size) != KERN_SUCCESS)
  {
    delete[] bufferCopy;
    return false;
  }

  delete[] bufferCopy;
  return true;
}
} // namespace DolphinComm
#endif