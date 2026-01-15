#include "StructDetailModel.h"

#include <QApplication>
#include <QMimeData>
#include <sstream>

#include "../../Common/CommonUtils.h"
#include "../../Structs/StructTreeNode.h"
#include "../GUICommon.h"

StructDetailModel::StructDetailModel(QObject* parent) : QAbstractListModel(parent)
{
}

StructDetailModel::~StructDetailModel()
{
  delete m_baseNode;
  qDeleteAll(m_fields);
}

int StructDetailModel::columnCount(const QModelIndex& parent) const
{
  (void)parent;
  return STRUCT_COL_NUM;
}

int StructDetailModel::rowCount(const QModelIndex& parent) const
{
  (void)parent;
  return static_cast<int>(m_fields.count());
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
      return QString("0x%1").arg(m_fields[index.row()]->getOffset(), 0, 16);
    case STRUCT_COL_SIZE:
      return m_fields[index.row()]->getFieldSize();
    case STRUCT_COL_LABEL:
      if (m_fields[index.row()]->isPadding())
        return QString("-- Padding --");
      return m_fields[index.row()]->getEntry()->getLabel();
    case STRUCT_COL_TYPE:
    {
      return getFieldDetails(m_fields[index.row()]);
    }

    default:
      break;
    }
  }

  if (role == Qt::EditRole && index.column() == STRUCT_COL_LABEL)
    if (!m_fields[index.row()]->isPadding())
      return m_fields[index.row()]->getEntry()->getLabel();

  return {};
}

bool StructDetailModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (~role == Qt::EditRole || !index.isValid())
    return false;

  FieldDef* field = getFieldByRow(index.row());

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
    case STRUCT_COL_TYPE:
      return tr("Type");
    case STRUCT_COL_LABEL:
      return tr("Name");
    default:
      break;
    }
  }
  return {};
}

Qt::ItemFlags StructDetailModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  Qt::ItemFlags flags =
      Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

  if (index.column() == STRUCT_COL_LABEL && !m_fields[index.row()]->isPadding())
    flags |= Qt::ItemIsEditable;

  return flags;
}

Qt::DropActions StructDetailModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

Qt::DropActions StructDetailModel::supportedDragActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

QStringList StructDetailModel::mimeTypes() const
{
  return QStringList() << "application/x-structfield";
}

QMimeData* StructDetailModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData;
  QByteArray data;

  QDataStream stream(&data, QIODevice::WriteOnly);

  int startRow = indexes[0].row();
  int endRow = indexes.last().row();

  stream << static_cast<int>(indexes[0].row());
  stream << static_cast<int>(endRow - startRow + 1);

  mimeData->setData("application/x-structfield", data);
  return mimeData;
}

bool StructDetailModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                                     int column, const QModelIndex& parent)
{
  (void)column;

  if (action != Qt::CopyAction && action != Qt::MoveAction)
    return false;

  if (!data->hasFormat("application/x-structfield"))
    return false;

  if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    action = Qt::CopyAction;

  row = parent.row();

  QByteArray bytes = data->data("application/x-structfield");
  QDataStream stream(&bytes, QIODevice::ReadOnly);

  int firstRow;
  stream >> firstRow;

  int count;
  stream >> count;

  int unmovedRowCount = static_cast<int>(m_fields.count()) - count;

  if (row == -1)
    row = unmovedRowCount;

  beginResetModel();

  QList<FieldDef*> fields = m_fields.mid(firstRow, count);
  if (action == Qt::MoveAction)
  {
    m_fields.remove(firstRow, count);
    for (int i = count - 1; i >= 0; i--)
    {
      m_fields.insert(row, fields.takeAt(i));
    }
  }
  else
  {
    for (int i = count - 1; i >= 0; i--)
    {
      FieldDef* newField = new FieldDef(fields[i]);
      m_fields.insert(row, newField);
    }
  }

  endResetModel();

  updateFieldOffsets();

  return true;
}

void StructDetailModel::addField(const QModelIndex& index, FieldDef* field)
{
  int start = index.row();

  if (m_fields.isEmpty())
  {
    start = 0;
  }
  else if (start < 0 || start >= m_fields.count())
  {
    start = static_cast<int>(m_fields.count());
  }

  beginInsertRows(QModelIndex(), start, start);
  m_fields.insert(start, field);
  endInsertRows();

  updateFieldOffsets();
}

