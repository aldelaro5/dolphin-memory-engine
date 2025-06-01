#include "MemWatchEntry.h"

#include <array>
#include <bitset>
#include <cstring>
#include <iomanip>
#include <ios>
#include <limits>
#include <sstream>
#include <utility>

#include <QJsonArray>

#include "../Common/CommonUtils.h"
#include "../DolphinProcess/DolphinAccessor.h"

MemWatchEntry::MemWatchEntry(QString label, const u32 consoleAddress, const Common::MemType type,
                             const Common::MemBase base, const bool isUnsigned, const size_t length,
                             const bool isBoundToPointer, const bool absoluteBranch)
    : m_label(std::move(label)), m_consoleAddress(consoleAddress), m_type(type), m_base(base),
      m_isUnsigned(isUnsigned), m_absoluteBranch(absoluteBranch),
      m_boundToPointer(isBoundToPointer), m_length(length)

{
  m_memory = new char[getSizeForType(m_type, m_length)];
  m_curActualAddress = getActualAddress();
}

MemWatchEntry::MemWatchEntry()
{
  m_type = Common::MemType::type_byte;
  m_base = Common::MemBase::base_decimal;
  m_memory = new char[sizeof(u8)];
  *m_memory = 0;
  m_isUnsigned = false;
  m_consoleAddress = 0x80000000;
  m_curActualAddress = 0x80000000;
  m_absoluteBranch = false;
}

MemWatchEntry::MemWatchEntry(MemWatchEntry* entry)
    : m_label(entry->m_label), m_consoleAddress(entry->m_consoleAddress), m_type(entry->m_type),
      m_base(entry->m_base), m_isUnsigned(entry->m_isUnsigned),
      m_absoluteBranch(entry->m_absoluteBranch), m_boundToPointer(entry->m_boundToPointer),
      m_pointerOffsets(entry->m_pointerOffsets), m_isValidPointer(entry->m_isValidPointer),
      m_length(entry->m_length), m_structName(entry->m_structName),
      m_curActualAddress(entry->m_curActualAddress), m_collectionCount(entry->m_collectionCount)
{
  m_memory = new char[getSizeForType(entry->getType(), entry->getLength())];
  std::memcpy(m_memory, entry->getMemory(), getSizeForType(entry->getType(), entry->getLength()));

  if (entry->getContainerEntry() != nullptr)
    m_containerEntry = new MemWatchEntry(entry->m_containerEntry);
}

MemWatchEntry::~MemWatchEntry()
{
  delete[] m_memory;
  delete[] m_freezeMemory;
  delete m_containerEntry;
}

QString MemWatchEntry::getLabel() const
{
  return m_label;
}

size_t MemWatchEntry::getLength() const
{
  return m_length;
}

Common::MemType MemWatchEntry::getType() const
{
  return m_type;
}

u32 MemWatchEntry::getConsoleAddress() const
{
  return m_consoleAddress;
}

bool MemWatchEntry::isLocked() const
{
  return m_lock;
}

bool MemWatchEntry::isBoundToPointer() const
{
  return m_boundToPointer;
}

Common::MemBase MemWatchEntry::getBase() const
{
  return m_base;
}

int MemWatchEntry::getPointerOffset(const int index) const
{
  return m_pointerOffsets.at(index);
}

std::vector<int> MemWatchEntry::getPointerOffsets() const
{
  return m_pointerOffsets;
}

size_t MemWatchEntry::getPointerLevel() const
{
  return m_pointerOffsets.size();
}

char* MemWatchEntry::getMemory() const
{
  return m_memory;
}

bool MemWatchEntry::isUnsigned() const
{
  return m_isUnsigned;
}

bool MemWatchEntry::isAbsoluteBranch() const
{
  return m_absoluteBranch;
}

void MemWatchEntry::setLabel(const QString& label)
{
  m_label = label;
}

void MemWatchEntry::setConsoleAddress(const u32 address)
{
  m_consoleAddress = address;
  updateActualAddress(getActualAddress());
}

