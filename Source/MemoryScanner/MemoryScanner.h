#pragma once

#include <cmath>
#include <cstring>
#include <stack>
#include <string>
#include <type_traits>
#include <vector>

#include "../Common/CommonTypes.h"
#include "../Common/CommonUtils.h"
#include "../Common/MemoryCommon.h"

class MemScanner
{
public:
  enum class ScanFiter
  {
    exact = 0,
    increasedBy,
    decreasedBy,
    between,
    biggerThan,
    smallerThan,
    increased,
    decreased,
    changed,
    unchanged,
    unknownInitial
  };

  enum class CompareResult
  {
    bigger,
    smaller,
    equal,
    nan
  };

  MemScanner();
  ~MemScanner();
  Common::MemOperationReturnCode firstScan(const ScanFiter filter, const std::string& searchTerm1,
                                           const std::string& searchTerm2);
  Common::MemOperationReturnCode nextScan(const ScanFiter filter, const std::string& searchTerm1,
                                          const std::string& searchTerm2);
  bool undoScan();
  void reset();
  inline CompareResult compareMemoryAsNumbers(const char* first, const char* second,
                                              const char* offset, bool offsetInvert,
                                              bool bswapSecond, size_t length) const;

  template <typename T>
  inline T convertMemoryToType(const char* memory, bool invert) const
  {
    T theType;
    std::memcpy(&theType, memory, sizeof(T));
    if (invert)
      theType *= -1;
    return theType;
  }

  template <typename T>
  inline CompareResult compareMemoryAsNumbersWithType(const char* first, const char* second,
                                                      const char* offset, bool offsetInvert,
                                                      bool bswapSecond) const
  {
    T firstByte;
    T secondByte;
    std::memcpy(&firstByte, first, sizeof(T));
    std::memcpy(&secondByte, second, sizeof(T));
    size_t size = sizeof(T);
    switch (size)
    {
    case 2:
    {
      u16 firstHalfword = 0;
      std::memcpy(&firstHalfword, &firstByte, sizeof(u16));
      firstHalfword = Common::bSwap16(firstHalfword);
      std::memcpy(&firstByte, &firstHalfword, sizeof(u16));
      if (bswapSecond)
      {
        std::memcpy(&firstHalfword, &secondByte, sizeof(u16));
        firstHalfword = Common::bSwap16(firstHalfword);
        std::memcpy(&secondByte, &firstHalfword, sizeof(u16));
      }
      break;
    }
    case 4:
    {
      u32 firstWord = 0;
      std::memcpy(&firstWord, &firstByte, sizeof(u32));
      firstWord = Common::bSwap32(firstWord);
      std::memcpy(&firstByte, &firstWord, sizeof(u32));
      if (bswapSecond)
      {
        std::memcpy(&firstWord, &secondByte, sizeof(u32));
        firstWord = Common::bSwap32(firstWord);
        std::memcpy(&secondByte, &firstWord, sizeof(u32));
      }
      break;
    }
    case 8:
    {
      u64 firstDoubleword = 0;
      std::memcpy(&firstDoubleword, &firstByte, sizeof(u64));
      firstDoubleword = Common::bSwap64(firstDoubleword);
      std::memcpy(&firstByte, &firstDoubleword, sizeof(u64));
      if (bswapSecond)
      {
        std::memcpy(&firstDoubleword, &secondByte, sizeof(u64));
        firstDoubleword = Common::bSwap64(firstDoubleword);
        std::memcpy(&secondByte, &firstDoubleword, sizeof(u64));
      }
      break;
    }
    }

    if constexpr (std::is_floating_point<T>::value)
      if (std::isnan(firstByte))
        return CompareResult::nan;

    if (firstByte < (secondByte + convertMemoryToType<T>(offset, offsetInvert)))
      return CompareResult::smaller;
    else if (firstByte > (secondByte + convertMemoryToType<T>(offset, offsetInvert)))
      return CompareResult::bigger;
    else
      return CompareResult::equal;
  }

  void setType(const Common::MemType type);
  void setBase(const Common::MemBase base);
  void setEnforceMemAlignment(const bool enforceAlignment);
  void setIsSigned(const bool isSigned);
  void resetSearchRange();
  bool setSearchRangeBegin(u32 beginIndex);
  bool setSearchRangeEnd(u32 endIndex);
  bool setSearchRange(u32 beginIndex, u32 endIndex);

  std::vector<u32> getResultsConsoleAddr() const;
  size_t getResultCount() const;
  bool hasUndo() const;
  size_t getUndoCount() const;
  int getTermsNumForFilter(const ScanFiter filter) const;
  Common::MemType getType() const;
  Common::MemBase getBase() const;
  size_t getLength() const;
  bool getIsUnsigned() const;
  std::string getFormattedScannedValueAt(const int index) const;
  std::string getFormattedCurrentValueAt(int index) const;
  void removeResultAt(int index);
  bool typeSupportsAdditionalOptions(const Common::MemType type) const;
  bool hasScanStarted() const;

private:
  inline bool isHitNextScan(const ScanFiter filter, const char* memoryToCompare1,
                            const char* memoryToCompare2, const char* noOffset,
                            const char* newerRAMCache, const size_t realSize,
                            const u32 consoleOffset) const;
  std::string addSpacesToBytesArrays(const std::string& bytesArray) const;

  bool m_searchInRangeBegin = false;
  bool m_searchInRangeEnd = false;
  u32 m_beginSearchRange = 0;
  u32 m_endSearchRange = 0;

  Common::MemType m_memType = Common::MemType::type_byte;
  Common::MemBase m_memBase = Common::MemBase::base_decimal;
  size_t m_memSize;
  bool m_enforceMemAlignment = true;
  bool m_memIsSigned = false;

  size_t m_resultCount = 0;
  size_t m_undoCount = 0;
  bool m_wasUnknownInitialValue = false;
  char* m_scanRAMCache = nullptr;
  bool m_scanStarted = false;

  struct MemScannerUndoAction
  {
    std::vector<u32> data;
    bool wasUnknownInitialState = false;
  };
  std::stack<MemScannerUndoAction> m_UndoStack;
  std::vector<u32> m_resultsConsoleAddr;
};
