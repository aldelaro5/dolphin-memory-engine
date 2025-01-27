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

StructDef::StructDef(StructDef* structDef)
    : m_label(structDef->getLabel()), m_length(structDef->getLength()), m_isValid(structDef->m_isValid)
{
  m_fields = QVector<FieldDef*>();
  for (FieldDef* field : structDef->getFields())
    m_fields.push_back(new FieldDef(field));
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
  const size_t segbitsize = sizeof(uint64_t) * 8;
  const size_t structSegments = ceil(static_cast<float>(length) / segbitsize);
  uint64_t* structBytes = new uint64_t[structSegments];
  for (size_t i = 0; i < structSegments; i++)
  {
    structBytes[i] = 0;
  }

  for (FieldDef* field : fields)
  {
    size_t fieldOffset = field->getOffset();
    size_t fieldLength = field->getFieldSize();
    if (fieldOffset + fieldLength > length)
    {
      return false;
    }

    size_t firstSegment = floor(static_cast<float>(fieldOffset) / segbitsize);
    size_t lastSegment = floor(static_cast<float>(fieldOffset + fieldLength) / segbitsize);

    size_t lengthChecked = 0;
    for (size_t i = firstSegment; i <= lastSegment; i++)
    {
      uint64_t entryByteMask = 0ULL;
      if (i == firstSegment)
      {
        size_t maskLength = fmin(fieldLength, (segbitsize - fieldOffset % segbitsize));
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
        lengthChecked += segbitsize;
      }

      if (structBytes[i] & entryByteMask)
      {
        return false;
      }

      structBytes[i] ^= entryByteMask;
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
    if (field->getEntry()->getType() == Common::MemType::type_struct && field->getEntry()->getStructName() == oldLabel)
      field->getEntry()->setStructName(newLabel);
}

void StructDef::updateStructFieldSize(QString structName, u32 newLength)
{
  for (FieldDef* field : m_fields)
    if (field->getEntry()->getType() == Common::MemType::type_struct && field->getEntry()->getStructName() == structName)
      field->setFieldSize(newLength);
  recalculateOffsets();
}

void StructDef::readFromJson(const QJsonObject& json)
{
  m_label = json["label"].toString();
  m_length = json["length"].toInt();
  QJsonArray fieldList = json["fieldList"].toArray();
  for (auto i : fieldList)
  {
    QJsonObject fieldJson = i.toObject();
    FieldDef* field = new FieldDef();
    field->readFromJSON(fieldJson);
    m_fields.append(field);
  }
}

void StructDef::writeToJson(QJsonObject& json)
{
  json["label"] = m_label;
  json["length"] = static_cast<double>(m_length);

  QJsonArray fieldArray;
  for (FieldDef* const field : m_fields)
  {
    QJsonObject node;
    field->writeToJson(node);
    fieldArray.append(node);
  }
  json["fieldList"] = fieldArray;
}

void StructDef::recalculateOffsets()
{
  int cur_offset = 0;
  for (int i = 0; i < m_fields.count(); ++i)
  {
    if (m_fields[i]->getOffset() != cur_offset)
      m_fields[i]->setOffset(cur_offset);
    cur_offset += m_fields[i]->getFieldSize();
  }
  calculateLength();
}

void StructDef::calculateLength()
{
  if (!m_fields.isEmpty())
    m_length = static_cast<u32>(fmax(m_fields.last()->getOffset() + m_fields.last()->getFieldSize(), m_length));
}
