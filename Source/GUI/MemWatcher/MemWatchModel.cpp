#include "MemWatchModel.h"

#include <QDataStream>
#include <QFontDatabase>
#include <QIcon>
#include <QMimeData>

#include <cassert>
#include <cstring>
#include <limits>
#include <sstream>

#include "../../CheatEngineParser/CheatEngineParser.h"
#include "../../Common/CommonUtils.h"
#include "../../DolphinProcess/DolphinAccessor.h"
#include "../GUICommon.h"

namespace
{
QString getAddressString(const MemWatchEntry* const entry)
{
  std::stringstream ss;
  if (entry->isBoundToPointer())
  {
    int level = static_cast<int>(entry->getPointerLevel());
    std::string strAddress = entry->getAddressStringForPointerLevel(level);
    ss << "(" << level << "*)" << std::hex << std::uppercase << strAddress;
    return QString::fromStdString(ss.str());
  }
  u32 address = entry->getConsoleAddress();
  ss << std::hex << std::uppercase << address;
  return QString::fromStdString(ss.str());
}
}  // namespace

MemWatchModel::MemWatchModel(QObject* parent) : QAbstractItemModel(parent)
{
  m_rootNode = new MemWatchTreeNode(nullptr);
}

MemWatchModel::~MemWatchModel()
{
  delete m_rootNode;
}

void MemWatchModel::onUpdateTimer()
{
  if (!updateNodeValueRecursive(m_rootNode))
    emit readFailed();
}

void MemWatchModel::onFreezeTimer()
{
  if (!freezeNodeValueRecursive(m_rootNode))
    emit writeFailed(QModelIndex(), Common::MemOperationReturnCode::operationFailed);
}

bool MemWatchModel::updateNodeValueRecursive(MemWatchTreeNode* node, const QModelIndex& parent,
                                             bool readSucess)
{
  QVector<MemWatchTreeNode*> children = node->getChildren();
  if (children.count() > 0)
  {
    for (MemWatchTreeNode* const child : children)
    {
      QModelIndex theIndex = index(child->getRow(), WATCH_COL_VALUE, parent);
      readSucess = updateNodeValueRecursive(child, theIndex, readSucess);
      if (!readSucess)
        return false;
      if (!GUICommon::g_valueEditing && !child->isGroup())
        emit dataChanged(theIndex, theIndex);
    }
  }

  MemWatchEntry* entry = node->getEntry();
  if (entry != nullptr)
    if (entry->readMemoryFromRAM() == Common::MemOperationReturnCode::operationFailed)
      return false;
  return true;
}

bool MemWatchModel::freezeNodeValueRecursive(MemWatchTreeNode* node, const QModelIndex& parent,
                                             bool writeSucess)
{
  QVector<MemWatchTreeNode*> children = node->getChildren();
  if (children.count() > 0)
  {
    for (MemWatchTreeNode* const child : children)
    {
      QModelIndex theIndex = index(child->getRow(), WATCH_COL_VALUE, parent);
      writeSucess = freezeNodeValueRecursive(child, theIndex, writeSucess);
      if (!writeSucess)
        return false;
    }
  }

  MemWatchEntry* entry = node->getEntry();
  if (entry != nullptr)
  {
    if (entry->isLocked())
    {
      Common::MemOperationReturnCode writeReturn = entry->freeze();
      // Here we want to not care about invalid pointers, it won't write anyway
      if (writeReturn == Common::MemOperationReturnCode::OK ||
          writeReturn == Common::MemOperationReturnCode::invalidPointer)
        return true;
    }
  }
  return true;
}

void MemWatchModel::changeType(const QModelIndex& index, Common::MemType type, size_t length)
{
  MemWatchEntry* entry = getEntryFromIndex(index);
  entry->setTypeAndLength(type, length);
  emit dataChanged(index, index);
}

MemWatchEntry* MemWatchModel::getEntryFromIndex(const QModelIndex& index)
{
  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  return node->getEntry();
}

