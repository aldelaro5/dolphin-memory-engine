#pragma once

#include <string>
#include <vector>

#include <QJsonObject>
#include <QString>

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"

class MemWatchEntry
{
public:
  MemWatchEntry();
  MemWatchEntry(QString label, u32 consoleAddress, Common::MemType type,
                Common::MemBase = Common::MemBase::base_decimal, bool m_isUnsigned = false,
                size_t length = 1, bool isBoundToPointer = false,
                const bool absoluteBranch = false);
  explicit MemWatchEntry(MemWatchEntry* entry);
  ~MemWatchEntry();

  MemWatchEntry(const MemWatchEntry&) = delete;
  MemWatchEntry(MemWatchEntry&&) = delete;
  MemWatchEntry& operator=(const MemWatchEntry&) = delete;
  MemWatchEntry& operator=(MemWatchEntry&&) = delete;

  QString getLabel() const;
  Common::MemType getType() const;
  u32 getConsoleAddress() const;
  bool isLocked() const;
  bool isBoundToPointer() const;
  Common::MemBase getBase() const;
  size_t getLength() const;
  char* getMemory() const;
  bool isUnsigned() const;
  bool isAbsoluteBranch() const;
  int getPointerOffset(int index) const;
  std::vector<int> getPointerOffsets() const;
  size_t getPointerLevel() const;
  void setLabel(const QString& label);
  void setConsoleAddress(u32 address);
  void setTypeAndLength(Common::MemType type, size_t length = 1);
  void setBase(Common::MemBase base);
  void setLock(bool doLock);
  void setSignedUnsigned(bool isUnsigned);
  void setBranchType(bool absoluteBranch);  // true = absolute, false = relative
  void setBoundToPointer(bool boundToPointer);
  void setPointerOffset(int pointerOffset, int index);
  void addOffset(int offset);
  void removeOffset();

  QString getStructName() const;
  void setStructName(QString structName);

  MemWatchEntry* getContainerEntry() const;
  void setContainerEntry(MemWatchEntry* elementEntry);
  size_t getContainerCount() const;
  void setContainerCount(size_t size);

  Common::MemOperationReturnCode freeze();

  u32 getAddressForPointerLevel(int level) const;
  u32 getActualAddress() const;
  void updateActualAddress(u32 addr);
  bool hasAddressChanged() const;
  std::string getAddressStringForPointerLevel(int level) const;
  Common::MemOperationReturnCode readMemoryFromRAM();

  std::string getStringFromMemory() const;
  Common::MemOperationReturnCode writeMemoryFromString(const std::string& inputString);

  void readFromJson(const QJsonObject& json);
  void writeToJson(QJsonObject& json) const;

private:
  Common::MemOperationReturnCode writeMemoryToRAM(const char* memory, size_t size);

  QString m_label;
  u32 m_consoleAddress;
  bool m_lock = false;
  Common::MemType m_type;
  Common::MemBase m_base;
  bool m_isUnsigned;
  bool m_absoluteBranch;  // true = absolute, false = relative
  bool m_boundToPointer = false;
  std::vector<int> m_pointerOffsets;
  bool m_isValidPointer = false;
  char* m_memory;
  char* m_freezeMemory = nullptr;
  size_t m_freezeMemSize = 0;
  size_t m_length = 1;
  QString m_structName;
  u32 m_curActualAddress;
  MemWatchEntry* m_containerEntry{};
  size_t m_collectionCount = 1;
};