void MemWatchEntry::setTypeAndLength(const Common::MemType type, const size_t length)
{
  size_t oldSize = getSizeForType(m_type, m_length);
  m_type = type;
  m_length = length;
  size_t newSize = getSizeForType(m_type, m_length);
  if (oldSize != newSize)
  {
    delete[] m_memory;
    m_memory = new char[newSize];
    std::fill(m_memory, m_memory + newSize, 0);
  }
}

void MemWatchEntry::setBase(const Common::MemBase base)
{
  m_base = base;
}

void MemWatchEntry::setLock(const bool doLock)
{
  m_lock = doLock;
  if (m_lock)
  {
    if (readMemoryFromRAM() == Common::MemOperationReturnCode::OK)
    {
      m_freezeMemSize = getSizeForType(m_type, m_length);
      m_freezeMemory = new char[m_freezeMemSize];
      std::memcpy(m_freezeMemory, m_memory, m_freezeMemSize);
    }
  }
  else if (m_freezeMemory != nullptr)
  {
    m_freezeMemSize = 0;
    delete[] m_freezeMemory;
    m_freezeMemory = nullptr;
  }
}

void MemWatchEntry::setSignedUnsigned(const bool isUnsigned)
{
  m_isUnsigned = isUnsigned;
}

void MemWatchEntry::setBranchType(const bool absoluteBranch)
{
  m_absoluteBranch = absoluteBranch;
}

void MemWatchEntry::setBoundToPointer(const bool boundToPointer)
{
  m_boundToPointer = boundToPointer;
}

void MemWatchEntry::setPointerOffset(const int pointerOffset, int index)
{
  m_pointerOffsets[index] = pointerOffset;
}

void MemWatchEntry::removeOffset()
{
  m_pointerOffsets.pop_back();
}

QString MemWatchEntry::getStructName() const
{
  return m_structName;
}

void MemWatchEntry::setStructName(QString structName)
{
  m_structName = structName;
}

MemWatchEntry* MemWatchEntry::getContainerEntry() const
{
  return m_containerEntry;
}

void MemWatchEntry::setContainerEntry(MemWatchEntry* elementEntry)
{
  delete m_containerEntry;
  m_containerEntry = elementEntry;
}

size_t MemWatchEntry::getContainerCount() const
{
  return m_collectionCount;
}

void MemWatchEntry::setContainerCount(size_t size)
{
  m_collectionCount = size;
}

void MemWatchEntry::addOffset(const int offset)
{
  m_pointerOffsets.push_back(offset);
}

Common::MemOperationReturnCode MemWatchEntry::freeze()
{
  Common::MemOperationReturnCode writeCode = writeMemoryToRAM(m_freezeMemory, m_freezeMemSize);
  return writeCode;
}

u32 MemWatchEntry::getAddressForPointerLevel(const int level) const
{
  if (!m_boundToPointer && level > static_cast<int>(m_pointerOffsets.size()) && level > 0)
    return 0;

  u32 address = m_consoleAddress;
  std::array<char, sizeof(u32)> addressBuffer{};
  for (int i = 0; i < level; ++i)
  {
    if (DolphinComm::DolphinAccessor::readFromRAM(
            Common::dolphinAddrToOffset(address, DolphinComm::DolphinAccessor::isARAMAccessible()),
            addressBuffer.data(), sizeof(u32), true))
    {
      std::memcpy(&address, addressBuffer.data(), sizeof(u32));
      if (DolphinComm::DolphinAccessor::isValidConsoleAddress(address))
        address += m_pointerOffsets.at(i);
      else
        return 0;
    }
    else
    {
      return 0;
    }
  }
  return address;
}

u32 MemWatchEntry::getActualAddress() const
{
  return getPointerLevel() == 0 ? m_consoleAddress :
                                  getAddressForPointerLevel(static_cast<int>(getPointerLevel()));
}

void MemWatchEntry::updateActualAddress(u32 addr)
{
  m_curActualAddress = addr;
}

bool MemWatchEntry::hasAddressChanged() const
{
  return getActualAddress() != m_curActualAddress;
}

