#pragma once

#ifdef __linux__
#include <byteswap.h>
#elif _WIN32
#include <stdlib.h>
#endif

#include "CommonTypes.h"

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

inline u32 dolphinAddrToOffset(u32 addr, u32 mem2_offset)
{
  addr &= 0x7FFFFFFF;
  if (addr >= 0x10000000)
  {
    // MEM2, calculate correct address from MEM2 offset
    addr -= 0x10000000;
    addr += mem2_offset;
  }
  return addr;
}

inline u32 offsetToDolphinAddr(u32 offset, u32 mem2_offset)
{
  if (offset < 0 or offset >= 0x2000000)
  {
    // MEM2, calculate correct address from MEM2 offset
    offset += 0x10000000;
    offset -= mem2_offset;
  }
  return offset | 0x80000000;
}
} // namespace Common
