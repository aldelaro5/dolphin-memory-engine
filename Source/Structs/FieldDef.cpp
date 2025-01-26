#include "FieldDef.h"

#include <QJsonArray>

FieldDef::FieldDef()
{
  m_structOffset = 0;
  m_size = -1;
  m_entry = new MemWatchEntry();
}

FieldDef::FieldDef(size_t offset, size_t size, bool is_padding)
{
  m_structOffset = offset;
  m_size = size;
  if (is_padding)
    m_entry = nullptr;
  else
    m_entry = new MemWatchEntry();
}

FieldDef::FieldDef(size_t offset, MemWatchEntry* entry)
{
  m_structOffset = offset;
  m_size = -1;
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

size_t FieldDef::getFieldSize() const
{
 return m_size;
}

void FieldDef::setFieldSize(size_t size)
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

void FieldDef::readFromJSON(const QJsonObject& json)
{
  m_structOffset = json["offset"].toInt();
  m_size = json["length"].toInt();

  if (json["entry"] == QJsonValue::Undefined)
    m_entry == nullptr;
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
