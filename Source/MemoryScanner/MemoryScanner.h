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
  enum class ScanFilter
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

  MemScanner(const MemScanner&) = delete;
  MemScanner(MemScanner&&) = delete;
  MemScanner& operator=(const MemScanner&) = delete;
  MemScanner& operator=(MemScanner&&) = delete;

  Common::MemOperationReturnCode firstScan(ScanFilter filter, const std::string& searchTerm1,
                                           const std::string& searchTerm2);
  Common::MemOperationReturnCode nextScan(ScanFilter filter, const std::string& searchTerm1,
                                          const std::string& searchTerm2);
  bool undoScan();
  void reset();
  inline CompareResult compareMemoryAsNumbers(const char* first, const char* second,
                                              const char* offset, bool offsetInvert,
                                              bool bswapSecond, size_t length) const;

  template <typename T>
  T convertMemoryToType(const char* memory, bool invert) const
  {
    T theType;
    std::memcpy(&theType, memory, sizeof(T));
    if (invert)
      theType *= -1;
    return theType;
  }

  template <typename T>
  CompareResult compareMemoryAsNumbersWithType(const char* first, const char* second,
                                               const char* offset, bool offsetInvert,
                                               bool bswapSecond) const
  {
    T firstByte;
    T secondByte;
    std::memcpy(&firstByte, first, sizeof(T));
    std::memcpy(&secondByte, second, sizeof(T));

    constexpr size_t size{sizeof(T)};
    static_assert(size == 1 || size == 2 || size == 4 || size == 8, "Unexpected type size");

    if constexpr (size == 2)
    {
      u16 firstHalfword = Common::bit_cast<u16, T>(firstByte);
      firstHalfword = Common::bSwap16(firstHalfword);
      firstByte = Common::bit_cast<T, u16>(firstHalfword);
      if (bswapSecond)
      {
        firstHalfword = Common::bit_cast<u16, T>(secondByte);
        firstHalfword = Common::bSwap16(firstHalfword);
        secondByte = Common::bit_cast<T, u16>(firstHalfword);
      }
    }
    else if constexpr (size == 4)
    {
      u32 firstWord = Common::bit_cast<u32, T>(firstByte);
      firstWord = Common::bSwap32(firstWord);
      firstByte = Common::bit_cast<T, u32>(firstWord);
      if (bswapSecond)
      {
        firstWord = Common::bit_cast<u32, T>(secondByte);
        firstWord = Common::bSwap32(firstWord);
        secondByte = Common::bit_cast<T, u32>(firstWord);
      }
    }
    else if constexpr (size == 8)
    {
      u64 firstDoubleword = Common::bit_cast<u64, T>(firstByte);
      firstDoubleword = Common::bSwap64(firstDoubleword);
      firstByte = Common::bit_cast<T, u64>(firstDoubleword);
      if (bswapSecond)
      {
        firstDoubleword = Common::bit_cast<u64, T>(secondByte);
        firstDoubleword = Common::bSwap64(firstDoubleword);
        secondByte = Common::bit_cast<T, u64>(firstDoubleword);
      }
    }

    if constexpr (std::is_floating_point<T>::value)
      if (std::isnan(firstByte))
        return CompareResult::nan;

    if (firstByte < (secondByte + convertMemoryToType<T>(offset, offsetInvert)))
      return CompareResult::smaller;
    if (firstByte > (secondByte + convertMemoryToType<T>(offset, offsetInvert)))
      return CompareResult::bigger;
    return CompareResult::equal;
  }

  void setType(Common::MemType type);
  void setBase(Common::MemBase base);
  void setEnforceMemAlignment(bool enforceAlignment);
  void setIsSigned(bool isSigned);
  void resetSearchRange();
  bool setSearchRangeBegin(u32 beginRange);
  bool setSearchRangeEnd(u32 endRange);
  bool setSearchRange(u32 beginRange, u32 endRange);

  std::vector<u32> getResultsConsoleAddr() const;
  size_t getResultCount() const;
  bool hasUndo() const;
  size_t getUndoCount() const;
  static int getTermsNumForFilter(ScanFilter filter);
  Common::MemType getType() const;
  Common::MemBase getBase() const;
  size_t getLength() const;
  bool getIsUnsigned() const;
  std::string getFormattedScannedValueAt(int index) const;
  std::string getFormattedCurrentValueAt(int index) const;
  void removeResultAt(int index);
  static bool typeSupportsAdditionalOptions(Common::MemType type);
  bool hasScanStarted() const;
  void removeKnownWatches();

private:
  inline bool isHitNextScan(ScanFilter filter, const char* memoryToCompare1,
                            const char* memoryToCompare2, const char* noOffset,
                            const char* newerRAMCache, size_t realSize, u32 consoleOffset) const;

  bool m_searchInRangeBegin = false;
  bool m_searchInRangeEnd = false;
  u32 m_beginSearchRange = 0;
  u32 m_endSearchRange = 0;

  Common::MemType m_memType = Common::MemType::type_byte;
  Common::MemBase m_memBase = Common::MemBase::base_decimal;
  size_t m_memSize{};
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
