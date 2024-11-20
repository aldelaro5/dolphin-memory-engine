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

StructDef::StructDef(QString label, u32 length, QVector<FieldDef*> entries)
{
  m_label = label;
  m_length = length;
  m_fields = entries;
  m_isValid = isValidStruct();
}

StructDef::StructDef(StructDef* structDef)
    : m_label(structDef->getLabel()), m_length(structDef->getLength()),
      m_isValid(structDef->m_isValid)
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
  const size_t structSegments = static_cast<size_t>(ceil(static_cast<float>(length) / segbitsize));
  uint64_t* structBytes = new uint64_t[structSegments];
  for (size_t i = 0; i < structSegments; i++)
  {
    structBytes[i] = 0;
  }

  for (FieldDef* field : fields)
  {
    size_t fieldOffset = field->getOffset();
    size_t fieldLength = static_cast<size_t>(field->getFieldSize());
    if (fieldOffset + fieldLength > length)
    {
      return false;
    }

    size_t firstSegment = static_cast<size_t>(floor(static_cast<float>(fieldOffset) / segbitsize));
    size_t lastSegment =
        static_cast<size_t>(floor(static_cast<float>(fieldOffset + fieldLength) / segbitsize));

    size_t lengthChecked = 0;
    for (size_t i = firstSegment; i <= lastSegment; i++)
    {
      uint64_t entryByteMask = 0ULL;
      if (i == firstSegment)
      {
        size_t maskLength = std::min(fieldLength, (segbitsize - fieldOffset % segbitsize));
        if (maskLength == 0x40)
          entryByteMask--;
        else
          entryByteMask = ((1ULL << (maskLength)) - 1) << fieldOffset;
        lengthChecked += maskLength;
      }
      else if (i == lastSegment)
      {
        entryByteMask = ((1ULL << (fieldLength - lengthChecked)) - 1);
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

void StructDef::addFields(FieldDef* field, int index)
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
    if (field->getEntry()->getType() == Common::MemType::type_struct &&
        field->getEntry()->getStructName() == oldLabel)
      field->getEntry()->setStructName(newLabel);
}

void StructDef::updateStructFieldSize(QString structName, u32 newLength)
{
  for (FieldDef* field : m_fields)
    if (field->getEntry()->getType() == Common::MemType::type_struct &&
        field->getEntry()->getStructName() == structName)
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

bool StructDef::isSame(const StructDef* other) const
{
  if (m_label != other->m_label || m_length != other->m_length ||
      m_fields.count() != other->m_fields.count())
    return false;

  for (int i = 0; i < m_fields.count(); i++)
    if (!m_fields[i]->isSame(other->m_fields[i]))
      return false;
  return true;
}

QString StructDef::getDiffString(const StructDef* other) const
{
  QString diffs = QString();
  if (m_label != other->m_label)
    diffs += QString("\nStruct Label: %1 -> %2").arg(m_label).arg(other->m_label);
  if (m_length != other->m_length)
    diffs += QString("\nStruct Length: %1 -> %2").arg(m_length).arg(other->m_length);
  int i = 0;
  while (i < std::max(m_fields.count(), other->m_fields.count()))
  {
    if (m_fields.count() > i)
    {
      if (other->m_fields.count() > i)
      {
        QStringList fieldDiff = m_fields[i]->diffList(other->m_fields[i]);
        diffs += fieldDiff.isEmpty() ?
                     "" :
                     QString("\n  Field %1:\n    %2").arg(i).arg(fieldDiff.join("\n    "));
      }
      else
        diffs += QString("\n  Field %1:\n    ").arg(i) +
                 m_fields[i]->getFieldDescLines().join(" -> N/A\n    ") + " -> N/A";
    }
    else
      diffs += QString("\n  Field %1:\n    N/A -> ").arg(i) +
               other->m_fields[i]->getFieldDescLines().join("\n    N/A -> ");
    i++;
  }
  return diffs;
}

void StructDef::recalculateOffsets()
{
  u32 cur_offset = 0;
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
    m_length = static_cast<u32>(
        fmax(m_fields.last()->getOffset() + m_fields.last()->getFieldSize(), m_length));
}