std::string MemWatchEntry::getAddressStringForPointerLevel(const int level) const
{
  u32 address = getAddressForPointerLevel(level);
  if (address == 0)
  {
    return "???";
  }

  std::stringstream ss;
  ss << std::hex << std::uppercase << address;
  return ss.str();
}

Common::MemOperationReturnCode MemWatchEntry::readMemoryFromRAM()
{
  u32 realConsoleAddress = m_consoleAddress;
  if (m_boundToPointer)
  {
    std::array<char, sizeof(u32)> realConsoleAddressBuffer{};
    for (int offset : m_pointerOffsets)
    {
      if (DolphinComm::DolphinAccessor::readFromRAM(
              Common::dolphinAddrToOffset(realConsoleAddress,
                                          DolphinComm::DolphinAccessor::isARAMAccessible()),
              realConsoleAddressBuffer.data(), sizeof(u32), true))
      {
        std::memcpy(&realConsoleAddress, realConsoleAddressBuffer.data(), sizeof(u32));
        if (DolphinComm::DolphinAccessor::isValidConsoleAddress(realConsoleAddress))
        {
          realConsoleAddress += offset;
        }
        else
        {
          m_isValidPointer = false;
          return Common::MemOperationReturnCode::invalidPointer;
        }
      }
      else
      {
        return Common::MemOperationReturnCode::operationFailed;
      }
    }
    // Resolve sucessful
    m_isValidPointer = true;
  }

  if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(realConsoleAddress))
    return Common::MemOperationReturnCode::OK;

  if (DolphinComm::DolphinAccessor::readFromRAM(
          Common::dolphinAddrToOffset(realConsoleAddress,
                                      DolphinComm::DolphinAccessor::isARAMAccessible()),
          m_memory, getSizeForType(m_type, m_length), shouldBeBSwappedForType(m_type)))
    return Common::MemOperationReturnCode::OK;
  return Common::MemOperationReturnCode::operationFailed;
}

Common::MemOperationReturnCode MemWatchEntry::writeMemoryToRAM(const char* memory,
                                                               const size_t size)
{
  u32 realConsoleAddress = m_consoleAddress;
  if (m_boundToPointer)
  {
    std::array<char, sizeof(u32)> realConsoleAddressBuffer{};
    for (int offset : m_pointerOffsets)
    {
      if (DolphinComm::DolphinAccessor::readFromRAM(
              Common::dolphinAddrToOffset(realConsoleAddress,
                                          DolphinComm::DolphinAccessor::isARAMAccessible()),
              realConsoleAddressBuffer.data(), sizeof(u32), true))
      {
        std::memcpy(&realConsoleAddress, realConsoleAddressBuffer.data(), sizeof(u32));
        if (DolphinComm::DolphinAccessor::isValidConsoleAddress(realConsoleAddress))
        {
          realConsoleAddress += offset;
        }
        else
        {
          m_isValidPointer = false;
          return Common::MemOperationReturnCode::invalidPointer;
        }
      }
      else
      {
        return Common::MemOperationReturnCode::operationFailed;
      }
    }
    // Resolve sucessful
    m_isValidPointer = true;
  }

  if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(realConsoleAddress))
    return Common::MemOperationReturnCode::OK;

  if (DolphinComm::DolphinAccessor::writeToRAM(
          Common::dolphinAddrToOffset(realConsoleAddress,
                                      DolphinComm::DolphinAccessor::isARAMAccessible()),
          memory, size, shouldBeBSwappedForType(m_type)))
    return Common::MemOperationReturnCode::OK;
  return Common::MemOperationReturnCode::operationFailed;
}

std::string MemWatchEntry::getStringFromMemory() const
{
  if ((m_boundToPointer && !m_isValidPointer) ||
      !DolphinComm::DolphinAccessor::isValidConsoleAddress(m_consoleAddress))
    return "???";
  return Common::formatMemoryToString(m_memory, m_type, m_length, m_base, m_isUnsigned, false,
                                      m_absoluteBranch ? m_consoleAddress : 0);
}

