#include <QJsonArray>

#include "StructDef.h"

StructDef::StructDef()
{
  m_label = QString("");
  m_length = 0;
  m_fields = QVector<FieldDef*>();
  m_isValid = true;
}

StructDef::StructDef(QString label)
{
  m_label = label;
  m_length = 0;
  m_fields = QVector<FieldDef*>();
  m_isValid = true;
}

StructDef::StructDef(QString label, u32 length, QVector<FieldDef*> entries, bool isPacked)
{
  m_label = label;
  m_length = length;
  m_fields = entries;
  m_isValid = isValidStruct();
}

StructDef::~StructDef()
{
  qDeleteAll(m_fields);
}

QString StructDef::getLabel()
{
  return m_label;
}

u32 StructDef::getLength()
{
  return m_length;
}

QVector<FieldDef*> StructDef::getFields()
{
  return m_fields;
}


bool StructDef::isValidFieldLayout(u32 length, QVector<FieldDef*> fields)
{
  size_t structSegments = ceil(length / sizeof(uint64_t));
  uint64_t* structBytes = new uint64_t[structSegments];
  for (size_t i = 0; i < structSegments; i++)
  {
    structBytes[i] = 0;
  }

  for (FieldDef* field : fields)
  {
    size_t fieldOffset = field->getOffset();
    size_t fieldLength = field->getEntry()->getLength();
    if (fieldOffset + fieldLength > length)
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

bool StructDef::isValidStruct()
{
  return isValidFieldLayout(m_length, m_fields);
}

void StructDef::setLength(u32 length)
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
}

void StructDef::clearFields()
{
  qDeleteAll(m_fields);
  m_fields.clear();
  m_length = 0;
}

void StructDef::setFields(QVector<FieldDef*> fields)
{
  if (!isValidFieldLayout(m_length, fields))
  {
    qDebug("Attempted to set invalid vector of fields to a StructDef");
    return;
  }
  qDeleteAll(m_fields);
  m_fields = fields;
}

void StructDef::updateStructTypeLabel(const QString& oldLabel, QString newLabel)
{
  for (FieldDef* field : m_fields)
  {
    if (field->getEntry()->getType() == Common::MemType::type_struct &&
        field->getEntry()->getStructName() == oldLabel)
      field->getEntry()->setStructName(newLabel);
  }
}

void StructDef::readFromJson(const QJsonObject& json)
{
  m_label = json["label"].toString();
  m_length = json["length"].toInt();
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
  u32 max_length = 0;
  u32 cur_length = 0;
  for (FieldDef* field : m_fields)
  {
    u32 field_length = field->getEntry()->getLength();
    max_length = fmax(max_length, field->getOffset() + field_length);
    cur_length += field_length;
  }
  m_length = fmax(max_length, m_length);
}