void StructDetailModel::addPaddingFields(int count, int start)
{
  if (m_fields.isEmpty())
  {
    start = 0;
  }
  else if (start < 0 || start >= m_fields.count())
  {
    start = static_cast<int>(m_fields.count());
  }

  beginInsertRows(QModelIndex(), start, start + count - 1);

  for (int i = 0; i < count; ++i)
  {
    m_fields.insert(start + i, new FieldDef(0, 1, true));
  }

  endInsertRows();

  updateFieldOffsets();
}

void StructDetailModel::removePaddingFields(int count, int start)
{
  if (start >= m_fields.count())
    return;

  int toRemove = 0;
  while (toRemove < count)
  {
    if (start + toRemove >= m_fields.count() || !m_fields[start + toRemove]->isPadding())
      break;
    ++toRemove;
  }

  if (toRemove > 0)
    removeFields(start, toRemove);
}

void StructDetailModel::removeFields(int start, int count)
{
  if (start == -1)
  {
    return;
  }
  beginRemoveRows(QModelIndex(), start, start + count - 1);

  for (int i = start; i < start + count; ++i)
  {
    if (m_fields[i]->getEntry() != nullptr &&
        m_fields[i]->getEntry()->getType() == Common::MemType::type_struct)
    {
      if (m_fields[i]->getEntry()->isBoundToPointer())
        emit modifyStructPointerReference(m_baseNode->getNameSpace(),
                                          m_fields[i]->getEntry()->getStructName(), false);
      else
      {
        bool _;
        emit modifyStructReference(m_baseNode->getNameSpace(),
                                   m_fields[i]->getEntry()->getStructName(), false, _);
      }
    }
    delete m_fields[i];
  }
  m_fields.remove(start, count);

  endRemoveRows();

  updateFieldOffsets();
}

void StructDetailModel::updateFieldOffsets()
{
  u32 cur_offset = 0;
  for (int i = 0; i < m_fields.count(); ++i)
  {
    FieldDef* field = m_fields[i];
    if (field->getOffset() != cur_offset)
    {
      field->setOffset(cur_offset);
      QModelIndex index = createIndex(i, i, field);
      dataChanged(index, index);
    }
    cur_offset += field->getFieldSize();
  }

  u32 newLength = 0;
  if (!m_fields.isEmpty())
    newLength = m_fields.last()->getOffset() + m_fields.last()->getFieldSize();

  if (m_baseNode->getStructDef()->getLength() != newLength)
  {
    m_baseNode->getStructDef()->setLength(newLength);
    emit lengthChanged(newLength);
  }
}

