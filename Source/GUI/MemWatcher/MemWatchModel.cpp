#include "MemWatchModel.h"

#include <QDataStream>
#include <QFontDatabase>
#include <QIcon>
#include <QJsonArray>
#include <QMimeData>

#include <cassert>
#include <cstring>
#include <limits>
#include <sstream>

#include "../../CheatEngineParser/CheatEngineParser.h"
#include "../../Common/CommonUtils.h"
#include "../../DolphinProcess/DolphinAccessor.h"
#include "../GUICommon.h"
#include "../Settings/SConfig.h"

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
  m_placeholderEntry = new MemWatchEntry();
  m_placeholderEntry->setTypeAndLength(
      Common::MemType::type_none,
      Common::getSizeForType(Common::MemType::type_none, m_placeholderEntry->getLength()));
  m_structDefMap = QMap<QString, StructDef*>();
  m_structNodes = QMap<QString, QVector<MemWatchTreeNode*>>();
}

MemWatchModel::~MemWatchModel()
{
  delete m_rootNode;
  delete m_placeholderEntry;
  m_structDefMap.clear();
  m_structNodes.clear();
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
  if (!node->isGroup() && node->getEntry() != nullptr &&
      GUICommon::isContainerType(node->getEntry()->getType()) &&
      node->getEntry()->hasAddressChanged())
    updateContainerAddresses(node);

  QVector<MemWatchTreeNode*> children = node->getChildren();
  for (MemWatchTreeNode* const child : children)
  {
    // Don't bother update children that aren't visible
    if (child->isGroup() && !child->isExpanded())
      continue;

    QModelIndex theIndex = index(child->getRow(), WATCH_COL_VALUE, parent);
    readSucess = updateNodeValueRecursive(child, theIndex, readSucess);
    if (!readSucess)
      return false;
    if (!GUICommon::g_valueEditing && !child->isGroup())
      emit dataChanged(theIndex, theIndex);
  }

  MemWatchEntry* entry = node->getEntry();
  if (entry != nullptr && entry->getType() != Common::MemType::type_none)
    if (entry->readMemoryFromRAM() == Common::MemOperationReturnCode::operationFailed)
      return false;
  return true;
}

bool MemWatchModel::freezeNodeValueRecursive(MemWatchTreeNode* node, const QModelIndex& parent,
                                             bool writeSucess)
{
  QVector<MemWatchTreeNode*> children = node->getChildren();
  for (MemWatchTreeNode* const child : children)
  {
    QModelIndex theIndex = index(child->getRow(), WATCH_COL_VALUE, parent);
    writeSucess = freezeNodeValueRecursive(child, theIndex, writeSucess);
    if (!writeSucess)
      return false;
  }

  MemWatchEntry* entry = node->getEntry();
  if (entry != nullptr && entry->isLocked())
  {
    Common::MemOperationReturnCode writeReturn = entry->freeze();
    // Here we want to not care about invalid pointers, it won't write anyway
    if (writeReturn == Common::MemOperationReturnCode::OK ||
        writeReturn == Common::MemOperationReturnCode::invalidPointer)
      return true;
  }
  return true;
}

void MemWatchModel::changeType(const QModelIndex& index, Common::MemType type, size_t length)
{
  MemWatchEntry* entry = getEntryFromIndex(index);
  entry->setTypeAndLength(type, length);
  if (GUICommon::isContainerType(entry->getType()))
  {
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
    if (entry->getType() == Common::MemType::type_struct)
      setupStructNode(node);
    else if (entry->getType() == Common::MemType::type_array)
      setupArrayNode(node);
  }
  emit dataChanged(index, index);
}

MemWatchEntry* MemWatchModel::getEntryFromIndex(const QModelIndex& index)
{
  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  if (node)
    return node->getEntry();
  return nullptr;
}

