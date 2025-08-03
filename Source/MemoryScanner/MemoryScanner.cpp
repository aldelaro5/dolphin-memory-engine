#include "MemoryScanner.h"

#include <string_view>

#include "../Common/CommonUtils.h"
#include "../DolphinProcess/DolphinAccessor.h"

namespace
{
std::string addSpacesToBytesArrays(const std::string_view bytesArray)
{
  std::string result(bytesArray);
  std::string::size_type spacesAdded = 0;
  for (std::string::size_type i{2}; i < bytesArray.length(); i += 2)
  {
    if (bytesArray[i] != ' ')
    {
      result.insert(i + spacesAdded, 1, ' ');
      spacesAdded++;
    }
    else
    {
      i++;
    }
  }
  return result;
}
}  // namespace

MemScanner::MemScanner() = default;

MemScanner::~MemScanner()
{
  delete[] m_scanRAMCache;
}

Common::MemOperationReturnCode MemScanner::firstScan(const MemScanner::ScanFilter filter,
                                                     const std::string& searchTerm1,
                                                     const std::string& searchTerm2)
{
  m_scanRAMCache = nullptr;
  u32 ramSize = static_cast<u32>(DolphinComm::DolphinAccessor::getRAMTotalSize());
  m_scanRAMCache = new char[ramSize];
  DolphinComm::DolphinAccessor::readEntireRAM(m_scanRAMCache);

  u32 beginA = m_searchInRangeBegin ? m_beginSearchRange : 0;
  u32 endA = m_searchInRangeEnd ? m_endSearchRange : ramSize;

  if (m_searchInRangeBegin || m_searchInRangeEnd)
  {
    ramSize = endA - beginA;
  }

  if (filter == ScanFilter::unknownInitial)
  {
    if (m_searchInRangeBegin || m_searchInRangeEnd)
    {
      int alignmentDivision =
          m_enforceMemAlignment ? Common::getNbrBytesAlignmentForType(m_memType) : 1;
      m_wasUnknownInitialValue = false;
      m_memSize = Common::getSizeForType(m_memType, static_cast<size_t>(1));
      m_scanStarted = true;

      bool aram = DolphinComm::DolphinAccessor::isARAMAccessible();

      u32 alignedBeginA =
          beginA + ((alignmentDivision - (beginA % alignmentDivision)) % alignmentDivision);

      for (u32 i = alignedBeginA; i < endA - m_memSize; i += alignmentDivision)
      {
        m_resultsConsoleAddr.push_back(Common::offsetToDolphinAddr(i, aram));
      }

      m_resultCount = m_resultsConsoleAddr.size();
    }
    else
    {
      int alignementDivision =
          m_enforceMemAlignment ? Common::getNbrBytesAlignmentForType(m_memType) : 1;
      m_resultCount = ((ramSize / alignementDivision) -
                       Common::getSizeForType(m_memType, static_cast<size_t>(1)));
      m_wasUnknownInitialValue = true;
      m_memSize = 1;
      m_scanStarted = true;
    }

    return Common::MemOperationReturnCode::OK;
  }

  m_wasUnknownInitialValue = false;
  Common::MemOperationReturnCode scanReturn = Common::MemOperationReturnCode::OK;
  size_t termActualLength = 0;
  size_t termMaxLength = 0;
  if (m_memType == Common::MemType::type_string)
    // This is just to have the string formatted with the appropriate length, byte arrays don't need
    // this because they get copied byte per byte
    termMaxLength = searchTerm1.length();
  else
    // Have no restriction on the length for the rest
    termMaxLength = ramSize;

  std::string formattedSearchTerm1;
  if (m_memType == Common::MemType::type_byteArray)
    formattedSearchTerm1 = addSpacesToBytesArrays(searchTerm1);
  else
    formattedSearchTerm1 = searchTerm1;

  char* memoryToCompare1 = Common::formatStringToMemory(
      scanReturn, termActualLength, formattedSearchTerm1, m_memBase, m_memType, termMaxLength);
  if (scanReturn != Common::MemOperationReturnCode::OK)
  {
    delete[] memoryToCompare1;
    delete[] m_scanRAMCache;
    m_scanRAMCache = nullptr;
    return scanReturn;
  }

  char* memoryToCompare2 = nullptr;
  if (filter == ScanFilter::between)
  {
    memoryToCompare2 = Common::formatStringToMemory(scanReturn, termActualLength, searchTerm2,
                                                    m_memBase, m_memType, ramSize);
    if (scanReturn != Common::MemOperationReturnCode::OK)
    {
      delete[] memoryToCompare1;
      delete[] memoryToCompare2;
      delete[] m_scanRAMCache;
      m_scanRAMCache = nullptr;
      return scanReturn;
    }
  }

  m_memSize = Common::getSizeForType(m_memType, termActualLength);

  char* noOffset = new char[m_memSize];
  std::memset(noOffset, 0, m_memSize);

  int increment = m_enforceMemAlignment ? Common::getNbrBytesAlignmentForType(m_memType) : 1;

  u32 beginSearch = beginA;
  u32 endSearch = endA - static_cast<u32>(m_memSize);

  for (u32 i = beginSearch; i < endSearch; i += increment)
  {
    char* memoryCandidate = &m_scanRAMCache[i];
    bool isResult = false;
    switch (filter)
    {
    case ScanFilter::exact:
    {
      if (m_memType == Common::MemType::type_string || m_memType == Common::MemType::type_byteArray)
        isResult = (std::memcmp(memoryCandidate, memoryToCompare1, m_memSize) == 0);
      else
        isResult = (compareMemoryAsNumbers(memoryCandidate, memoryToCompare1, noOffset, false,
                                           false, m_memSize) == MemScanner::CompareResult::equal);
      break;
    }
    case ScanFilter::between:
    {
      MemScanner::CompareResult result1 = compareMemoryAsNumbers(memoryCandidate, memoryToCompare1,
                                                                 noOffset, false, false, m_memSize);
      MemScanner::CompareResult result2 = compareMemoryAsNumbers(memoryCandidate, memoryToCompare2,
                                                                 noOffset, false, false, m_memSize);
      isResult = ((result1 == MemScanner::CompareResult::bigger ||
                   result1 == MemScanner::CompareResult::equal) &&
                  (result2 == MemScanner::CompareResult::smaller ||
                   result2 == MemScanner::CompareResult::equal));
      break;
    }
    case ScanFilter::biggerThan:
    {
      isResult = (compareMemoryAsNumbers(memoryCandidate, memoryToCompare1, noOffset, false, false,
                                         m_memSize) == MemScanner::CompareResult::bigger);
      break;
    }
    case ScanFilter::smallerThan:
    {
      isResult = (compareMemoryAsNumbers(memoryCandidate, memoryToCompare1, noOffset, false, false,
                                         m_memSize) == MemScanner::CompareResult::smaller);
      break;
    }
    default:
      break;
    }

    if (isResult)
    {
      bool aramAccessible = DolphinComm::DolphinAccessor::isARAMAccessible();
      u32 consoleOffset = Common::cacheIndexToOffset(i, aramAccessible);
      m_resultsConsoleAddr.push_back(Common::offsetToDolphinAddr(consoleOffset, aramAccessible));
    }
  }
  delete[] noOffset;
  delete[] memoryToCompare1;
  delete[] memoryToCompare2;
  m_resultCount = m_resultsConsoleAddr.size();
  m_scanStarted = true;
  return Common::MemOperationReturnCode::OK;
}