void StructDetailModel::reduceIndicesToRows(QModelIndexList& indices)
{
  QVector<int> rowsSeen{};
  int i = 0;
  while (i < indices.count())
  {
    if (rowsSeen.contains(indices[i].row()))
      indices.removeAt(i);
    else
    {
      rowsSeen.push_back(indices[i].row());
      ++i;
    }
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
  if (m_baseNode)
  {
    unloadStruct();
  }

  beginResetModel();
  m_baseNode = baseNode;
  m_curSize = m_baseNode->getStructDef()->getLength();

  u32 cur_offset = 0;
  for (FieldDef* field : m_baseNode->getStructDef()->getFields())
  {
    while (cur_offset < field->getOffset())
    {
      m_fields.push_back(new FieldDef(cur_offset, 1, true));
      cur_offset += 1;
    }
    FieldDef* newField = new FieldDef(field);
    m_fields.push_back(newField);
    cur_offset = field->getOffset() + field->getFieldSize();
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
      new_fields.push_back(new FieldDef(field));
  }
  m_baseNode->getStructDef()->setFields(new_fields);
}

void StructDetailModel::unloadStruct()
{
  beginResetModel();
  delete m_baseNode;
  m_baseNode = nullptr;
  qDeleteAll(m_fields);
  m_fields = QVector<FieldDef*>();
  endResetModel();
}

bool StructDetailModel::willRemoveFields(u32 newLength)
{
  for (int i = static_cast<int>(m_fields.count()) - 1; i >= 0; --i)
  {
    if (m_fields[i]->getOffset() + m_fields[i]->getFieldSize() <= newLength)
      break;
    if (!m_fields[i]->isPadding())
      return true;
  }
  return false;
}

QString StructDetailModel::getRemovedFieldDescriptions(u32 newLength)
{
  QVector<QString> field_strings;
  for (FieldDef* field : m_fields)
  {
    if (field->isPadding() || field->getOffset() + field->getFieldSize() <= newLength)
      continue;
    QString f_string = QString("0x%1").arg(field->getOffset()) + " - " +
                       QString("0x%1").arg(field->getFieldSize()) + " - " + getFieldDetails(field);
    field_strings.push_back(f_string);
  }
  return field_strings.join("\n");
}

void StructDetailModel::updateFieldsWithNewLength()
{
  u32 old_length = 0;
  if (m_fields.count() != 0)
    old_length = m_fields.last()->getOffset() + m_fields.last()->getFieldSize();
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
    int field_count = static_cast<int>(m_fields.count());
    for (int i = field_count - 1; i >= 0; --i)
    {
      if (m_fields[i]->getOffset() + m_fields[i]->getFieldSize() > new_length)
        continue;
      removeFields(i, field_count - i);
      break;
    }
    u32 cur_length = m_fields.last()->getOffset() + m_fields.last()->getFieldSize();
    if (cur_length < new_length)
    {
      addPaddingFields(new_length - cur_length);
    }
  }
}

