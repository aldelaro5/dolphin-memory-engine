#include "StructDefTreeNode.h"

#include <QJsonArray>

StructTreeNode::StructTreeNode(StructDef* structDef, StructTreeNode* parent, bool isGroup,
                                     bool isAuto, QString groupName)
{
  QVector<StructTreeNode*> m_children;
}

StructTreeNode::~StructTreeNode()
{
  deleteChildren();
  m_parent = nullptr;
}

bool StructTreeNode::isGroup() const
{
  return m_isGroup;
}

bool StructTreeNode::isExpanded() const
{
  return m_expanded;
}

void StructTreeNode::setExpanded(const bool expanded)
{
  m_expanded = expanded;
}

const QString& StructTreeNode::getGroupName() const
{
  return m_groupName;
}

void StructTreeNode::setGroupName(const QString& groupName)
{
  m_groupName = groupName;
}

QString StructTreeNode::getStructName() const
{
  return m_structName;
}

void StructTreeNode::setStructName( QString structName)
{
  m_structName = structName;
}

StructTreeNode* StructTreeNode::getParent() const
{
  return m_parent;
}

void StructTreeNode::setParent(StructTreeNode* parent)
{
  m_parent = parent;
}

int StructTreeNode::getRow() const
{
  if (m_parent != nullptr)
    return static_cast<int>(m_parent->m_children.indexOf(const_cast<StructTreeNode*>(this)));

  return 0;
}

bool StructTreeNode::hasChildren() const
{
  return m_children.count() > 0;
}

int StructTreeNode::childrenCount() const
{
  return m_children.count();
}

const QVector<StructTreeNode*>& StructTreeNode::getChildren() const
{
  return m_children;
}

void StructTreeNode::setChildren(QVector<StructTreeNode*> children)
{
  m_children = children;
}

void StructTreeNode::appendChild(StructTreeNode* node)
{
  m_children.append(node);
  node->m_parent = this;
}

void StructTreeNode::insertChild(const int row, StructTreeNode* node)
{
  m_children.insert(row, node);
  node->m_parent = this;
}

void StructTreeNode::removeChild(const int row)
{
  m_children[row]->m_parent = nullptr;
  m_children.remove(row);
}

void StructTreeNode::removeChild(StructTreeNode* child)
{
  m_children.removeAll(child);
  if (child->m_parent == this)
  {
    child->m_parent = nullptr;
  }
}

void StructTreeNode::removeChildren()
{
  m_children.clear();
}

void StructTreeNode::deleteChildren()
{
  qDeleteAll(m_children);
  m_children.clear();
}

void StructTreeNode::readFromJson(const QJsonObject& json, StructTreeNode* parent, QStringList structNames)
{
  m_parent = parent;
  if (json["rootNode"] != QJsonValue::Undefined)
  {
    m_isGroup = false;
    QJsonArray structTree = json["rootNode"].toArray();
    for (auto i : structTree)
    {
      QJsonObject node = i.toObject();
      StructTreeNode* childNode = new StructTreeNode(nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else if (json["groupName"] != QJsonValue::Undefined)
  {
    m_isGroup = true;
    m_groupName = json["groupName"].toString();
    m_expanded = json["expanded"].toBool();
    QJsonArray groupChildren = json["groupChildren"].toArray();
    for (auto i : groupChildren)
    {
      QJsonObject node = i.toObject();
      StructTreeNode* childNode = new StructTreeNode(nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else
  {
    m_isGroup = false;
    m_structName = json["structName"].toString();
  }
}

void StructTreeNode::writeToJson(QJsonObject& json, QStringList structNames) const
{
  if (isGroup())
  {
    json["groupName"] = m_groupName;
    json["expanded"] = m_expanded;
    QJsonArray entries;
    for (StructTreeNode* const child : m_children)
    {
      QJsonObject nextNode;
      child->writeToJson(nextNode, structNames);
      entries.append(nextNode);
    }
    json["groupChildren"] = entries;
  }
  else
  {
    if (m_parent == nullptr)
    {
      QJsonArray rootNode;
      for (StructTreeNode* const child : m_children)
      {
        QJsonObject nextNode;
        child->writeToJson(nextNode, structNames);
        rootNode.append(nextNode);
      }
      json["rootNode"] = rootNode;
    }
    else if (structNames.contains(m_structName))
    {
      json["structName"] = m_structName;
    }
  }
}
