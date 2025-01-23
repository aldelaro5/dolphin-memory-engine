#include "StructDetailModel.h"

#include <QMimeData>

#include "../GUICommon.h"
#include "../../Common/CommonUtils.h"
#include "../../Structs/StructTreeNode.h"



StructDetailModel::StructDetailModel(QObject* parent) : QAbstractListModel(parent)
{
}

StructDetailModel::~StructDetailModel()
{
  qDeleteAll(m_fields);
}

int StructDetailModel::columnCount(const QModelIndex& parent) const
{
  return STRUCT_COL_NUM;
}

int StructDetailModel::rowCount(const QModelIndex& parent) const
{
  return m_fields.count();
}

QVariant StructDetailModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return {};

  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
    case STRUCT_COL_OFFSET:
      return QString("0x%1").arg(m_fields[index.row()]->getOffset());
    case STRUCT_COL_SIZE:
      return m_fields[index.row()]->getSize();
    case STRUCT_COL_LABEL:
      if (m_fields[index.row()]->isPadding())
        return QString("-- Padding --");
      return m_fields[index.row()]->getEntry()->getLabel();
    case STRUCT_COL_DETAIL:
    {
      return getFieldDetails(m_fields[index.row()]);
    }
      
    default:
      break;
    }
  }
  return {};
}

bool StructDetailModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (~role == Qt::EditRole || !index.isValid())
    return false;

  FieldDef* field = static_cast<FieldDef*>(index.internalPointer());

  switch (index.column())
  {
  case STRUCT_COL_LABEL:
  {
    if (field->isPadding())
      return false;
    field->getEntry()->setLabel(value.toString());
    emit dataChanged(index, index);
    emit dataEdited(index, value, role);
    return true;
  }
  default:
    return false;
  }
}

QVariant StructDetailModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case STRUCT_COL_OFFSET:
      return tr("Offset");
    case STRUCT_COL_SIZE:
      return tr("Size");
    case STRUCT_COL_LABEL:
      return tr("Name");
    case STRUCT_COL_DETAIL:
      return tr("Value");
    default:
      break;
    }
  }
  return {};
}

Qt::ItemFlags StructDetailModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  Qt::ItemFlags flags =  Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (index.column() == STRUCT_COL_LABEL && !m_fields[index.row()]->isPadding())
    flags |= Qt::ItemIsEditable;

  return flags;
}

Qt::DropActions StructDetailModel::supportedDropActions() const
{
  return Qt::IgnoreAction;
}

Qt::DropActions StructDetailModel::supportedDragActions() const
{
  return Qt::IgnoreAction;
}

bool StructDetailModel::editData(const QModelIndex& index, const QVariant& value, int role,
                                 bool emitEdit)
{
  if (!index.isValid())
    return false;

  if (index.column() == STRUCT_COL_LABEL)
  {
    FieldDef* field = static_cast<FieldDef*>(index.internalPointer());
    field->setLabel(value.toString());
  }
}

void StructDetailModel::addPaddingFields(int count, int start)
{
  bool updateOffsets = start >= 0;
  u32 starting_offset;

  if (m_fields.isEmpty())
  {
    start = 0;
    starting_offset = 0;
  }
  else if (start < 0)
  {
    start = m_fields.count();
    starting_offset = m_fields.last()->getOffset() + m_fields.last()->getSize();
  }
  else
  {
    starting_offset = m_fields[start]->getOffset() + m_fields[start]->getSize();
  }

  beginInsertRows(QModelIndex(), start, start + count - 1);

  for (int i = 0; i < count; ++i)
  {
    m_fields.insert(start + i, new FieldDef(starting_offset + i, 1, true));
  }

  endInsertRows();

  if (updateOffsets)
    updateFieldOffsets();
}

void StructDetailModel::removeFields(int start, int count)
{
  beginRemoveRows(QModelIndex(), start, start + count - 1);

  for (int i = start; i < start + count; ++i)
  {
    delete m_fields[i];
  }
  m_fields.remove(start, count);

  endRemoveRows();

  updateFieldOffsets();
}

void StructDetailModel::updateFieldOffsets()
{
  int cur_offset = 0;
  for (int i = 0; i < m_fields.count(); ++i)
  {
    FieldDef* field = m_fields[i];
    if (field->getOffset() != cur_offset)
    {
      field->setOffset(cur_offset);
      QModelIndex index = createIndex(i, i, field);
      dataChanged(index, index);
    }
    cur_offset += field->getSize();
  }
}

bool StructDetailModel::hasStructLoaded() const
{
  return m_baseNode != nullptr;
}

StructTreeNode* StructDetailModel::getLoadedStructNode() const
{
  return m_baseNode;
}

void StructDetailModel::loadStruct(StructTreeNode* baseNode)
{
  beginResetModel();
  if (m_baseNode)
  {
    unloadStruct();
  }

  m_baseNode = baseNode;
  m_curSize = m_baseNode->getStructDef()->getLength();

  size_t cur_offset = 0;
  for (FieldDef* field : m_baseNode->getStructDef()->getFields())
  {
    while (cur_offset < field->getOffset())
    {
      m_fields.push_back(new FieldDef(cur_offset, 1, true));
      cur_offset += 1;
    }
    FieldDef* newField = new FieldDef(field);
    m_fields.push_back(newField);
    cur_offset = field->getOffset() + field->getSize();
  }
  while (cur_offset < m_curSize)
  {
    m_fields.push_back(new FieldDef(cur_offset, 1, true));
    cur_offset += 1;
  }
  endResetModel();
}

