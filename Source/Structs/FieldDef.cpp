#include "FieldDef.h"

#include <QJsonArray>

FieldDef::FieldDef()
{
  m_structOffset = 0;
  m_size = 0;
  m_entry = new MemWatchEntry();
}

FieldDef::FieldDef(u32 offset, u32 size, bool is_padding)
{
  m_structOffset = offset;
  m_size = size;
  if (is_padding)
    m_entry = nullptr;
  else
    m_entry = new MemWatchEntry();
}

FieldDef::FieldDef(u32 offset, MemWatchEntry* entry)
{
  m_structOffset = offset;
  m_size = 0;
  m_entry = entry;
}

FieldDef::FieldDef(FieldDef* field)
    : m_structOffset(field->m_structOffset), m_size(field->m_size),
      m_entry(new MemWatchEntry(field->m_entry))
{
}

FieldDef::~FieldDef()
{
  delete m_entry;
  m_entry = nullptr;
}

u32 FieldDef::getOffset() const
{
  return m_structOffset;
}

void FieldDef::setOffset(u32 offset)
{
  m_structOffset = offset;
}

MemWatchEntry* FieldDef::getEntry() const
{
  return m_entry;
}

void FieldDef::setEntry(MemWatchEntry* entry)
{
  if (m_entry != nullptr)
    delete (m_entry);
  if (entry != nullptr)
    m_entry = entry;
}

u32 FieldDef::getFieldSize() const
{
 return m_size;
}

void FieldDef::setFieldSize(u32 size)
{
 m_size = size;
}

QString FieldDef::getLabel() const
{
  if (!m_entry)
    return QString("");
  return m_entry->getLabel();
}

void FieldDef::setLabel(QString label)
{
  if (m_entry)
    m_entry->setLabel(label);
}

bool FieldDef::isPadding() const
{
  return m_entry == nullptr;
}

void FieldDef::convertToPadding()
{
  if (m_entry != nullptr)
    delete m_entry;
  m_entry = nullptr;
  m_size = 1;
}

bool FieldDef::isSame(FieldDef* const other) const
{
  // May want to eventually include m_entry->m_base or m_entry->m_isUnsigned in this check.
  if (
    m_size != other->m_size || m_structOffset != other->m_structOffset ||
    m_entry->getLabel() != other->m_entry->getLabel() ||
    m_entry->getType() != other->m_entry->getType() ||
    Common::getSizeForType(m_entry->getType(), m_entry->getLength()) != Common::getSizeForType(other->m_entry->getType(), other->m_entry->getLength()) ||
    m_entry->isBoundToPointer() != other->m_entry->isBoundToPointer()
    )
    return false;
  if (m_entry->isBoundToPointer())
  {
    if (m_entry->getPointerOffsets().size() != other->m_entry->getPointerOffsets().size())
      return false;
    else
      for (int i = 0; i < m_entry->getPointerLevel(); i++)
        if (m_entry->getPointerOffset(i) != other->m_entry->getPointerOffset(i))
          return false;
  }
  if (m_entry->getType() == Common::MemType::type_struct &&
      m_entry->getStructName() != other->m_entry->getStructName())
    return false;
  return true;
}

void FieldDef::readFromJSON(const QJsonObject& json)
{
  m_structOffset = json["offset"].toInt();
  m_size = json["length"].toInt();

  if (json["entry"] == QJsonValue::Undefined)
    m_entry = nullptr;
  else
    m_entry->readFromJson(json["entry"].toObject());
}

void FieldDef::writeToJson(QJsonObject& json)
{
  json["offset"] = static_cast<double>(m_structOffset);
  json["length"] = static_cast<double>(m_size);

  if (!m_entry)
    return;

  QJsonObject entryJson;
  m_entry->writeToJson(entryJson);
  json["entry"] = entryJson;
}
