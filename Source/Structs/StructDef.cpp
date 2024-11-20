#include <QJsonArray>

#include "StructDef.h"

StructDef::StructDef()
{
  m_label = QString("");
  m_length = 0;
  m_fields = QVector<FieldDef*>();
  m_isPacked = false;
}

StructDef::StructDef(QString label, size_t length, QVector<FieldDef*> entries, bool isPacked)
{
  m_label = label;
  m_length = length;
  m_fields = entries;
  m_isPacked = isPacked;
}

StructDef::~StructDef()
{
  qDeleteAll(m_fields);
}

QString StructDef::getLabel()
{
  return m_label;
}

size_t StructDef::getLength()
{
  return m_length;
}

bool StructDef::isValidStruct()
{
  if (m_isPacked)
    return true;

  size_t structSegments = ceil(m_length / sizeof(uint64_t));
  uint64_t* structBytes = new uint64_t[structSegments];
  for (size_t i = 0; i < structSegments; i++)
  {
    structBytes[i] = 0;
  }

  for (FieldDef* field : m_fields)
  {
    size_t fieldOffset = field->getOffset();
    size_t fieldLength = field->getEntry()->getLength();
    if (fieldOffset + fieldLength > m_length)
    {
      return false;
    }

    size_t firstSegment = floor(fieldOffset / sizeof(uint64_t));
    size_t lastSegment = floor((fieldOffset + fieldLength) / sizeof(uint64_t)) - firstSegment;

    size_t lengthChecked = 0;
    for (size_t i = firstSegment; i <= lastSegment; i++)
    {
      uint64_t entryByteMask = 0ULL;
      if (i == firstSegment)
      {
        size_t maskLength = fmin(fieldLength, (sizeof(uint64_t) - fieldOffset % sizeof(uint64_t)));
        entryByteMask = ((1ULL << maskLength) - 1) << fieldOffset;
        lengthChecked += maskLength;
      }
      else if (firstSegment != lastSegment && i == lastSegment)
      {
        entryByteMask = ((1ULL << (fieldLength - lengthChecked)) - 1) << fieldOffset;
        lengthChecked += (fieldLength - lengthChecked);
      }
      else
      {
        entryByteMask--;
        lengthChecked += sizeof(uint64_t);
      }

      if (structBytes[firstSegment] & entryByteMask)
      {
        return false;
      }

      structBytes[firstSegment] ^= entryByteMask;
    }

    
  }

  return true;
}

void StructDef::setLength(size_t length)
{
  m_length = length;
}

void StructDef::setLabel(const QString& label)
{
  m_label = label;
}

void StructDef::addFields(FieldDef* field, size_t index)
{
  if (index == -1)
    m_fields.append(field);

  m_fields.insert(index, field);

  if (m_isPacked)
    calculateLength();
}

void StructDef::clearFields()
{
  qDeleteAll(m_fields);
  m_fields.clear();
  m_length = 0;
}

void StructDef::setFields(QVector<FieldDef*> fields)
{
  qDeleteAll(m_fields);
  m_fields = fields;

  if (m_isPacked)
    calculateLength();
}

void StructDef::readFromJson(const QJsonObject& json)
{
  m_label = json["label"].toString();
  m_length = json["length"].toInt();
  m_isPacked = json["isPacked"].toBool();
  QJsonArray entry_list = json["entryList"].toArray();
  for (auto i : entry_list)
  {
    QJsonObject entry = i.toObject();
    FieldDef* childEntry = new FieldDef();
    childEntry->readFromJSON(entry);
    m_fields.append(childEntry);
  }
}

void StructDef::writeToJson(QJsonObject& json)
{
  json["label"] = m_label;
  json["length"] = static_cast<double>(m_length);
  json["isPacked"] = m_isPacked;

  QJsonArray entryArray;
  for (FieldDef* const entry : m_fields)
  {
    QJsonObject node;
    entry->writeToJson(node);
    entryArray.append(node);
  }
  json["entryList"] = entryArray;
}

void StructDef::calculateLength()
{
  size_t max_length = 0;
  for (FieldDef* field : m_fields)
  {
    max_length = fmax(max_length, field->getOffset() + field->getEntry()->getLength());
  }
  m_length = max_length;
}