void StructDetailModel::saveStruct()
{
  QVector<FieldDef*> new_fields{};
  for (FieldDef* field : m_fields)
  {
    if (field->getEntry())
      new_fields.push_back(field);
  }
  m_baseNode->getStructDef()->setFields(new_fields);
}

void StructDetailModel::unloadStruct()
{
  m_baseNode = nullptr;
  qDeleteAll(m_fields);
  m_fields = QVector<FieldDef*>();
}

bool StructDetailModel::willRemoveFields(u32 newLength)
{
  for (int i = m_fields.count() - 1; i >= 0; --i)
  {
    if (m_fields[i]->getOffset() + m_fields[i]->getSize() <= newLength)
      break;
    if (m_fields[i]->isPadding())
      continue;
    return false;
  }
  return true;
}

QString StructDetailModel::getRemovedFieldDescriptions(u32 newLength)
{
  QVector<QString> field_strings;
  for (FieldDef* field : m_fields)
  {
    if (field->isPadding() || field->getOffset() + field->getSize() <= newLength)
      continue;
    QString f_string = QString("0x%1").arg(field->getOffset()) + " - " +
                       QString("0x%1").arg(field->getSize()) + " - " + getFieldDetails(field);
    field_strings.push_back(f_string);
  }
  return field_strings.join("\n");
}

void StructDetailModel::updateFieldsWithNewLength()
{
  u32 old_length = 0;
  if (m_fields.count() != 0)
    old_length = m_fields.last()->getOffset() + m_fields.last()->getSize();
  u32 new_length = m_baseNode->getStructDef()->getLength();

  if (old_length == new_length)
  {
    qDebug("How did we get here? On updateFields in StructDetailModel, old_length and new_length "
           "should never be equal!");
    return;
  }
  else if (old_length < new_length)
  {
    addPaddingFields(new_length - old_length);
  }
  else if (new_length == 0)
  {
    beginResetModel();
    qDeleteAll(m_fields);
    m_fields = QVector<FieldDef*>();
    endResetModel();
  }
  else
  {
    for (int i = m_fields.count() - 1; i >= 0; --i)
    {
      if (m_fields[i]->getOffset() + m_fields[i]->getSize() > new_length)
        continue;
      removeFields(i, m_fields.count() - i);
      break;
    }
    u32 cur_length = m_fields.last()->getOffset() + m_fields.last()->getSize();
    if (cur_length < new_length)
    {
      addPaddingFields(new_length - cur_length);
    }
  }
}

QString StructDetailModel::getFieldDetails(FieldDef* field) const
{
  if (field->isPadding())
    return QString("-- Padding --");

  std::stringstream ss;
  ss << GUICommon::getStringFromType(field->getEntry()->getType(), field->getEntry()->getLength())
            .toStdString();

  if (field->getEntry()->isBoundToPointer())
  {
    size_t pLevel = field->getEntry()->getPointerLevel();
    ss << " (" << pLevel << "*)";
  }

  if (GUICommon::isContainerType(field->getEntry()->getType()))
  {
    if (field->getEntry()->getType() == Common::MemType::type_struct)
    {
      ss << " <" << field->getEntry()->getStructName().toStdString() << ">";
    }
  }
  return QString::fromStdString(ss.str());
}

void StructDetailModel::removeFields(QModelIndexList indices)
{
  u32 start = indices[0].row();
  u32 count = indices.count();
  removeFields(start, count);
  m_baseNode->getStructDef()->setLength(m_fields.last()->getOffset() + m_fields.last()->getSize());
}

void StructDetailModel::removeLastField()
{
  removeFields(m_fields.count() - 1, 1);
  m_baseNode->getStructDef()->setLength(m_fields.last()->getOffset() + m_fields.last()->getSize());
}

void StructDetailModel::clearFields(QModelIndexList indices)
{
  int start = indices[0].row();
  int count = indices.count();

  for (int i = start + count; i > start; --i)
  {
    if (!m_fields[i]->isPadding())
    {
      int new_field_count = m_fields[i]->getSize() - 1;
      FieldDef* cur_field = m_fields[i];
      cur_field->convertToPadding();
      QModelIndex field_index = createIndex(i, 0, cur_field);
      emit dataChanged(field_index, field_index.siblingAtColumn(columnCount({}) - 1));
      if (0 < new_field_count)
        addPaddingFields(new_field_count, i + 1);
    }
  }
}

void StructDetailModel::updateFieldEntry(MemWatchEntry* entry, const QModelIndex& index)
{
  FieldDef* field = static_cast<FieldDef*>(index.internalPointer());
  field->setEntry(entry);
  emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(columnCount({}) - 1));
}

FieldDef* StructDetailModel::getFieldByRow(int row)
{
  if (row >= m_fields.count())
    return nullptr;
  return m_fields[row];
}

