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

inline u32 dolphinAddrToOffset(u32 addr)
{
  return addr &= 0x7FFFFFFF;
}

inline u32 offsetToDolphinAddr(u32 offset)
{
  return offset |= 0x80000000;
}
} // namespace Common