void MemWatchModel::addNodes(const std::vector<MemWatchTreeNode*>& nodes,
                             const QModelIndex& referenceIndex)
{
  if (nodes.empty())
    return;

  QModelIndex targetIndex;
  MemWatchTreeNode* parentNode{};
  int rowIndex{};

  if (referenceIndex.isValid())
  {
    parentNode = static_cast<MemWatchTreeNode*>(referenceIndex.internalPointer());
    if (parentNode->isGroup())
    {
      targetIndex = referenceIndex.siblingAtColumn(0);
      rowIndex = parentNode->childrenCount();
    }
    else
    {
      parentNode = parentNode->getParent();
      targetIndex = referenceIndex.parent();
      rowIndex = parentNode->hasChildren() ? referenceIndex.row() + 1 : 0;
    }
  }
  else
  {
    targetIndex = QModelIndex{};
    parentNode = m_rootNode;
    rowIndex = parentNode->childrenCount();
  }

  const int first{rowIndex};
  const int last{rowIndex + static_cast<int>(nodes.size()) - 1};

  beginInsertRows(targetIndex, first, last);
  for (int i{first}; i <= last; ++i)
  {
    parentNode->insertChild(i, nodes[static_cast<size_t>(i - first)]);
  }
  endInsertRows();
}

void MemWatchModel::addGroup(const QString& name, const QModelIndex& referenceIndex)
{
  addNodes({new MemWatchTreeNode(nullptr, m_rootNode, true, name)}, referenceIndex);
}

void MemWatchModel::addEntry(MemWatchEntry* const entry, const QModelIndex& referenceIndex)
{
  addNodes({new MemWatchTreeNode(entry)}, referenceIndex);
}

void MemWatchModel::editEntry(MemWatchEntry* entry, const QModelIndex& index)
{
  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  node->setEntry(entry);
  emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(columnCount({}) - 1));
}

void MemWatchModel::clearRoot()
{
  beginResetModel();
  m_rootNode->deleteChildren();
  endResetModel();
}

void MemWatchModel::deleteNode(const QModelIndex& index)
{
  if (index.isValid())
  {
    MemWatchTreeNode* toDelete = static_cast<MemWatchTreeNode*>(index.internalPointer());

    int toDeleteRow = toDelete->getRow();

    beginRemoveRows(index.parent(), toDeleteRow, toDeleteRow);
    bool removeChildren = (toDelete->isGroup() && toDelete->hasChildren());
    if (removeChildren)
      beginRemoveRows(index, 0, toDelete->childrenCount());
    toDelete->getParent()->removeChild(toDeleteRow);
    delete toDelete;
    if (removeChildren)
      endRemoveRows();
    endRemoveRows();
  }
}

void MemWatchModel::groupSelection(const QModelIndexList& indexes)
{
  if (indexes.isEmpty())
    return;

  // Collect nodes from indexes; indexes will be invalidated shortly as nodes are removed from their
  // parents.
  std::vector<MemWatchTreeNode*> nodes;
  for (const QModelIndex& index : indexes)
  {
    nodes.push_back(static_cast<MemWatchTreeNode*>(index.internalPointer()));
  }

  MemWatchTreeNode* newParent{};
  int newRow{std::numeric_limits<int>::max()};

  // Extract nodes from their current parent.
  for (MemWatchTreeNode* const node : nodes)
  {
    MemWatchTreeNode* const parent{node->getParent()};
    const int row{static_cast<int>(parent->getChildren().indexOf(node))};

    if (!newParent || newParent == parent)
    {
      newParent = parent;
      newRow = std::min(newRow, row);
    }

    beginRemoveRows(getIndexFromTreeNode(parent), row, row);
    parent->removeChild(node);
    endRemoveRows();
  }

  beginInsertRows(getIndexFromTreeNode(newParent), newRow, newRow);

  // Create new group with all the extracted nodes, and insert in the new parent.
  MemWatchTreeNode* const group{new MemWatchTreeNode(nullptr, nullptr, true, "New Group")};
  for (MemWatchTreeNode* const node : nodes)
  {
    group->appendChild(node);
  }
  newParent->insertChild(newRow, group);

  endInsertRows();
}

