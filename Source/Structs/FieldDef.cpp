#include "FieldDef.h"

#include <QJsonArray>

#include "../GUI/GUICommon.h"

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

FieldDef::FieldDef(FieldDef* field) : m_structOffset(field->m_structOffset), m_size(field->m_size)

{
  if (field->isPadding())
    m_entry = nullptr;
  else
    m_entry = new MemWatchEntry(field->m_entry);
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
  if (m_size != other->m_size || m_structOffset != other->m_structOffset ||
      m_entry->getLabel() != other->m_entry->getLabel() ||
      m_entry->getType() != other->m_entry->getType() ||
      Common::getSizeForType(m_entry->getType(), m_entry->getLength()) !=
          Common::getSizeForType(other->m_entry->getType(), other->m_entry->getLength()) ||
      m_entry->isBoundToPointer() != other->m_entry->isBoundToPointer())
    return false;
  if (m_entry->isBoundToPointer())
  {
    if (m_entry->getPointerLevel() != other->m_entry->getPointerLevel())
      return false;
    else
      for (int i = 0; i < static_cast<int>(m_entry->getPointerLevel()); i++)
        if (m_entry->getPointerOffset(i) != other->m_entry->getPointerOffset(i))
          return false;
  }
  if (m_entry->getType() == Common::MemType::type_struct &&
      m_entry->getStructName() != other->m_entry->getStructName())
    return false;
  return true;
}

QStringList FieldDef::diffList(FieldDef* const other) const
{
  QStringList diffs{};
  if (m_entry->getLabel() != other->m_entry->getLabel())
    diffs += QString("Name: %1 -> %2").arg(m_entry->getLabel()).arg(other->m_entry->getLabel());
  if (m_size != other->m_size)
    diffs += QString("Field Size: %1 -> %2").arg(m_size).arg(other->m_size);
  if (m_structOffset != other->m_structOffset)
    diffs += QString("Struct Offset: %1 -> %2").arg(m_structOffset).arg(other->m_structOffset);
  if (m_entry->getType() != other->m_entry->getType())
    diffs += QString("Type: %1 -> %2")
                 .arg(GUICommon::getStringFromType(m_entry->getType()))
                 .arg(GUICommon::getStringFromType(other->m_entry->getType()));
  if (m_entry->getType() == Common::MemType::type_struct)
  {
    if (other->m_entry->getType() == Common::MemType::type_struct &&
        m_entry->getStructName() != other->m_entry->getStructName())
      diffs += QString("Struct Name: %1 -> %2")
                   .arg(m_entry->getStructName())
                   .arg(other->m_entry->getStructName());
    else if (other->m_entry->getType() != Common::MemType::type_struct)
      diffs += QString("Struct Name: %1 -> N/A").arg(m_entry->getStructName());
  }
  else if (other->m_entry->getType() == Common::MemType::type_struct)
    diffs += QString("Struct Name: N/A -> %1").arg(other->m_entry->getStructName());

  size_t oldSize = Common::getSizeForType(m_entry->getType(), m_entry->getLength());
  if (m_entry->isBoundToPointer())
    oldSize = 4;

  size_t newSize = Common::getSizeForType(other->m_entry->getType(), other->m_entry->getLength());
  if (other->m_entry->isBoundToPointer())
    newSize = 4;

  if (oldSize != newSize)
    diffs += QString("Entry Length: %1 -> %2").arg(oldSize).arg(newSize);

  if (m_entry->isBoundToPointer() != other->m_entry->isBoundToPointer())
    diffs += QString("Is Pointer: %1 -> %2")
                 .arg(m_entry->isBoundToPointer() ? "Yes" : "No")
                 .arg(other->m_entry->isBoundToPointer() ? "Yes" : "No");

  if (m_entry->isBoundToPointer() || other->m_entry->isBoundToPointer())
  {
    int i = 0;
    while (i < static_cast<int>(
                   std::max(m_entry->getPointerLevel(), other->m_entry->getPointerLevel())))
    {
      if (static_cast<int>(m_entry->getPointerLevel()) > i)
      {
        if (static_cast<int>(other->m_entry->getPointerLevel()) > i &&
            m_entry->getPointerOffset(i) != other->m_entry->getPointerOffset(i))
          diffs += QString("  Pointer Offset %1: %2 -> %3")
                       .arg(i)
                       .arg(m_entry->getPointerOffset(i))
                       .arg(other->m_entry->getPointerOffset(i));
        else if (!(static_cast<int>(other->m_entry->getPointerLevel()) > i))
          diffs +=
              QString("  Pointer Offset %1: %2 -> N/A").arg(i).arg(m_entry->getPointerOffset(i));
      }
      else if (static_cast<int>(other->m_entry->getPointerLevel()) > i)
        diffs += QString("  Pointer Offset %1: N/A -> %3")
                     .arg(i)
                     .arg(other->m_entry->getPointerOffset(i));
      i++;
    }
  }
  return diffs;
}

QStringList FieldDef::getFieldDescLines() const
{
  QStringList descLines{};
  descLines += QString("Name: %1").arg(m_entry->getLabel());
  descLines += QString("Field Size: %1").arg(m_size);
  descLines += QString("Struct Offset: %1").arg(m_structOffset);
  descLines += QString("Type: %1").arg(GUICommon::getStringFromType(m_entry->getType()));
  if (m_entry->getType() == Common::MemType::type_struct)
    descLines += QString("Struct Name: %1").arg(m_entry->getStructName());
  size_t size = Common::getSizeForType(m_entry->getType(), m_entry->getLength());
  descLines += QString("Entry Length: %1").arg(size);
  descLines += QString("Is Pointer: %1").arg(m_entry->isBoundToPointer() ? "Yes" : "No");
  if (m_entry->isBoundToPointer())
  {
    int i = 0;
    while (i < static_cast<int>(m_entry->getPointerLevel()))
    {
      descLines += QString("  Pointer Offset %1: %2").arg(i).arg(m_entry->getPointerOffset(i));
      i++;
    }
  }
  return descLines;
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
