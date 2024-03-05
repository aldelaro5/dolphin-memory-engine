#pragma once

#ifdef __linux__
#include <byteswap.h>
#elif _WIN32
#include <stdlib.h>
#elif __APPLE__
#define bswap_16(value) \
((((value) & 0xff) << 8) | ((value) >> 8))

#define bswap_32(value) \
(((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
(uint32_t)bswap_16((uint16_t)((value) >> 16)))

#define bswap_64(value) \
(((uint64_t)bswap_32((uint32_t)((value) & 0xffffffff)) \
<< 32) | \
(uint64_t)bswap_32((uint32_t)((value) >> 32)))
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

#else
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

constexpr u32 NextPowerOf2(u32 value)
{
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  ++value;

  return value;
};

inline u32 dolphinAddrToOffset(u32 addr, bool considerAram)
{
  // ARAM address
  if (addr >= ARAM_START && addr < ARAM_END)
  {
    addr -= ARAM_START;
  }
  // MEM1 address
  else if (addr >= MEM1_START && addr < GetMEM1End())
  {
    addr -= MEM1_START;
    if (considerAram)
      addr += ARAM_FAKESIZE;
  }
  // MEM2 address
  else if (addr >= MEM2_START && addr < GetMEM2End())
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
    else if (offset >= ARAM_FAKESIZE && offset < ARAM_FAKESIZE + GetMEM1SizeReal())
    {
      offset += MEM1_START;
      offset -= ARAM_FAKESIZE;
    }
  }
  else
  {
    if (offset < GetMEM1SizeReal())
    {
      offset += MEM1_START;
    }
    else if (offset >= MEM2_START - MEM1_START &&
             offset < MEM2_START - MEM1_START + GetMEM2SizeReal())
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
    if (offset >= ARAM_FAKESIZE && offset < GetMEM1SizeReal() + ARAM_FAKESIZE)
    {
      offset -= (ARAM_FAKESIZE - ARAM_SIZE);
    }
  }
  else
  {
    if (offset >= MEM2_START - MEM1_START && offset < MEM2_START - MEM1_START + GetMEM2SizeReal())
    {
      offset -= (MEM2_START - GetMEM1End());
    }
  }
  return offset;
}

inline u32 cacheIndexToOffset(u32 cacheIndex, bool considerAram)
{
  if (considerAram)
  {
    if (cacheIndex >= ARAM_SIZE && cacheIndex < GetMEM1SizeReal() + ARAM_SIZE)
    {
      cacheIndex += (ARAM_FAKESIZE - ARAM_SIZE);
    }
  }
  else
  {
    if (cacheIndex >= GetMEM1SizeReal() && cacheIndex < GetMEM1SizeReal() + GetMEM2SizeReal())
    {
      cacheIndex += (MEM2_START - GetMEM1End());
    }
  }
  return cacheIndex;
}
}  // namespace Common
