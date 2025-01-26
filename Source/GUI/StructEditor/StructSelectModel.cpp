#include "StructSelectModel.h"

#include <QIcon>
#include <QMimeData>
#include <QDataStream>
#include <QIODevice>

#include "../../Common/CommonUtils.h"

StructSelectModel::StructSelectModel(QObject* parent, StructTreeNode* rootNode)
    : QAbstractItemModel(parent)
{
  m_rootNode = rootNode;
}

StructSelectModel::~StructSelectModel()
{
  delete m_rootNode;
}

int StructSelectModel::columnCount(const QModelIndex& parent) const
{
  (void) parent;

  return WATCH_COL_NUM;
}

int StructSelectModel::rowCount(const QModelIndex& parent) const
{
  if (parent.column() > 0)
    return 0;

  StructTreeNode* parentItem;
  if (!parent.isValid())
    parentItem = m_rootNode;
  else
    parentItem = static_cast<StructTreeNode*>(parent.internalPointer());

  return static_cast<int>(parentItem->getChildren().size());
}

QVariant StructSelectModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return {};

  StructTreeNode* item = static_cast<StructTreeNode*>(index.internalPointer());

  if (!item->isGroup())
  {

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
      switch (index.column())
      {
      case WATCH_COL_LABEL:
      {
        return item->getName();
      }
      default:
        break;
      }
    }
  }
  else
  {
    if (index.column() == WATCH_COL_LABEL && (role == Qt::DisplayRole || role == Qt::EditRole))
      return item->getName();
    if (index.column() == WATCH_COL_LABEL && role == Qt::DecorationRole)
    {
      static const QIcon s_folderIcon(":/folder.svg");
      static const QIcon s_emptyFolderIcon(":/folder_empty.svg");
      return item->hasChildren() ? s_folderIcon : s_emptyFolderIcon;
    }
  }
  return {};
}

QModelIndex StructSelectModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!hasIndex(row, column, parent))
    return {};

  StructTreeNode* parentNode;

  if (!parent.isValid())
    parentNode = m_rootNode;
  else
    parentNode = static_cast<StructTreeNode*>(parent.internalPointer());

  StructTreeNode* childNode = parentNode->getChildren().at(row);
  if (childNode)
    return createIndex(row, column, childNode);

  return {};
}

QModelIndex StructSelectModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return {};

  StructTreeNode* childNode = static_cast<StructTreeNode*>(index.internalPointer());
  StructTreeNode* parentNode = childNode->getParent();

  if (parentNode == m_rootNode || parentNode == nullptr)
    return {};

  return createIndex(parentNode->getRow(), 0, parentNode);
}

QVariant StructSelectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case WATCH_COL_LABEL:
      return tr("Name");
    default:
      break;
    }
  }
  return {};
}

bool StructSelectModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role == Qt::EditRole)
  {
    QString newName = value.toString();
    if (newName.isEmpty())
      return false;

    StructTreeNode* node = static_cast<StructTreeNode*>(index.internalPointer());
    if (node->getParent()->isNameAvailable(newName))
    {
      node->setName(newName);
    }
    else
    {
      emit nameChangeFailed(node, newName);
      return false;
    }
    emit dataChanged(index, index);
    emit dataEdited(index, value, role);
    return true;
  }

  return false;
}

Qt::ItemFlags StructSelectModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  StructTreeNode* node = static_cast<StructTreeNode*>(index.internalPointer());
  if (node == m_rootNode)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  // These flags are common to every node
  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;

  if (node->isGroup())
  {
    flags |= Qt::ItemIsDropEnabled;
  }

  if (index.column() == WATCH_COL_LABEL)
  {
    flags |= Qt::ItemIsEditable;
  }

  return flags;
}

Qt::DropActions StructSelectModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

Qt::DropActions StructSelectModel::supportedDragActions() const
{
  return Qt::MoveAction;
}

QStringList StructSelectModel::mimeTypes() const
{
  return QStringList() << "application/x-structtreenode";
}

QMimeData* StructSelectModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData;
  QByteArray data;

  QDataStream stream(&data, QIODevice::WriteOnly);
  QList<StructTreeNode*> nodes;

  foreach (const QModelIndex& index, indexes)
  {
    StructTreeNode* node = static_cast<StructTreeNode*>(index.internalPointer());
    if (!nodes.contains(node))
      nodes << node;
  }
  StructTreeNode* leastDeepNode = getLeastDeepNodeFromList(nodes);
  const qulonglong leastDeepPointer{Common::bit_cast<qulonglong, StructTreeNode*>(leastDeepNode)};
  stream << leastDeepPointer;
  stream << static_cast<int>(nodes.count());
  foreach (StructTreeNode* node, nodes)
  {
    const auto pointer{Common::bit_cast<qulonglong, StructTreeNode*>(node)};
    stream << pointer;
  }
  mimeData->setData("application/x-structtreenode", data);
  return mimeData;
}