Common::MemOperationReturnCode MemScanner::nextScan(const MemScanner::ScanFilter filter,
                                                    const std::string& searchTerm1,
                                                    const std::string& searchTerm2)
{
  char* newerRAMCache = nullptr;
  u32 ramSize = static_cast<u32>(DolphinComm::DolphinAccessor::getRAMTotalSize());
  newerRAMCache = new char[ramSize];
  DolphinComm::DolphinAccessor::readEntireRAM(newerRAMCache);

  Common::MemOperationReturnCode scanReturn = Common::MemOperationReturnCode::OK;
  size_t termActualLength = 0;
  size_t termMaxLength = 0;
  if (m_memType == Common::MemType::type_string)
    // This is just to have the string formatted with the appropriate length, byte arrays don't need
    // this because they get copied byte per byte
    termMaxLength = searchTerm1.length();
  else
    // Have no restriction on the length for the rest
    termMaxLength = ramSize;

  char* memoryToCompare1 = nullptr;
  if (filter != ScanFilter::increased && filter != ScanFilter::decreased &&
      filter != ScanFilter::changed && filter != ScanFilter::unchanged)
  {
    std::string formattedSearchTerm1;
    if (m_memType == Common::MemType::type_byteArray)
      formattedSearchTerm1 = addSpacesToBytesArrays(searchTerm1);
    else
      formattedSearchTerm1 = searchTerm1;

    memoryToCompare1 = Common::formatStringToMemory(
        scanReturn, termActualLength, formattedSearchTerm1, m_memBase, m_memType, termMaxLength);
    if (scanReturn != Common::MemOperationReturnCode::OK)
      return scanReturn;
  }

  char* memoryToCompare2 = nullptr;
  if (filter == ScanFilter::between)
  {
    memoryToCompare2 = Common::formatStringToMemory(scanReturn, termActualLength, searchTerm2,
                                                    m_memBase, m_memType, ramSize);
    if (scanReturn != Common::MemOperationReturnCode::OK)
      return scanReturn;
  }

  m_memSize = Common::getSizeForType(m_memType, termActualLength);

  char* noOffset = new char[m_memSize];
  std::memset(noOffset, 0, m_memSize);

  std::vector<u32> newerResults = std::vector<u32>();
  bool aramAccessible = DolphinComm::DolphinAccessor::isARAMAccessible();

  bool wasUninitialized = m_wasUnknownInitialValue;

  if (m_wasUnknownInitialValue)
  {
    m_wasUnknownInitialValue = false;

    int increment = m_enforceMemAlignment ? Common::getNbrBytesAlignmentForType(m_memType) : 1;
    for (u32 i = 0; i < (ramSize - m_memSize); i += increment)
    {
      if (isHitNextScan(filter, memoryToCompare1, memoryToCompare2, noOffset, newerRAMCache,
                        m_memSize, i))
      {
        u32 offset = Common::cacheIndexToOffset(i, aramAccessible);
        newerResults.push_back(Common::offsetToDolphinAddr(offset, aramAccessible));
      }
    }
  }
  else
  {
    for (auto i : m_resultsConsoleAddr)
    {
      u32 offset = Common::dolphinAddrToOffset(i, aramAccessible);
      u32 ramIndex = Common::offsetToCacheIndex(offset, aramAccessible);
      if (isHitNextScan(filter, memoryToCompare1, memoryToCompare2, noOffset, newerRAMCache,
                        m_memSize, ramIndex))
      {
        newerResults.push_back(i);
      }
    }
  }

  delete[] noOffset;

  m_UndoStack.push({m_resultsConsoleAddr, wasUninitialized});
  m_undoCount = m_UndoStack.size();

  m_resultsConsoleAddr.clear();
  std::swap(m_resultsConsoleAddr, newerResults);

  delete[] m_scanRAMCache;
  m_scanRAMCache = nullptr;
  m_scanRAMCache = newerRAMCache;
  m_resultCount = m_resultsConsoleAddr.size();
  return Common::MemOperationReturnCode::OK;
}

