#pragma once

#ifdef __linux__
#include <byteswap.h>
#elif _WIN32
#include <stdlib.h>
#endif

#include "CommonTypes.h"
#include "MemoryCommon.h"

namespace Common
{
#ifdef _WIN32
inline u16 bSwap16(u16 data)
{
  return _byteswap_ushort(data);
}
inline u32 bSwap32(u32 data)
{
  return _byteswap_ulong(data);
}
inline u64 bSwap64(u64 data)
{
  return _byteswap_uint64(data);
}

#elif __linux__
inline u16 bSwap16(u16 data)
{
  return bswap_16(data);
}
inline u32 bSwap32(u32 data)
{
  return bswap_32(data);
}
inline u64 bSwap64(u64 data)
{
  return bswap_64(data);
}
#endif

inline u32 dolphinAddrToOffset(u32 addr, bool considerAram)
{
  // ARAM address
  if (addr >= ARAM_START && addr < ARAM_END)
  {
    addr -= ARAM_START;
  }
  // MEM1 address
  else if (addr >= MEM1_START && addr < MEM1_END)
  {
    addr -= MEM1_START;
    if (considerAram)
      addr += ARAM_FAKESIZE;
  }
  // MEM2 address
  else if (addr >= MEM2_START && addr < MEM2_END)
  {
    addr -= MEM2_START;
    addr += (MEM2_START - MEM1_START);
  }
  return addr;
}

inline u32 offsetToDolphinAddr(u32 offset, bool considerAram)
{
  if (considerAram)
  {
    if (offset < ARAM_SIZE)
    {
      offset += ARAM_START;
    }
    else if (offset >= ARAM_FAKESIZE && offset < ARAM_FAKESIZE + MEM1_SIZE)
    {
      offset += MEM1_START;
      offset -= ARAM_FAKESIZE;
    }
  }
  else
  {
    if (offset < MEM1_SIZE)
    {
      offset += MEM1_START;
    }
    else if (offset >= MEM2_START - MEM1_START && offset < MEM2_START - MEM1_START + MEM2_SIZE)
    {
      offset += MEM2_START;
      offset -= (MEM2_START - MEM1_START);
    }
  }
  return offset;
}

inline u32 offsetToCacheIndex(u32 offset, bool considerAram)
{
  if (considerAram)
  {
    if (offset >= ARAM_FAKESIZE && offset < MEM1_SIZE + ARAM_FAKESIZE)
    {
      offset -= (ARAM_FAKESIZE - ARAM_SIZE);
    }
  }
  else
  {
    if (offset >= MEM2_START - MEM1_START && offset < MEM2_START - MEM1_START + MEM2_SIZE)
    {
      offset -= (MEM2_START - MEM1_END);
    }
  }
  return offset;
}

inline u32 cacheIndexToOffset(u32 cacheIndex, bool considerAram)
{
  if (considerAram)
  {
    if (cacheIndex >= ARAM_SIZE && cacheIndex < MEM1_SIZE + ARAM_SIZE)
    {
      cacheIndex += (ARAM_FAKESIZE - ARAM_SIZE);
    }
  }
  else
  {
    if (cacheIndex >= MEM1_SIZE && cacheIndex < MEM1_SIZE + MEM2_SIZE)
    {
      cacheIndex += (MEM2_START - MEM1_END);
    }
  }
  return cacheIndex;
}
} // namespace Common