void StructDetailModel::updateStructTypeLabel(QString oldName, QString newName)
{
  for (int i = 0; i < m_fields.count(); i++)
  {
    if (m_fields[i]->isPadding() ||
        m_fields[i]->getEntry()->getType() != Common::MemType::type_struct ||
        m_fields[i]->getEntry()->getStructName() != oldName)
    {
      continue;
    }
    m_fields[i]->getEntry()->setStructName(newName);
    dataChanged(createIndex(i, 0), createIndex(i, STRUCT_COL_NUM - 1));
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
  reduceIndicesToRows(indices);
  int start = indices[0].row();
  int count = static_cast<int>(indices.count());
  removeFields(start, count);
}

void StructDetailModel::removeLastField()
{
  removeFields(static_cast<int>(m_fields.count()) - 1, 1);
}

void StructDetailModel::clearFields(QModelIndexList indices)
{
  reduceIndicesToRows(indices);
  int start = indices[0].row();
  int count = static_cast<int>(indices.count());

  for (int i = start + count - 1; i >= start; --i)
  {
    if (!m_fields[i]->isPadding())
    {
      FieldDef* cur_field = m_fields[i];
      if (cur_field->getEntry()->getType() == Common::MemType::type_struct)
      {
        if (cur_field->getEntry()->isBoundToPointer())
          emit modifyStructPointerReference(m_baseNode->getNameSpace(),
                                            cur_field->getEntry()->getStructName(), false);
        else
        {
          bool _;
          emit modifyStructReference(m_baseNode->getNameSpace(),
                                     cur_field->getEntry()->getStructName(), false, _);
        }
      }

      if (cur_field->getFieldSize() == 0)
      {
        removeFields({createIndex(i, 0, cur_field)});
        continue;
      }

      u32 new_field_count = cur_field->getFieldSize() - 1;
      cur_field->convertToPadding();
      QModelIndex field_index = createIndex(i, 0, cur_field);
      emit dataChanged(field_index, field_index.siblingAtColumn(columnCount({}) - 1));
      if (0 < new_field_count)
        addPaddingFields(new_field_count, i + 1);
    }
  }
}

bool StructDetailModel::updateFieldEntry(MemWatchEntry* entry, const QModelIndex& index)
{
  FieldDef* field = getFieldByRow(index.row());

  u32 oldFieldLen = field->getFieldSize();
  MemWatchEntry* oldEntry = field->getEntry();

  if (oldEntry != nullptr && oldEntry->getType() == Common::MemType::type_struct)
  {
    if (oldEntry->isBoundToPointer())
      emit modifyStructPointerReference(m_baseNode->getNameSpace(), oldEntry->getStructName(),
                                        false);
    else
    {
      bool ok = true;
      emit modifyStructReference(m_baseNode->getNameSpace(), oldEntry->getStructName(), false, ok);
      if (!ok)
        return false;
    }
  }
  else if (oldEntry != nullptr && oldEntry->getType() == Common::MemType::type_array)
  {
    MemWatchEntry* oldContainerEntry = oldEntry->getContainerEntry();
    while (oldContainerEntry && oldContainerEntry->getType() == Common::MemType::type_array)
      oldContainerEntry = oldContainerEntry->getContainerEntry();

    if (oldContainerEntry != nullptr && oldEntry->getType() == Common::MemType::type_struct)
    {
      if (oldContainerEntry->isBoundToPointer())
        emit modifyStructPointerReference(m_baseNode->getNameSpace(),
                                          oldContainerEntry->getStructName(), false);
      else
      {
        bool ok = true;
        emit modifyStructReference(m_baseNode->getNameSpace(), oldContainerEntry->getStructName(),
                                   false, ok);
        if (!ok)
          return false;
      }
    }
  }

  u32 fieldLen = 0;
  if (entry->isBoundToPointer())
  {
    fieldLen = 4;
    if (entry->getType() == Common::MemType::type_struct)
    {
      emit modifyStructPointerReference(m_baseNode->getNameSpace(), entry->getStructName(), true);
    }
  }
  else if (entry->getType() == Common::MemType::type_struct)
  {
    if (entry->getStructName() == m_baseNode->getNameSpace())
    {
      return false;
    }
    else
    {
      fieldLen = m_baseNode->getParent()->getSizeOfStruct(entry->getStructName());
      bool ok = true;
      emit modifyStructReference(m_baseNode->getNameSpace(), entry->getStructName(), true, ok);
      if (!ok)
        return false;
    }
  }
  else if (entry->getType() == Common::MemType::type_array)
  {
    fieldLen = getTotalContainerLength(entry);

    MemWatchEntry* containerEntry = entry->getContainerEntry();
    while (containerEntry != nullptr && containerEntry->getType() == Common::MemType::type_array)
      containerEntry = containerEntry->getContainerEntry();

    if (containerEntry != nullptr && containerEntry->getType() == Common::MemType::type_struct)
    {
      if (containerEntry->getStructName() == m_baseNode->getNameSpace())
      {
        return false;
      }
      else
      {
        bool ok = true;
        emit modifyStructReference(m_baseNode->getNameSpace(), containerEntry->getStructName(),
                                   true, ok);
        if (!ok)
          return false;
      }
    }
  }
  else
    fieldLen = static_cast<u32>(Common::getSizeForType(entry->getType(), entry->getLength()));

  field->setEntry(entry);
  field->setFieldSize(fieldLen);
  emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(columnCount({}) - 1));

  if (oldFieldLen < fieldLen)
    removePaddingFields(fieldLen - oldFieldLen, index.row() + 1);

  if (oldFieldLen > fieldLen)
    addPaddingFields(oldFieldLen - fieldLen, index.row() + 1);

  updateFieldOffsets();

  return true;
}

int StructDetailModel::getTotalContainerLength(MemWatchEntry* entry)
{
  if (entry->isBoundToPointer())
    return 4;
  else if (entry->getType() != Common::MemType::type_array &&
           entry->getType() != Common::MemType::type_struct)
    return static_cast<int>(Common::getSizeForType(entry->getType(), entry->getLength()));
  else if (entry->getType() == Common::MemType::type_array && entry->getContainerEntry() != nullptr)
    return static_cast<int>(entry->getContainerCount() *
                            getTotalContainerLength(entry->getContainerEntry()));
  else if (entry->getType() == Common::MemType::type_struct)
  {
    int len = 0;
    emit getStructLength(entry->getStructName(), len);
    return len;
  }
  return 0;
}

FieldDef* StructDetailModel::getFieldByRow(int row)
{
  if (row < 0 || row >= m_fields.count())
    return nullptr;
  return m_fields[row];
}

QModelIndex StructDetailModel::getLastIndex(int col)
{
  return createIndex(static_cast<int>(m_fields.count()) - 1, col);
}

QModelIndex StructDetailModel::getIndexAt(int row, int col)
{
  return createIndex(row, col);
}