void MemWatchModel::addNodes(const std::vector<MemWatchTreeNode*>& nodes,
                             const QModelIndex& referenceIndex, const bool insertInContainer)
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
    else if (insertInContainer && GUICommon::isContainerType(parentNode->getEntry()->getType()))
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

  for (size_t i = 0; i < nodes.size(); i++)
  {
    MemWatchTreeNode* node = nodes[i];
    if (!node->isGroup())
    {
      if (node->getEntry()->getType() == Common::MemType::type_struct)
        setupStructNode(node);
      else if (node->getEntry()->getType() == Common::MemType::type_array)
        setupArrayNode(node);
    }
  }
}

void MemWatchModel::addGroup(const QString& name, const QModelIndex& referenceIndex)
{
  addNodes({new MemWatchTreeNode(nullptr, m_rootNode, true, name)}, referenceIndex);
}

void MemWatchModel::addEntry(MemWatchEntry* const entry, const QModelIndex& referenceIndex)
{
  MemWatchTreeNode* node = new MemWatchTreeNode(entry);
  addNodes({node}, referenceIndex);

  // Check if entry is a container: set isGroup to true, add a placeholder node as a child, make
  // sure it is not expanded
  if (!GUICommon::isContainerType(entry->getType()))
    return;

  node->setExpanded(false);
}

void MemWatchModel::editEntry(MemWatchEntry* entry, const QModelIndex& index)
{
  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  MemWatchEntry* oldEntry = node->getEntry();

  // Check if entry has children to delete, create tree of expansion, delete all nodes
  if (GUICommon::isContainerType(oldEntry->getType()))
  {
    collapseContainerNode(node);
  }

  node->setEntry(entry);
  emit dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(columnCount({}) - 1));

  if (GUICommon::isContainerType(entry->getType()))
  {
    if (!GUICommon::isContainerType(oldEntry->getType()))
    {
      if (entry->getType() == Common::MemType::type_struct)
        setupStructNode(node);
      else if (entry->getType() == Common::MemType::type_array)
        setupArrayNode(node);
    }
    else if (node->isExpanded())
    {
      expandContainerNode(node);
    }
  }
  else if (GUICommon::isContainerType(oldEntry->getType()))
  {
    for (MemWatchTreeNode* child : node->getChildren())
      deleteNode(getIndexFromTreeNode(child));
  }
}

void MemWatchModel::clearRoot()
{
  beginResetModel();
  m_rootNode->deleteChildren();
  endResetModel();
  m_structNodes.clear();
}

