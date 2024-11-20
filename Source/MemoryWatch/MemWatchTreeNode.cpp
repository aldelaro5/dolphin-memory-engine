#include "MemWatchTreeNode.h"

#include <QJsonArray>

#include <sstream>
#include <utility>

#include "../GUI/GUICommon.h"

MemWatchTreeNode::MemWatchTreeNode(MemWatchEntry* const entry, MemWatchTreeNode* const parent,
                                   const bool isGroup, QString groupName)
    : m_isGroup(isGroup), m_groupName(std::move(groupName)), m_entry(entry), m_parent(parent)
{
}

MemWatchTreeNode::~MemWatchTreeNode()
{
  deleteChildren();
  m_parent = nullptr;
  delete m_entry;
  m_entry = nullptr;
}

bool MemWatchTreeNode::isGroup() const
{
  return m_isGroup;
}

bool MemWatchTreeNode::hasChildren() const
{
  return (m_children.count() >= 1);
}

int MemWatchTreeNode::childrenCount() const
{
  return static_cast<int>(m_children.count());
}

bool MemWatchTreeNode::isValueEditing() const
{
  return m_isValueEditing;
}

void MemWatchTreeNode::setValueEditing(const bool valueEditing)
{
  m_isValueEditing = valueEditing;
}

const QString& MemWatchTreeNode::getGroupName() const
{
  return m_groupName;
}

void MemWatchTreeNode::setGroupName(const QString& groupName)
{
  m_groupName = groupName;
}

MemWatchEntry* MemWatchTreeNode::getEntry() const
{
  return m_entry;
}

void MemWatchTreeNode::setEntry(MemWatchEntry* entry)
{
  delete m_entry;
  m_entry = entry;
}

const QVector<MemWatchTreeNode*>& MemWatchTreeNode::getChildren() const
{
  return m_children;
}

void MemWatchTreeNode::setChildren(QVector<MemWatchTreeNode*> children)
{
  m_children = std::move(children);
}

MemWatchTreeNode* MemWatchTreeNode::getParent() const
{
  return m_parent;
}

void MemWatchTreeNode::setParent(MemWatchTreeNode* const parent)
{
  m_parent = parent;
}

int MemWatchTreeNode::getRow() const
{
  if (m_parent != nullptr)
    return static_cast<int>(m_parent->m_children.indexOf(const_cast<MemWatchTreeNode*>(this)));

  return 0;
}

void MemWatchTreeNode::appendChild(MemWatchTreeNode* node)
{
  m_children.append(node);
  node->m_parent = this;
}

void MemWatchTreeNode::insertChild(const int row, MemWatchTreeNode* node)
{
  m_children.insert(row, node);
  node->m_parent = this;
}

void MemWatchTreeNode::removeChild(const int row)
{
  m_children[row]->m_parent = nullptr;
  m_children.remove(row);
}

void MemWatchTreeNode::removeChild(MemWatchTreeNode* const child)
{
  m_children.removeAll(child);
  if (child->m_parent == this)
  {
    child->m_parent = nullptr;
  }
}

void MemWatchTreeNode::removeChildren()
{
  m_children.clear();
}

void MemWatchTreeNode::deleteChildren()
{
  qDeleteAll(m_children);
  m_children.clear();
}

void MemWatchTreeNode::readFromJson(const QJsonObject& json, MemWatchTreeNode* parent)
{
  m_parent = parent;
  if (json["watchList"] != QJsonValue::Undefined)
  {
    m_isGroup = false;
    QJsonArray watchList = json["watchList"].toArray();
    for (auto i : watchList)
    {
      QJsonObject node = i.toObject();
      MemWatchTreeNode* childNode = new MemWatchTreeNode(nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else if (json["groupName"] != QJsonValue::Undefined)
  {
    m_isGroup = true;
    m_groupName = json["groupName"].toString();
    if (json.contains("expanded"))
    {
      m_expanded = json["expanded"].toBool();
    }
    QJsonArray groupEntries = json["groupEntries"].toArray();
    for (auto i : groupEntries)
    {
      QJsonObject node = i.toObject();
      MemWatchTreeNode* childNode = new MemWatchTreeNode(nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else
  {
    m_isGroup = false;
    m_entry = new MemWatchEntry();
    m_entry->readFromJson(json);
    }
  }

void MemWatchTreeNode::writeToJson(QJsonObject& json, const bool writeExpandedState) const
{
  if (isGroup())
  {
    json["groupName"] = m_groupName;
    if (m_expanded && writeExpandedState)
    {
      json["expanded"] = m_expanded;
    }
    QJsonArray entries;
    for (MemWatchTreeNode* const child : m_children)
    {
      QJsonObject theNode;
      child->writeToJson(theNode, writeExpandedState);
      entries.append(theNode);
    }
    json["groupEntries"] = entries;
  }
  else
  {
    if (m_parent == nullptr)
    {
      QJsonArray watchList;
      for (MemWatchTreeNode* const child : m_children)
      {
        QJsonObject theNode;
        child->writeToJson(theNode, writeExpandedState);
        watchList.append(theNode);
      }
      json["watchList"] = watchList;
    }
    else
    {
      m_entry->writeToJson(json);
    }
  }
}

QString MemWatchTreeNode::writeAsCSV() const
{
  if (isGroup() || m_parent == nullptr)
  {
    QString rootCsv;
    for (MemWatchTreeNode* const child : m_children)
    {
      QString theCsvLine = child->writeAsCSV();
      rootCsv.append(theCsvLine);
    }
    return rootCsv;
  }

  std::stringstream ssAddress;
  ssAddress << std::hex << std::uppercase << m_entry->getConsoleAddress();
  if (m_entry->isBoundToPointer())
  {
    for (int i = 0; i < static_cast<int>(m_entry->getPointerLevel()); ++i)
    {
      std::stringstream ssOffset;
      ssOffset << std::hex << std::uppercase << m_entry->getPointerOffset(i);
      ssAddress << "[" << ssOffset.str() << "]";
    }
  }
  std::string csvLine =
      m_entry->getLabel().toStdString() + ";" + ssAddress.str() + ";" +
      GUICommon::getStringFromType(m_entry->getType(), m_entry->getLength()).toStdString() + "\n";
  return QString::fromStdString(csvLine);
}