int MemWatchModel::columnCount(const QModelIndex& parent) const
{
  (void)parent;

  return WATCH_COL_NUM;
}

int MemWatchModel::rowCount(const QModelIndex& parent) const
{
  MemWatchTreeNode* parentItem;
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid())
    parentItem = m_rootNode;
  else
    parentItem = static_cast<MemWatchTreeNode*>(parent.internalPointer());

  return static_cast<int>(parentItem->getChildren().size());
}

QVariant MemWatchModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return {};

  const int column{index.column()};

  MemWatchTreeNode* item = static_cast<MemWatchTreeNode*>(index.internalPointer());

  if (!item->isGroup())
  {
    if (role == Qt::FontRole)
    {
      if (column == WATCH_COL_ADDRESS || column == WATCH_COL_VALUE)
      {
        static const QFont s_fixedFont{
            QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont)};
        return s_fixedFont;
      }
    }

    if (role == Qt::EditRole && index.column() == WATCH_COL_TYPE)
      return {static_cast<int>(item->getEntry()->getType())};

    MemWatchEntry* entry = item->getEntry();
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
      switch (index.column())
      {
      case WATCH_COL_LABEL:
      {
        return entry->getLabel();
      }
      case WATCH_COL_TYPE:
      {
        Common::MemType type = entry->getType();
        size_t length = entry->getLength();
        return GUICommon::getStringFromType(type, length);
      }
      case WATCH_COL_ADDRESS:
      {
        return getAddressString(entry);
      }
      case WATCH_COL_VALUE:
      {
        return QString::fromStdString(entry->getStringFromMemory());
      }
      default:
        break;
      }
    }
  }
  else
  {
    if (index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
      return item->getGroupName();
    if (index.column() == 0 && role == Qt::DecorationRole)
    {
      static const QIcon s_folderIcon(":/folder.svg");
      static const QIcon s_emptyFolderIcon(":/folder_empty.svg");
      return item->hasChildren() ? s_folderIcon : s_emptyFolderIcon;
    }
  }
  return {};
}

bool MemWatchModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  return editData(index, value, role, true);
}

bool MemWatchModel::editData(const QModelIndex& index, const QVariant& value, const int role,
                             const bool emitEdit)
{
  if (!index.isValid())
    return false;

  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  if (node == m_rootNode)
    return false;

  if (!node->isGroup())
  {
    MemWatchEntry* entry = node->getEntry();
    if (role == Qt::EditRole)
    {
      switch (index.column())
      {
      case WATCH_COL_LABEL:
      {
        entry->setLabel(value.toString());
        emit dataChanged(index, index);
        if (emitEdit)
          emit dataEdited(index, value, role);
        return true;
      }
      case WATCH_COL_VALUE:
      {
        Common::MemOperationReturnCode returnCode =
            entry->writeMemoryFromString((value.toString().toStdString()));
        if (returnCode != Common::MemOperationReturnCode::OK)
        {
          emit writeFailed(index, returnCode);
          return false;
        }
        emit dataChanged(index, index);
        if (emitEdit)
          emit dataEdited(index, value, role);
        return true;
      }
      default:
      {
        return false;
      }
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    node->setGroupName(value.toString());
    return true;
  }
}

Qt::ItemFlags MemWatchModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  if (node == m_rootNode)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  // These flags are common to every node
  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;

  if (node->isGroup())
  {
    flags |= Qt::ItemIsDropEnabled;
    if (index.column() == WATCH_COL_LABEL)
      flags |= Qt::ItemIsEditable;
    return flags;
  }

  if (index.column() == WATCH_COL_LOCK)
    return flags;

  if (index.column() == WATCH_COL_LABEL)
  {
    flags |= Qt::ItemIsEditable;
  }
  else if (index.column() == WATCH_COL_VALUE)
  {
    const bool hooked{DolphinComm::DolphinAccessor::getStatus() ==
                      DolphinComm::DolphinAccessor::DolphinStatus::hooked};
    const Qt::ItemFlag itemIsEditable{hooked ? Qt::ItemIsEditable : Qt::NoItemFlags};
    flags |= itemIsEditable;
  }

  return flags;
}

QModelIndex MemWatchModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!hasIndex(row, column, parent))
    return {};

  MemWatchTreeNode* parentNode;

  if (!parent.isValid())
    parentNode = m_rootNode;
  else
    parentNode = static_cast<MemWatchTreeNode*>(parent.internalPointer());

  MemWatchTreeNode* childNode = parentNode->getChildren().at(row);
  if (childNode)
    return createIndex(row, column, childNode);

  return {};
}

