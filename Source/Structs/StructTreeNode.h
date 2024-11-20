#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "StructDef.h"

class StructTreeNode
{
public:
  explicit StructTreeNode(StructDef* structDef, StructTreeNode* parent = nullptr,
                             bool isGroup = false, bool isAuto = false, QString groupName = {});
  ~StructTreeNode();

  StructTreeNode(const StructTreeNode&) = delete;
  StructTreeNode(StructTreeNode&&) = delete;
  StructTreeNode& operator=(const StructTreeNode&) = delete;
  StructTreeNode& operator=(StructTreeNode&&) = delete;

  bool isGroup() const;
  bool isExpanded() const;
  void setExpanded(const bool expanded);
  const QString& getGroupName() const;
  void setGroupName(const QString& groupName);
  QString getStructName() const;
  void setStructName(QString structDef);
  StructTreeNode* getParent() const;
  void setParent(StructTreeNode* parent);
  int getRow() const;
  bool hasChildren() const;
  int childrenCount() const;
  const QVector<StructTreeNode*>& getChildren() const;
  void setChildren(QVector<StructTreeNode*> children);

  void appendChild(StructTreeNode* node);
  void insertChild(int row, StructTreeNode* node);
  void removeChild(int row);
  void removeChild(StructTreeNode* child);
  void removeChildren();
  void deleteChildren();

  void readFromJson(const QJsonObject& json, StructTreeNode* parent = nullptr, QStringList structNames);
  void writeToJson(QJsonObject& json, QStringList structNames) const;

private:
  bool m_isGroup;
  QString m_groupName;
  bool m_expanded{};

  QString m_structName;
  QVector<StructTreeNode*> m_children;
  StructTreeNode* m_parent;
  
};
