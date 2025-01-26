#pragma once

#include <QAbstractItemModel>
#include "../../Structs/StructTreeNode.h"

class StructSelectModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum
  {
    WATCH_COL_LABEL = 0,
    WATCH_COL_NUM
  };

  explicit StructSelectModel(QObject* parent, StructTreeNode* rootNode);
  ~StructSelectModel() override;

  StructSelectModel(const StructSelectModel&) = delete;
  StructSelectModel(StructSelectModel&&) = delete;
  StructSelectModel& operator=(const StructSelectModel&) = delete;
  StructSelectModel& operator=(StructSelectModel&&) = delete;

  int columnCount(const QModelIndex& parent) const override;
  int rowCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  Qt::DropActions supportedDropActions() const override;
  Qt::DropActions supportedDragActions() const override;
  QStringList mimeTypes() const override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                    const QModelIndex& parent) override;

  void addNodes(const std::vector<StructTreeNode*>& nodes,
                const QModelIndex& referenceIndex = QModelIndex{});
  void addGroup(const QString& name, const QModelIndex& referenceIndex = QModelIndex{});
  StructTreeNode* addStruct(const QString& name, const QModelIndex& referenceIndex = QModelIndex{});
  void deleteNode(const QModelIndex& index);

  void setNodeLabel(StructTreeNode* node, const QString name);

  static StructTreeNode* getTreeNodeFromIndex(const QModelIndex& index);
  QModelIndex getIndexFromTreeNode(const StructTreeNode* node);

  QMap<QString, StructDef*> getStructMap();

signals:
  void dataEdited(const QModelIndex& index, const QVariant& value, int role);
  void dropSucceeded();
  void nameChangeFailed(StructTreeNode* node, QString name);

private:
  StructTreeNode* getLeastDeepNodeFromList(const QList<StructTreeNode*>& nodes) const;
  int getNodeDeepness(const StructTreeNode* node) const;

  StructTreeNode* m_rootNode;
};