void MemScanner::reset()
{
  m_resultsConsoleAddr.clear();
  m_wasUnknownInitialValue = false;
  delete[] m_scanRAMCache;
  m_scanRAMCache = nullptr;
  m_resultCount = 0;
  m_scanStarted = false;
  while (!m_UndoStack.empty())
  {
    m_UndoStack.pop();
  }
  m_undoCount = 0;
}

inline bool MemScanner::isHitNextScan(const MemScanner::ScanFilter filter,
                                      const char* memoryToCompare1, const char* memoryToCompare2,
                                      const char* noOffset, const char* newerRAMCache,
                                      const size_t realSize, const u32 consoleOffset) const
{
  char* olderMemory = &m_scanRAMCache[consoleOffset];
  const char* newerMemory = &newerRAMCache[consoleOffset];

  switch (filter)
  {
  case ScanFilter::exact:
  {
    if (m_memType == Common::MemType::type_string || m_memType == Common::MemType::type_byteArray)
      return (std::memcmp(newerMemory, memoryToCompare1, realSize) == 0);

    return (compareMemoryAsNumbers(newerMemory, memoryToCompare1, noOffset, false, false,
                                   realSize) == MemScanner::CompareResult::equal);
  }
  case ScanFilter::between:
  {
    MemScanner::CompareResult result1 =
        compareMemoryAsNumbers(newerMemory, memoryToCompare1, noOffset, false, false, realSize);
    MemScanner::CompareResult result2 =
        compareMemoryAsNumbers(newerMemory, memoryToCompare2, noOffset, false, false, realSize);
    return ((result1 == MemScanner::CompareResult::bigger ||
             result1 == MemScanner::CompareResult::equal) &&
            (result2 == MemScanner::CompareResult::smaller ||
             result2 == MemScanner::CompareResult::equal));
  }
  case ScanFilter::biggerThan:
  {
    return (compareMemoryAsNumbers(newerMemory, memoryToCompare1, noOffset, false, false,
                                   realSize) == MemScanner::CompareResult::bigger);
  }
  case ScanFilter::smallerThan:
  {
    return (compareMemoryAsNumbers(newerMemory, memoryToCompare1, noOffset, false, false,
                                   realSize) == MemScanner::CompareResult::smaller);
  }
  case ScanFilter::increasedBy:
  {
    return (compareMemoryAsNumbers(newerMemory, olderMemory, memoryToCompare1, false, true,
                                   realSize) == MemScanner::CompareResult::equal);
  }
  case ScanFilter::decreasedBy:
  {
    return (compareMemoryAsNumbers(newerMemory, olderMemory, memoryToCompare1, true, true,
                                   realSize) == MemScanner::CompareResult::equal);
  }
  case ScanFilter::increased:
  {
    return (compareMemoryAsNumbers(newerMemory, olderMemory, noOffset, false, true, realSize) ==
            MemScanner::CompareResult::bigger);
  }
  case ScanFilter::decreased:
  {
    return (compareMemoryAsNumbers(newerMemory, olderMemory, noOffset, false, true, realSize) ==
            MemScanner::CompareResult::smaller);
  }
  case ScanFilter::changed:
  {
    MemScanner::CompareResult result =
        compareMemoryAsNumbers(newerMemory, olderMemory, noOffset, false, true, realSize);
    return (result == MemScanner::CompareResult::bigger ||
            result == MemScanner::CompareResult::smaller);
  }
  case ScanFilter::unchanged:
  {
    return (compareMemoryAsNumbers(newerMemory, olderMemory, noOffset, false, true, realSize) ==
            MemScanner::CompareResult::equal);
  }
  default:
  {
    return false;
  }
  }
}