void MemWatchModel::deleteNode(const QModelIndex& index)
{
  if (index.isValid())
  {
    MemWatchTreeNode* toDelete = static_cast<MemWatchTreeNode*>(index.internalPointer());
    removeNodeFromStructNodeMap(toDelete);

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

    MemWatchEntry* entry = item->getEntry();
    if (role == Qt::EditRole && index.column() == WATCH_COL_TYPE)
      return {static_cast<int>(entry->getType())};

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
      if (entry->getType() == Common::MemType::type_none)
        return QString();

      switch (index.column())
      {
      case WATCH_COL_LABEL:
      {
        return entry->getLabel();
      }
      case WATCH_COL_TYPE:
      {
        Common::MemType type = entry->getType();
        if (type == Common::MemType::type_struct && !entry->getStructName().isEmpty())
          return entry->getStructName();
        size_t length = entry->getLength();
        if (type == Common::MemType::type_array && entry->getContainerEntry() != nullptr)
        {
          std::stringstream prefix = {};
          std::stringstream suffix = {};
          prefix << GUICommon::getStringFromType(type, length).toStdString() + "<";
          while (entry->getContainerEntry() != nullptr &&
                 entry->getType() == Common::MemType::type_array)
          {
            suffix.seekp(0);
            suffix << ">[" + std::to_string(entry->getContainerCount()) + "]";
            entry = entry->getContainerEntry();
            if (entry->getType() == Common::MemType::type_struct)
              prefix << entry->getStructName().toStdString();
            else if (entry->getType() == Common::MemType::type_array)
            {
              prefix << entry->getStructName().toStdString() + "<";
            }
            else
              prefix << GUICommon::getStringFromType(entry->getType(), entry->getLength())
                            .toStdString();
          }
          std::string typeOut = prefix.str() + suffix.str();
          return QString().fromStdString(typeOut);
        }
        return GUICommon::getStringFromType(type, length);
      }
      case WATCH_COL_ADDRESS:
      {
        return getAddressString(entry);
      }
      case WATCH_COL_VALUE:
      {
        if (!GUICommon::isContainerType(entry->getType()))
          return QString::fromStdString(entry->getStringFromMemory());
        else if (entry->getType() == Common::MemType::type_struct)
        {
          if (entry->getStructName().isEmpty())
            return QString("No Struct type assigned");
          else if (!m_structDefMap.contains(entry->getStructName()))
            return QString("%1 not found").arg(entry->getStructName());
          else if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(entry->getActualAddress()))
            return QString("???");
          else
            return QString();
        }
        break;
      }
      default:
        break;
      }
    }

    if (index.column() == 0 && role == Qt::DecorationRole &&
        GUICommon::isContainerType(entry->getType()))
    {
      if (entry->getType() == Common::MemType::type_struct)
      {
        static const QIcon s_structIcon(":/struct2.svg");
        return s_structIcon;
      }
      else if (entry->getType() == Common::MemType::type_array)
      {
        static const QIcon s_arrayIcon(":/array.svg");
        return s_arrayIcon;
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

  MemWatchTreeNode* parent = node->getParent();
  // These flags are common to every node
  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

  if (node->isGroup())
  {
    flags |= Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
    if (index.column() == WATCH_COL_LABEL)
      flags |= Qt::ItemIsEditable;
    return flags;
  }
  else if (node->getEntry()->getType() == Common::MemType::type_none)
    return Qt::ItemFlag::NoItemFlags;
  else if (parent == m_rootNode || parent->isGroup())
    flags |= Qt::ItemIsDragEnabled;

  if (index.column() == WATCH_COL_LOCK)
    return flags;

  if (index.column() == WATCH_COL_LABEL)
  {
    const bool parentContainer{parent->getEntry() != nullptr &&
                               GUICommon::isContainerType(parent->getEntry()->getType())};
    const Qt::ItemFlag itemIsEditable{!parentContainer ? Qt::ItemIsEditable : Qt::NoItemFlags};
    flags |= itemIsEditable;
  }
  else if (index.column() == WATCH_COL_VALUE)
  {
    const bool container{node->getEntry() != nullptr &&
                         GUICommon::isContainerType(node->getEntry()->getType())};
    const bool hooked{DolphinComm::DolphinAccessor::getStatus() ==
                      DolphinComm::DolphinAccessor::DolphinStatus::hooked};
    const Qt::ItemFlag itemIsEditable{hooked && !container ? Qt::ItemIsEditable : Qt::NoItemFlags};
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

void MemWatchModel::loadRootFromJsonRecursive(const QJsonObject& json,
                                              const QMap<QString, QString> structNameReplacements)
{
  beginResetModel();
  m_rootNode->readFromJson(json, structNameReplacements);
  endResetModel();
  setupContainersRecursive(m_rootNode);
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
  m_rootNode->writeToJson(json, !SConfig::getInstance().getCollapseGroupsOnSave());
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

void MemWatchModel::setupStructNode(MemWatchTreeNode* node)
{
  bool wasExpanded = node->isExpanded();
  collapseStructNode(node);
  removeNodeFromStructNodeMap(node, true);

  if (m_structDefMap.contains(node->getEntry()->getStructName()))
  {
    addNodeToStructNodeMap(node);
    if (!m_structDefMap[node->getEntry()->getStructName()]->getFields().isEmpty())
    {
      if (wasExpanded)
        expandStructNode(node);
      else
        addNodes({new MemWatchTreeNode(new MemWatchEntry(m_placeholderEntry))},
                 getIndexFromTreeNode(node), true);
    }
  }
}

void MemWatchModel::addNodeToStructNodeMap(MemWatchTreeNode* node)
{
  QString name = node->getEntry()->getStructName();
  if (name.isEmpty())
    return;
  if (!m_structNodes.contains(name))
    m_structNodes.insert(name, {node});
  else if (!m_structNodes[name].contains(node))
    m_structNodes[name].push_back(node);
}

void MemWatchModel::removeNodeFromStructNodeMap(MemWatchTreeNode* node, bool allEntries)
{
  QList<MemWatchTreeNode*> queue{node};

  while (queue.count() > 0)
  {
    MemWatchTreeNode* curNode = queue.takeFirst();
    if (curNode->hasChildren())
    {
      queue.append(curNode->getChildren());
    }

    if (!curNode->isGroup() && curNode->getEntry() &&
        curNode->getEntry()->getType() == Common::MemType::type_struct)
    {
      QStringList names{};
      if (allEntries)
        names.append(m_structNodes.keys());
      else
        names.append(node->getEntry()->getStructName());

      for (QString name : names)
      {
        if (name.isEmpty() || m_structNodes.isEmpty() || !m_structNodes.contains(name) ||
            m_structNodes[name].isEmpty() || !m_structNodes[name].contains(node))
          continue;

        m_structNodes[name].remove(m_structNodes[name].indexOf(node));
        if (m_structNodes[name].isEmpty())
          m_structNodes.remove(name);
      }
    }
  }
}

void MemWatchModel::setStructMap(QMap<QString, StructDef*> structDefMap)
{
  m_structDefMap = structDefMap;
}

void MemWatchModel::onStructNameChanged(const QString old_name, const QString new_name)
{
  StructDef* changedStruct = m_structDefMap.take(old_name);
  m_structDefMap.insert(new_name, changedStruct);

  if (!m_structNodes.keys().contains(old_name))
    return;

  for (MemWatchTreeNode* node : m_structNodes[old_name])
  {
    node->getEntry()->setStructName(new_name);
    QModelIndex index = getIndexFromTreeNode(node);
    emit dataChanged(index, index.siblingAtColumn(WATCH_COL_NUM - 1));
  }

  QVector<MemWatchTreeNode*> nodes = m_structNodes.take(old_name);
  m_structNodes.insert(new_name, nodes);
}

void MemWatchModel::onStructDefAddRemove(QString structName, StructDef* structDef)
{
  if (structDef == nullptr)
    m_structDefMap.remove(structName);
  else if (m_structDefMap.contains(structName))
    m_structDefMap[structName] = structDef;
  else
    m_structDefMap.insert(structName, structDef);

  updateStructEntries(structName);
}

void MemWatchModel::updateStructEntries(const QString structName)
{
  if (!m_structNodes.contains(structName))
    return;
  for (MemWatchTreeNode* node : m_structNodes[structName])
  {
    if (!m_structNodes[structName].contains(node))
      continue;
    updateStructNode(node);
  }
}

void MemWatchModel::updateStructNode(MemWatchTreeNode* node)
{
  const QString structName = node->getEntry()->getStructName();

  if (!m_structDefMap.contains(structName) || m_structDefMap[structName]->getFields().isEmpty())
  {
    while (node->hasChildren())
      deleteNode(getIndexFromTreeNode(node->getChildren()[0]));
    return;
  }

  bool isExpanded{node->isExpanded()};

  if (!node->hasChildren())
  {
    addNodes({new MemWatchTreeNode(new MemWatchEntry(m_placeholderEntry))},
             getIndexFromTreeNode(node), true);
  }
  else if (node->hasChildren() && isExpanded)
  {
    // Shortcut for deleting all children and adding the placeholder child. Also overrides expanded
    // state, so need to capture above and use below
    collapseStructNode(node);
  }

  if (isExpanded)
  {
    if (m_structDefMap.contains(structName))
      expandStructNode(node);
  }
}

void MemWatchModel::expandContainerNode(MemWatchTreeNode* node)
{
  if (node->getEntry()->getType() == Common::MemType::type_struct)
    expandStructNode(node);
  if (node->getEntry()->getType() == Common::MemType::type_array)
    expandArrayNode(node);
}

void MemWatchModel::expandStructNode(MemWatchTreeNode* node)
{
  MemWatchEntry* entry = node->getEntry();
  if (!m_structDefMap.contains(entry->getStructName()))
  {
    while (node->hasChildren())
      deleteNode(getIndexFromTreeNode(node->getChildren()[0]));
    return;
  }

  for (MemWatchTreeNode* child : node->getChildren())
    deleteNode(getIndexFromTreeNode(child));

  StructDef* def = m_structDefMap[entry->getStructName()];
  if (def->getFields().isEmpty())
    return;

  entry->setTypeAndLength(entry->getType(), def->getLength());
  node->setExpanded(true);

  u32 addr = entry->getActualAddress();
  std::vector<MemWatchTreeNode*> childNodes{};
  for (FieldDef* field : def->getFields())
  {
    MemWatchTreeNode* nextNode = new MemWatchTreeNode(new MemWatchEntry(field->getEntry()));
    nextNode->getEntry()->setConsoleAddress(addr + field->getOffset());
    childNodes.push_back(nextNode);
  }
  addNodes(childNodes, getIndexFromTreeNode(node), true);
}

void MemWatchModel::expandArrayNode(MemWatchTreeNode* node)
{
  for (MemWatchTreeNode* child : node->getChildren())
    deleteNode(getIndexFromTreeNode(child));

  std::vector<MemWatchTreeNode*> childNodes{};
  for (size_t i = 0; i < node->getEntry()->getContainerCount(); i++)
  {
    MemWatchEntry* childEntry = new MemWatchEntry(node->getEntry()->getContainerEntry());
    QString childLabel = QString("[%1]").arg(i);
    if (childEntry->getLabel() != "No label")
      childLabel += childEntry->getLabel();
    childEntry->setLabel(childLabel);

    MemWatchTreeNode* child = new MemWatchTreeNode(childEntry, node);
    childNodes.push_back(child);
  }
  addNodes(childNodes, getIndexFromTreeNode(node), true);
  updateArrayAddresses(node);
}

void MemWatchModel::collapseArrayNode(MemWatchTreeNode* node)
{
  while (node->hasChildren())
    deleteNode(getIndexFromTreeNode(node->getChildren()[0]));

  addNodes({new MemWatchTreeNode(new MemWatchEntry(m_placeholderEntry))},
           getIndexFromTreeNode(node), true);
}

int MemWatchModel::getTotalContainerLength(MemWatchEntry* entry)
{
  if (entry->isBoundToPointer())
    return 4;
  if (!GUICommon::isContainerType(entry->getType()))
    return static_cast<int>(Common::getSizeForType(entry->getType(), entry->getLength()));
  if (entry->getType() == Common::MemType::type_array && entry->getContainerEntry() != nullptr)
    return static_cast<int>(entry->getContainerCount() *
                            getTotalContainerLength(entry->getContainerEntry()));
  else if (entry->getType() == Common::MemType::type_struct &&
           m_structDefMap.contains(entry->getStructName()))
    return static_cast<int>(m_structDefMap.find(entry->getStructName()).value()->getLength());
  return 0;
}

void MemWatchModel::collapseContainerNode(MemWatchTreeNode* node)
{
  if (node->getEntry()->getType() == Common::MemType::type_struct)
    collapseStructNode(node);
  else if (node->getEntry()->getType() == Common::MemType::type_array)
    collapseArrayNode(node);
}

void MemWatchModel::collapseStructNode(MemWatchTreeNode* node)
{
  while (node->hasChildren())
    deleteNode(getIndexFromTreeNode(node->getChildren()[0]));

  if (m_structDefMap.contains(node->getEntry()->getStructName()) &&
      !m_structDefMap[node->getEntry()->getStructName()]->getFields().isEmpty())
    addNodes({new MemWatchTreeNode(new MemWatchEntry(m_placeholderEntry))},
             getIndexFromTreeNode(node), true);
  node->setExpanded(false);
}

void MemWatchModel::updateContainerAddresses(MemWatchTreeNode* node)
{
  if (node->getEntry()->getType() == Common::MemType::type_struct)
    updateStructAddresses(node);
  else if (node->getEntry()->getType() == Common::MemType::type_array)
    updateArrayAddresses(node);
}

void MemWatchModel::updateStructAddresses(MemWatchTreeNode* node)
{
  if (!m_structDefMap.contains(node->getEntry()->getStructName()))
  {
    while (node->hasChildren())
      deleteNode(getIndexFromTreeNode(node->getChildren()[0]));
    return;
  }

  StructDef* def = m_structDefMap[node->getEntry()->getStructName()];

  if (node->isExpanded())
    updateStructNode(node);
  else if (!node->getEntry()->hasAddressChanged())
    return;
  else
  {
    u32 addr = node->getEntry()->getActualAddress();
    node->getEntry()->updateActualAddress(addr);
    QVector<FieldDef*> fields = def->getFields();
    QVector<MemWatchTreeNode*> children = node->getChildren();

    for (int i = 0; i < children.count(); ++i)
    {
      children[i]->getEntry()->setConsoleAddress(addr + fields[i]->getOffset());
      if (GUICommon::isContainerType(children[i]->getEntry()->getType()))
        updateContainerAddresses(children[i]);
    }
  }
}

void MemWatchModel::updateArrayAddresses(MemWatchTreeNode* node)
{
  if (!node->isExpanded())
    return;

  u32 addr = node->getEntry()->getActualAddress();
  node->getEntry()->updateActualAddress(addr);
  QVector<MemWatchTreeNode*> children = node->getChildren();

  MemWatchEntry* containerEntry = node->getEntry()->getContainerEntry();
  u32 entrySize = getTotalContainerLength(containerEntry);

  for (int i = 0; i < children.count(); i++)
  {
    children[i]->getEntry()->setConsoleAddress(addr + (entrySize * i));
    if (GUICommon::isContainerType(children[i]->getEntry()->getType()))
      updateContainerAddresses(children[i]);
  }
}

void MemWatchModel::setupContainersRecursive(MemWatchTreeNode* node)
{
  if (node->getChildren().isEmpty())
    return;

  for (MemWatchTreeNode* child : node->getChildren())
  {
    if (child->getParent() == nullptr || child->isGroup())
      setupContainersRecursive(child);
    else if (GUICommon::isContainerType(child->getEntry()->getType()))
    {
      if (child->getEntry()->getType() == Common::MemType::type_struct)
        setupStructNode(child);
      if (child->getEntry()->getType() == Common::MemType::type_array)
        setupArrayNode(child);
    }
  }
}

void MemWatchModel::setContainerCount(MemWatchTreeNode* node, size_t count)
{
  node->getEntry()->setContainerCount(count);
  setupArrayNode(node);
}

void MemWatchModel::setupArrayNode(MemWatchTreeNode* node)
{
  if (node->childrenCount() == 0)
  {
    addNodes({new MemWatchTreeNode(new MemWatchEntry(m_placeholderEntry))},
             getIndexFromTreeNode(node), true);
    return;
  }
  else if (node->isExpanded())
  {
    collapseArrayNode(node);
    expandArrayNode(node);
  }
}