QModelIndex MemWatchModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return {};

  MemWatchTreeNode* childNode = static_cast<MemWatchTreeNode*>(index.internalPointer());
  MemWatchTreeNode* parentNode = childNode->getParent();

  if (parentNode == m_rootNode || parentNode == nullptr)
    return {};

  return createIndex(parentNode->getRow(), 0, parentNode);
}

QVariant MemWatchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case WATCH_COL_LABEL:
      return tr("Name");
    case WATCH_COL_TYPE:
      return tr("Type");
    case WATCH_COL_ADDRESS:
      return tr("Address");
    case WATCH_COL_VALUE:
      return tr("Value");
    case WATCH_COL_LOCK:
      return tr("Lock");
    default:
      break;
    }
  }
  return {};
}

Qt::DropActions MemWatchModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

Qt::DropActions MemWatchModel::supportedDragActions() const
{
  return Qt::MoveAction;
}

QStringList MemWatchModel::mimeTypes() const
{
  return QStringList() << "application/x-memwatchtreenode";
}

int MemWatchModel::getNodeDeepness(const MemWatchTreeNode* node) const
{
  if (node == m_rootNode)
    return 0;
  if (node->getParent() == m_rootNode)
    return 1;

  return getNodeDeepness(node->getParent()) + 1;
}

MemWatchTreeNode*
MemWatchModel::getLeastDeepNodeFromList(const QList<MemWatchTreeNode*>& nodes) const
{
  int leastLevelFound = std::numeric_limits<int>::max();
  MemWatchTreeNode* returnNode = new MemWatchTreeNode(nullptr);
  for (MemWatchTreeNode* const node : nodes)
  {
    int deepness = getNodeDeepness(node);
    if (deepness < leastLevelFound)
    {
      returnNode = node;
      leastLevelFound = deepness;
    }
  }
  return returnNode;
}

QMimeData* MemWatchModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData;
  QByteArray data;

  QDataStream stream(&data, QIODevice::WriteOnly);
  QList<MemWatchTreeNode*> nodes;

  foreach (const QModelIndex& index, indexes)
  {
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
    if (!nodes.contains(node))
      nodes << node;
  }
  MemWatchTreeNode* leastDeepNode = getLeastDeepNodeFromList(nodes);
  const qulonglong leastDeepPointer{Common::bit_cast<qulonglong, MemWatchTreeNode*>(leastDeepNode)};
  stream << leastDeepPointer;
  stream << static_cast<int>(nodes.count());
  foreach (MemWatchTreeNode* node, nodes)
  {
    const auto pointer{Common::bit_cast<qulonglong, MemWatchTreeNode*>(node)};
    stream << pointer;
  }
  mimeData->setData("application/x-memwatchtreenode", data);
  return mimeData;
}