Common::MemOperationReturnCode MemWatchEntry::writeMemoryFromString(const std::string& inputString)
{
  Common::MemOperationReturnCode writeReturn = Common::MemOperationReturnCode::OK;
  size_t sizeToWrite = 0;
  char* buffer = Common::formatStringToMemory(writeReturn, sizeToWrite, inputString, m_base, m_type,
                                              m_length, m_consoleAddress);
  if (writeReturn != Common::MemOperationReturnCode::OK)
    return writeReturn;

  writeReturn = writeMemoryToRAM(buffer, sizeToWrite);
  if (writeReturn == Common::MemOperationReturnCode::OK)
  {
    if (m_lock)
      std::memcpy(m_freezeMemory, buffer, m_freezeMemSize);
  }
  delete[] buffer;
  return writeReturn;
}

void MemWatchEntry::readFromJson(const QJsonObject& json)
{
  setLabel(json["label"].toString());
  std::stringstream ss(json["address"].toString().toStdString());
  u32 address = 0;
  ss >> std::hex >> std::uppercase >> address;
  setConsoleAddress(address);
  size_t length = 1;
  if (json["length"] != QJsonValue::Undefined)
    length = static_cast<size_t>(json["length"].toDouble());
  setTypeAndLength(static_cast<Common::MemType>(json["typeIndex"].toInt()), length);

  if (json["structName"] != QJsonValue::Undefined)
    setStructName(json["structName"].toString());

  if (json["containerCount"] != QJsonValue::Undefined)
  {
    setContainerCount(static_cast<size_t>(json["containerCount"].toDouble()));
    MemWatchEntry* containerEntry = new MemWatchEntry();
    containerEntry->readFromJson(json["containerEntry"].toObject());
    setContainerEntry(containerEntry);
  }

  setSignedUnsigned(json["unsigned"].toBool());
  setBase(static_cast<Common::MemBase>(json["baseIndex"].toInt()));
  if (json["pointerOffsets"] != QJsonValue::Undefined)
  {
    setBoundToPointer(true);
    QJsonArray pointerOffsets = json["pointerOffsets"].toArray();
    for (auto i : pointerOffsets)
    {
      std::stringstream ssOffset(i.toString().toStdString());
      int offset = 0;
      ssOffset >> std::hex >> std::uppercase >> offset;
      addOffset(offset);
    }
  }
  else
  {
    setBoundToPointer(false);
  }

  if (json["absoluteBranch"] != QJsonValue::Undefined)
    m_absoluteBranch = json["absoluteBranch"].toBool();
}

void MemWatchEntry::writeToJson(QJsonObject& json) const
{
  json["label"] = getLabel();
  std::stringstream ss;
  ss << std::hex << std::uppercase << getConsoleAddress();
  json["address"] = QString::fromStdString(ss.str());
  json["typeIndex"] = static_cast<double>(getType());
  json["unsigned"] = isUnsigned();
  if (getType() == Common::MemType::type_string || getType() == Common::MemType::type_byteArray)
    json["length"] = static_cast<double>(getLength());
  else if (getType() == Common::MemType::type_struct)
    json["structName"] = getStructName();
  else if (getType() == Common::MemType::type_array)
  {
    QJsonObject containerEntryJson{};
    getContainerEntry()->writeToJson(containerEntryJson);
    json["containerEntry"] = containerEntryJson;
    json["containerCount"] = static_cast<double>(getContainerCount());
  }

  json["baseIndex"] = static_cast<double>(getBase());
  if (isBoundToPointer())
  {
    QJsonArray offsets;
    for (int i{0}; i < static_cast<int>(getPointerOffsets().size()); ++i)
    {
      std::stringstream ssOffset;
      ssOffset << std::hex << std::uppercase << getPointerOffset(i);
      offsets.append(QString::fromStdString(ssOffset.str()));
    }
    json["pointerOffsets"] = offsets;
  }

  json["absoluteBranch"] = m_absoluteBranch;
}