StructTreeNode*
StructSelectModel::getLeastDeepNodeFromList(const QList<StructTreeNode*>& nodes) const
{
  int leastLevelFound = std::numeric_limits<int>::max();
  StructTreeNode* returnNode = new StructTreeNode(nullptr, nullptr);
  for (StructTreeNode* const node : nodes)
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

int StructSelectModel::getNodeDeepness(const StructTreeNode* node) const
{
  if (node == m_rootNode)
    return 0;
  if (node->getParent() == m_rootNode)
    return 1;

  return getNodeDeepness(node->getParent()) + 1;
}

bool StructSelectModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                                     int column,
                                 const QModelIndex& parent)
{
  (void)column;

  if (action != Qt::CopyAction && action != Qt::MoveAction)
    return false;

  if (!data->hasFormat("application/x-structtreenode"))
    return false;

  QByteArray bytes = data->data("application/x-structtreenode");
  QDataStream stream(&bytes, QIODevice::ReadOnly);
  StructTreeNode* destParentNode = nullptr;
  if (!parent.isValid())
    destParentNode = m_rootNode;
  else
    destParentNode = static_cast<StructTreeNode*>(parent.internalPointer());

  qulonglong leastDeepNodePtr{};
  stream >> leastDeepNodePtr;
  auto* const leastDeepNode{Common::bit_cast<StructTreeNode*, qulonglong>(leastDeepNodePtr)};

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
    auto* const srcNode{Common::bit_cast<StructTreeNode*, qulonglong>(nodePtr)};

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

void StructSelectModel::addNodes(const std::vector<StructTreeNode*>& nodes,
                                 const QModelIndex& referenceIndex)
{
  if (nodes.empty())
    return;

  QModelIndex targetIndex;
  StructTreeNode* parentNode{};
  int rowIndex{};

  if (referenceIndex.isValid())
  {
    parentNode = static_cast<StructTreeNode*>(referenceIndex.internalPointer());
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

void StructSelectModel::addGroup(const QString& name, const QModelIndex& referenceIndex)
{
  addNodes({new StructTreeNode(NULL, m_rootNode, true, name)}, referenceIndex);
}

StructTreeNode* StructSelectModel::addStruct(const QString& name, const QModelIndex& referenceIndex)
{
  StructTreeNode* newNode = new StructTreeNode(new StructDef(name), m_rootNode, false, name);
  addNodes({newNode}, referenceIndex);
  return newNode;
}

void StructSelectModel::deleteNode(const QModelIndex& index)
{
  if (index.isValid())
  {
    StructTreeNode* toDelete = static_cast<StructTreeNode*>(index.internalPointer());

    int toDeleteRow = toDelete->getRow();

    beginRemoveRows(index.parent(), toDeleteRow, toDeleteRow);
    bool removeChildren = (toDelete->isGroup() && toDelete->hasChildren());
    if (removeChildren)
      beginRemoveRows(index, 0, toDelete->childrenCount() - 1);
    toDelete->getParent()->removeChild(toDeleteRow);
    delete toDelete;
    if (removeChildren)
      endRemoveRows();
    endRemoveRows();
  }
}

void StructSelectModel::setNodeLabel(StructTreeNode* node, const QString name)
{
  node->setName(name);
  QModelIndex index = getIndexFromTreeNode(node);
  dataChanged(index, index);
}

StructTreeNode* StructSelectModel::getTreeNodeFromIndex(const QModelIndex& index)
{
  return static_cast<StructTreeNode*>(index.internalPointer());
}

QModelIndex StructSelectModel::getIndexFromTreeNode(const StructTreeNode* node)
{
  if (node == m_rootNode)
    return createIndex(0, 0, m_rootNode);

  const StructTreeNode* const parent{node->getParent()};
  return index(static_cast<int>(parent->getChildren().indexOf(node)), 0,
               getIndexFromTreeNode(parent));
}

QMap<QString, StructDef*> StructSelectModel::getStructMap()
{
  QMap<QString, StructDef*> structMap = {};
  StructTreeNode* curNode = m_rootNode;
  QVector<StructTreeNode*> queue = {curNode->getChildren()};

  while (!queue.isEmpty())
  {
    curNode = queue.takeFirst();
    if (curNode->isGroup())
      for (int i = curNode->getChildren().count() - 1; i >= 0; --i)
        queue.push_front(curNode->getChildren()[i]);
    else
      structMap.insert(curNode->getNameSpace(), curNode->getStructDef());
  }

  return structMap;
}