inline MemScanner::CompareResult
MemScanner::compareMemoryAsNumbers(const char* first, const char* second, const char* offset,
                                   bool offsetInvert, bool bswapSecond, size_t length) const
{
  (void)length;

  switch (m_memType)
  {
  case Common::MemType::type_byte:
    if (m_memIsSigned)
      return compareMemoryAsNumbersWithType<s8>(first, second, offset, offsetInvert, bswapSecond);
    return compareMemoryAsNumbersWithType<u8>(first, second, offset, offsetInvert, bswapSecond);
  case Common::MemType::type_halfword:
    if (m_memIsSigned)
      return compareMemoryAsNumbersWithType<s16>(first, second, offset, offsetInvert, bswapSecond);
    return compareMemoryAsNumbersWithType<u16>(first, second, offset, offsetInvert, bswapSecond);
  case Common::MemType::type_word:
    if (m_memIsSigned)
      return compareMemoryAsNumbersWithType<s32>(first, second, offset, offsetInvert, bswapSecond);
    return compareMemoryAsNumbersWithType<u32>(first, second, offset, offsetInvert, bswapSecond);
  case Common::MemType::type_float:
    return compareMemoryAsNumbersWithType<float>(first, second, offset, offsetInvert, bswapSecond);
  case Common::MemType::type_double:
    return compareMemoryAsNumbersWithType<double>(first, second, offset, offsetInvert, bswapSecond);
  default:
    return MemScanner::CompareResult::nan;
  }
}

void MemScanner::setType(const Common::MemType type)
{
  m_memType = type;
}

void MemScanner::setBase(const Common::MemBase base)
{
  m_memBase = base;
}

void MemScanner::setEnforceMemAlignment(const bool enforceAlignment)
{
  m_enforceMemAlignment = enforceAlignment;
}

void MemScanner::setIsSigned(const bool isSigned)
{
  m_memIsSigned = isSigned;
}

void MemScanner::resetSearchRange()
{
  m_searchInRangeBegin = false;
  m_searchInRangeEnd = false;
  m_beginSearchRange = 0;
  m_endSearchRange = 0;
}

bool MemScanner::setSearchRange(u32 beginRange, u32 endRange)
{
  if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(beginRange) ||
      !DolphinComm::DolphinAccessor::isValidConsoleAddress(endRange))
  {
    return false;
  }

  m_searchInRangeBegin = true;
  m_searchInRangeEnd = true;
  bool aram = DolphinComm::DolphinAccessor::isARAMAccessible();
  m_beginSearchRange =
      Common::offsetToCacheIndex(Common::dolphinAddrToOffset(beginRange, aram), aram);
  m_endSearchRange = Common::offsetToCacheIndex(Common::dolphinAddrToOffset(endRange, aram), aram);

  return true;
}

