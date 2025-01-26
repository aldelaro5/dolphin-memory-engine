#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "StructDef.h"

class StructTreeNode
{
public:
  explicit StructTreeNode(StructDef* const structDef, StructTreeNode* const parent, bool isGroup = false, QString name = {});
  explicit StructTreeNode(StructTreeNode* node);
  ~StructTreeNode();

  StructTreeNode(const StructTreeNode&) = delete;
  StructTreeNode(StructTreeNode&&) = delete;
  StructTreeNode& operator=(const StructTreeNode&) = delete;
  StructTreeNode& operator=(StructTreeNode&&) = delete;

  bool isGroup() const;
  bool isExpanded() const;
  void setExpanded(const bool expanded);
  const QString& getName();
  void setName(const QString& name);
  bool isNameAvailable(const QString name) const;
  StructTreeNode* getParent() const;
  void setParent(StructTreeNode* parent);
  int getRow() const;
  bool hasChildren() const;
  int childrenCount() const;
  const QVector<StructTreeNode*>& getChildren() const;
  void setChildren(QVector<StructTreeNode*> children);
  QVector<QString> getChildNames();
  StructDef* getStructDef() const;
  void setStructDef(StructDef* structDef);

  void appendChild(StructTreeNode* node);
  void insertChild(int row, StructTreeNode* node);
  void removeChild(int row);
  void removeChild(StructTreeNode* child);
  void removeChildren();
  void deleteChildren();

  void readFromJson(const QJsonObject& json, StructTreeNode* parent = nullptr);
  void writeToJson(QJsonObject& json) const;

  QVector<QString> getStructNames(bool includeGroups = false, QString prefix = QString(""));
  QString getNameSpace();
  QString appendNameToNameSpace(QString nameSpace) const;
  u32 getSizeOfStruct(QString nameSpace);

private:
  void updateName();
  StructTreeNode* findNode(QString nameSpace);

  bool m_isGroup;
  QString m_nodeName;
  bool m_expanded{};
  StructDef* m_structDef;

  QVector<StructTreeNode*> m_children{};
  StructTreeNode* m_parent;
  
};
