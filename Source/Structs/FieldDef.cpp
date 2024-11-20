#include "FieldDef.h"

#include <QJsonArray>

FieldDef::FieldDef()
{
  m_structOffset = 0;
  m_isPropagated = true;
  m_entry = new MemWatchEntry();
}

FieldDef::FieldDef(size_t offset, bool isPropagated, MemWatchEntry* entry)
{
  m_structOffset = offset;
  m_isPropagated = isPropagated;
  m_entry = entry;
}

FieldDef::~FieldDef()
{
  delete m_entry;
  m_entry = nullptr;
}

size_t FieldDef::getOffset() const
{
  return m_structOffset;
}

bool FieldDef::getIsPropagated() const
{
  return m_isPropagated;
}

void FieldDef::setOffset() const
{
}

void FieldDef::setIsPropagated() const
{
}

MemWatchEntry* FieldDef::getEntry() const
{
  return m_entry;
}

void FieldDef::readFromJSON(const QJsonObject& json)
{
  m_structOffset = json["offset"].toInt();
  m_isPropagated = json["isPropagated"].toBool();

  MemWatchEntry* entry = new MemWatchEntry();

  if (json["entry"] != QJsonValue::Undefined)
    entry->readFromJson(json["entry"].toObject());
}

void FieldDef::writeToJson(QJsonObject& json)
{
  json["offset"] = static_cast<double>(m_structOffset);
  json["isPropagated"] = m_isPropagated;

  if (!m_entry)
    return;

  QJsonObject entryJson;
  m_entry->writeToJson(entryJson);
  json["entry"] = entryJson;
}