bool MemScanner::setSearchRangeBegin(u32 beginRange)
{
  if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(beginRange))
  {
    return false;
  }

  m_searchInRangeBegin = true;
  bool aram = DolphinComm::DolphinAccessor::isARAMAccessible();
  m_beginSearchRange =
      Common::offsetToCacheIndex(Common::dolphinAddrToOffset(beginRange, aram), aram);

  return true;
}

bool MemScanner::setSearchRangeEnd(u32 endRange)
{
  if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(endRange))
  {
    return false;
  }

  m_searchInRangeEnd = true;
  bool aram = DolphinComm::DolphinAccessor::isARAMAccessible();
  m_endSearchRange =
      Common::offsetToCacheIndex(Common::dolphinAddrToOffset(endRange, aram), aram) + 1;

  return true;
}

int MemScanner::getTermsNumForFilter(const MemScanner::ScanFilter filter)
{
  if (filter == MemScanner::ScanFilter::between)
    return 2;
  if (filter == MemScanner::ScanFilter::exact || filter == MemScanner::ScanFilter::increasedBy ||
      filter == MemScanner::ScanFilter::decreasedBy ||
      filter == MemScanner::ScanFilter::biggerThan || filter == MemScanner::ScanFilter::smallerThan)
    return 1;
  return 0;
}

bool MemScanner::typeSupportsAdditionalOptions(const Common::MemType type)
{
  return (type == Common::MemType::type_byte || type == Common::MemType::type_halfword ||
          type == Common::MemType::type_word || type == Common::MemType::type_float ||
          type == Common::MemType::type_double);
}

std::vector<u32> MemScanner::getResultsConsoleAddr() const
{
  return m_resultsConsoleAddr;
}

std::string MemScanner::getFormattedScannedValueAt(const int index) const
{
  bool aramAccessible = DolphinComm::DolphinAccessor::isARAMAccessible();
  u32 offset = Common::dolphinAddrToOffset(m_resultsConsoleAddr.at(index), aramAccessible);
  u32 ramIndex = Common::offsetToCacheIndex(offset, aramAccessible);
  const size_t length{m_memType == Common::MemType::type_string ? ~0ULL : m_memSize};
  return Common::formatMemoryToString(&m_scanRAMCache[ramIndex], m_memType, length, m_memBase,
                                      !m_memIsSigned, Common::shouldBeBSwappedForType(m_memType));
}

std::string MemScanner::getFormattedCurrentValueAt(const int index) const
{
  if (DolphinComm::DolphinAccessor::isValidConsoleAddress(m_resultsConsoleAddr.at(index)))
  {
    bool aramAccessible = DolphinComm::DolphinAccessor::isARAMAccessible();
    u32 offset = Common::dolphinAddrToOffset(m_resultsConsoleAddr.at(index), aramAccessible);
    return DolphinComm::DolphinAccessor::getFormattedValueFromMemory(offset, m_memType, m_memSize,
                                                                     m_memBase, !m_memIsSigned);
  }
  return "";
}

void MemScanner::removeResultAt(int index)
{
  m_resultsConsoleAddr.erase(m_resultsConsoleAddr.begin() + index);
  m_resultCount--;
}

bool MemScanner::undoScan()
{
  if (m_undoCount > 0)
  {
    m_resultsConsoleAddr = m_UndoStack.top().data;
    m_resultCount = m_resultsConsoleAddr.size();
    bool wasUninitialzed = m_UndoStack.top().wasUnknownInitialState;

    m_UndoStack.pop();
    m_undoCount = m_UndoStack.size();

    if (wasUninitialzed)
    {
      u32 ramSize = static_cast<u32>(DolphinComm::DolphinAccessor::getRAMTotalSize());
      int alignementDivision =
          m_enforceMemAlignment ? Common::getNbrBytesAlignmentForType(m_memType) : 1;
      m_resultCount = ((ramSize / alignementDivision) -
                       Common::getSizeForType(m_memType, static_cast<size_t>(1)));
      m_wasUnknownInitialValue = true;
      m_memSize = 1;
    }

    return true;
  }
  return false;
}

size_t MemScanner::getResultCount() const
{
  return m_resultCount;
}

bool MemScanner::hasUndo() const
{
  return m_undoCount > 0;
}

size_t MemScanner::getUndoCount() const
{
  return m_undoCount;
}

bool MemScanner::hasScanStarted() const
{
  return m_scanStarted;
}

Common::MemType MemScanner::getType() const
{
  return m_memType;
}

Common::MemBase MemScanner::getBase() const
{
  return m_memBase;
}

size_t MemScanner::getLength() const
{
  return m_memSize;
}

bool MemScanner::getIsUnsigned() const
{
  return !m_memIsSigned;
}