bool MemWatchModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                                 const QModelIndex& parent)
{
  (void)column;

  if (action != Qt::CopyAction && action != Qt::MoveAction)
    return false;

  if (!data->hasFormat("application/x-memwatchtreenode"))
    return false;

  QByteArray bytes = data->data("application/x-memwatchtreenode");
  QDataStream stream(&bytes, QIODevice::ReadOnly);
  MemWatchTreeNode* destParentNode = nullptr;
  if (!parent.isValid())
    destParentNode = m_rootNode;
  else
    destParentNode = static_cast<MemWatchTreeNode*>(parent.internalPointer());

  qulonglong leastDeepNodePtr{};
  stream >> leastDeepNodePtr;
  auto* const leastDeepNode{Common::bit_cast<MemWatchTreeNode*, qulonglong>(leastDeepNodePtr)};

  if (row == -1)
  {
    if (parent.isValid() && destParentNode->isGroup())
      row = rowCount(parent);
    else if (!parent.isValid())
      return false;
  }

  // beginMoveRows will cause a segfault if it ends up with doing nothing (so moving one row to the
  // same place), but there's also a discrepancy of 1 in the row received / the row given to
  // beginMoveRows and the actuall row of the node.  This discrepancy has to be reversed before
  // checking if we are trying to move the source (using the least deep one to accomodate
  // multi-select) to the same place.
  int trueRow = (leastDeepNode->getRow() < row) ? (row - 1) : (row);
  if (destParentNode == leastDeepNode->getParent() && leastDeepNode->getRow() == trueRow)
    return false;

  int count;
  stream >> count;

  for (int i = 0; i < count; ++i)
  {
    qulonglong nodePtr{};
    stream >> nodePtr;
    auto* const srcNode{Common::bit_cast<MemWatchTreeNode*, qulonglong>(nodePtr)};

    // Since beginMoveRows uses the same row format then the one received, we want to keep that, but
    // still use the correct row number for inserting.
    int destMoveRow = row;
    if (srcNode->getRow() < row && destParentNode == srcNode->getParent())
      --row;

    const int srcNodeRow = srcNode->getRow();
    const QModelIndex idx = createIndex(srcNodeRow, 0, srcNode);

    // A move is imperative here to not have the view collapse the source on its own.
    beginMoveRows(idx.parent(), srcNodeRow, srcNodeRow, parent, destMoveRow);
    srcNode->getParent()->removeChild(srcNodeRow);
    destParentNode->insertChild(row, srcNode);
    endMoveRows();

    ++row;
  }
  emit dropSucceeded();
  return true;
}

void MemWatchModel::loadRootFromJsonRecursive(const QJsonObject& json)
{
  beginResetModel();
  m_rootNode->readFromJson(json);
  endResetModel();
}

MemWatchModel::CTParsingErrors MemWatchModel::importRootFromCTFile(QFile* const CTFile,
                                                                   const bool useDolphinPointer,
                                                                   const u32 CEStart)
{
  CheatEngineParser parser = CheatEngineParser();
  parser.setTableStartAddress(CEStart);
  MemWatchTreeNode* importedRoot = parser.parseCTFile(CTFile, useDolphinPointer);
  if (importedRoot != nullptr)
  {
    beginResetModel();
    delete m_rootNode;
    m_rootNode = importedRoot;
    endResetModel();
  }

  return {parser.getErrorMessages(), parser.hasACriticalErrorOccured()};
}

void MemWatchModel::writeRootToJsonRecursive(QJsonObject& json) const
{
  m_rootNode->writeToJson(json);
}

QString MemWatchModel::writeRootToCSVStringRecursive() const
{
  QString csv = QString("Name;Address[offsets] (in hex);Type\n");
  csv.append(m_rootNode->writeAsCSV());
  if (csv.endsWith("\n"))
    csv.remove(csv.length() - 1, 1);
  return csv;
}

bool MemWatchModel::hasAnyNodes() const
{
  return m_rootNode->hasChildren();
}

MemWatchTreeNode* MemWatchModel::getRootNode() const
{
  return m_rootNode;
}

MemWatchTreeNode* MemWatchModel::getTreeNodeFromIndex(const QModelIndex& index)
{
  return static_cast<MemWatchTreeNode*>(index.internalPointer());
}

QModelIndex MemWatchModel::getIndexFromTreeNode(const MemWatchTreeNode* const node)
{
  if (node == m_rootNode)
  {
    return createIndex(0, 0, m_rootNode);
  }

  const MemWatchTreeNode* const parent{node->getParent()};
  return index(static_cast<int>(parent->getChildren().indexOf(node)), 0,
               getIndexFromTreeNode(parent));
}
