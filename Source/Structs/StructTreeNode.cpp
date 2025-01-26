#include "StructTreeNode.h"

#include <QJsonArray>

StructTreeNode::StructTreeNode(StructDef* const structDef, StructTreeNode* const parent, bool isGroup, QString name)
{
  m_parent = parent;
  m_isGroup = isGroup;
  m_nodeName = std::move(name);
  m_structDef = structDef;
  updateName();
}

StructTreeNode::StructTreeNode(StructTreeNode* node)
    : m_parent(node->getParent()), m_isGroup(node->isGroup()), m_nodeName(QString(node->getName())),
      m_structDef(new StructDef(node->getStructDef()))
{
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

const QString& StructTreeNode::getName()
{
  updateName();
  return m_nodeName;
}

void StructTreeNode::setName(const QString& name)
{
  if (m_structDef != nullptr)
    m_structDef->setLabel(name);
  m_nodeName = name;
}

bool StructTreeNode::isNameAvailable(QString name) const
{


  if (m_children.isEmpty())
    return true;

  for (StructTreeNode* child : m_children)
  {
    if (name == child->getName())
      return false;
  }
  return true;
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
  deleteChildren();
  m_children = children;
}

QVector<QString> StructTreeNode::getChildNames()
{
  if (m_children.isEmpty())
    return QVector<QString>();

  QVector<QString> names = QVector<QString>();
  for (StructTreeNode* child : m_children)
  {
    names.push_back(child->getName());
  }
  return names;
}

StructDef* StructTreeNode::getStructDef() const
{
  return m_structDef;
}

void StructTreeNode::setStructDef(StructDef* structDef)
{
  delete m_structDef;
  m_structDef = structDef;
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

void StructTreeNode::readFromJson(const QJsonObject& json, StructTreeNode* parent)
{
  m_parent = parent;
  if (json["rootNode"] != QJsonValue::Undefined)
  {
    m_isGroup = false;
    QJsonArray structTree = json["rootNode"].toArray();
    for (auto i : structTree)
    {
      QJsonObject node = i.toObject();
      StructTreeNode* childNode = new StructTreeNode(nullptr, nullptr);
      childNode->readFromJson(node, this);
      m_children.append(childNode);
    }
  }
  else if (json["groupName"] != QJsonValue::Undefined)
  {
    m_isGroup = true;
    m_nodeName = json["groupName"].toString();
    m_expanded = json["expanded"].toBool();
    QJsonArray groupChildren = json["groupChildren"].toArray();
    for (auto i : groupChildren)
    {
      QJsonObject node = i.toObject();
      StructTreeNode* childNode = new StructTreeNode(nullptr, nullptr);
      childNode->readFromJson(node, this);
      if (isNameAvailable(childNode->getName()))
        m_children.append(childNode);
    }
  }
  else
  {
    m_isGroup = false;
    m_nodeName = json["structName"].toString();
    m_structDef = new StructDef();
    m_structDef->readFromJson(json["struct"].toObject());
  }
}

void StructTreeNode::writeToJson(QJsonObject& json) const
{
  if (isGroup())
  {
    json["groupName"] = m_nodeName;
    json["expanded"] = m_expanded;
    QJsonArray entries;
    for (StructTreeNode* const child : m_children)
    {
      QJsonObject nextNode;
      child->writeToJson(nextNode);
      entries.append(nextNode);
    }
    json["groupChildren"] = entries;
  }
  else if (m_parent == nullptr)
  {
    QJsonArray rootNode;
    for (StructTreeNode* const child : m_children)
    {
      QJsonObject nextNode;
      child->writeToJson(nextNode);
      rootNode.append(nextNode);
    }
    json["rootNode"] = rootNode;
  }
  else
  {
    json["structName"] = m_nodeName;
    QJsonObject structDefJson;
    m_structDef->writeToJson(structDefJson);
    json["struct"] = structDefJson;
  }
}

QVector<QString> StructTreeNode::getStructNames(bool includeGroups, QString prefix)
{
  updateName();

  QVector<QString> names;

  if (prefix == QString("") && m_parent)
    prefix = m_parent->getNameSpace();

  QString nodeName = appendNameToNameSpace(prefix);
  if (!m_isGroup || includeGroups)
    names.push_back(nodeName);

  for (StructTreeNode* child : m_children)
  {
    QVector<QString> childNames = child->getStructNames(includeGroups, nodeName);
    for (QString name : childNames)
    {
      names.push_back(name);
    }
  }

  return names;
}

QString StructTreeNode::getNameSpace()
{
  updateName();

  if (m_parent != nullptr)
  {
    QString parentNamespace = m_parent->getNameSpace();
    return parentNamespace.isEmpty() ? m_nodeName : parentNamespace + QString("::") + m_nodeName;
  }
  return QString();
}

QString StructTreeNode::appendNameToNameSpace(QString nameSpace) const
{
  return nameSpace + QString("::") + m_nodeName;
}

u32 StructTreeNode::getSizeOfStruct(QString nameSpace)
{
  StructTreeNode* targetStructNode = findNode(nameSpace);
  if (targetStructNode == nullptr || targetStructNode->isGroup())
    return 0;
  else
    return targetStructNode->getStructDef()->getLength();
}

void StructTreeNode::updateName()
{
  if (m_structDef != nullptr)
    m_nodeName = m_structDef->getLabel();
}

StructTreeNode* StructTreeNode::findNode(QString nameSpace)
{
  StructTreeNode* cur_node = this;
  while (cur_node->getParent() != nullptr)
    cur_node = cur_node->getParent();

  QStringList names = nameSpace.split("::");
  for (QString name : names)
  {
    bool childFound = false;
    for (StructTreeNode* child : cur_node->getChildren())
    {
      if (child->getName() == name)
      {
        childFound = true;
        cur_node = child;
        break;
      }
    }
    if (!childFound)
      return nullptr;
  }

  return cur_node;
}
