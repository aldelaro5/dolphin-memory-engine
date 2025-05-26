#pragma once

#include <vector>

#include <QAbstractItemModel>
#include <QFile>
#include <QJsonObject>

#include "../../MemoryWatch/MemWatchEntry.h"
#include "../../MemoryWatch/MemWatchTreeNode.h"
#include "../../Structs/StructDef.h"

class MemWatchModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum
  {
    WATCH_COL_LABEL = 0,
    WATCH_COL_TYPE,
    WATCH_COL_ADDRESS,
    WATCH_COL_LOCK,
    WATCH_COL_VALUE,
    WATCH_COL_NUM
  };

  struct CTParsingErrors
  {
    QString errorStr;
    bool isCritical;
  };

  explicit MemWatchModel(QObject* parent);
  ~MemWatchModel() override;

  MemWatchModel(const MemWatchModel&) = delete;
  MemWatchModel(MemWatchModel&&) = delete;
  MemWatchModel& operator=(const MemWatchModel&) = delete;
  MemWatchModel& operator=(MemWatchModel&&) = delete;

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

  void changeType(const QModelIndex& index, Common::MemType type, size_t length);
  static MemWatchEntry* getEntryFromIndex(const QModelIndex& index);
  void addNodes(const std::vector<MemWatchTreeNode*>& nodes,
                const QModelIndex& referenceIndex = QModelIndex{},
                const bool insertInContainer = false);
  void addGroup(const QString& name, const QModelIndex& referenceIndex = QModelIndex{});
  void addEntry(MemWatchEntry* entry, const QModelIndex& referenceIndex = QModelIndex{});
  void editEntry(MemWatchEntry* entry, const QModelIndex& index);
  void clearRoot();
  void deleteNode(const QModelIndex& index);
  void groupSelection(const QModelIndexList& indexes);
  void onUpdateTimer();
  void onFreezeTimer();
  void loadRootFromJsonRecursive(const QJsonObject& jsonconst,
                                 QMap<QString, QString> structNameReplacements = {});
  CTParsingErrors importRootFromCTFile(QFile* CTFile, bool useDolphinPointer, u32 CEStart = 0);
  void writeRootToJsonRecursive(QJsonObject& json) const;
  QString writeRootToCSVStringRecursive() const;
  bool hasAnyNodes() const;
  MemWatchTreeNode* getRootNode() const;
  static MemWatchTreeNode* getTreeNodeFromIndex(const QModelIndex& index);
  QModelIndex getIndexFromTreeNode(const MemWatchTreeNode* node);
  bool editData(const QModelIndex& index, const QVariant& value, int role, bool emitEdit = false);

  void setStructMap(QMap<QString, StructDef*> structDefmap);
  void onStructNameChanged(const QString old_name, const QString new_name);
  void onStructDefAddRemove(QString structName, StructDef* structDef);
  void updateStructEntries(QString structName);
  void updateStructNode(MemWatchTreeNode* node);
  void expandContainerNode(MemWatchTreeNode* node);
  void collapseContainerNode(MemWatchTreeNode* node);
  void setupContainersRecursive(MemWatchTreeNode* node);
  void setContainerCount(MemWatchTreeNode* node, size_t count);

signals:
  void dataEdited(const QModelIndex& index, const QVariant& value, int role);
  void writeFailed(const QModelIndex& index, Common::MemOperationReturnCode writeReturn);
  void readFailed();
  void dropSucceeded();

private:
  bool updateNodeValueRecursive(MemWatchTreeNode* node, const QModelIndex& parent = QModelIndex(),
                                bool readSucess = true);
  bool freezeNodeValueRecursive(MemWatchTreeNode* node, const QModelIndex& parent = QModelIndex(),
                                bool writeSucess = true);
  MemWatchTreeNode* getLeastDeepNodeFromList(const QList<MemWatchTreeNode*>& nodes) const;
  int getNodeDeepness(const MemWatchTreeNode* node) const;

  void updateContainerAddresses(MemWatchTreeNode* node);
  void updateStructAddresses(MemWatchTreeNode* node);
  void updateArrayAddresses(MemWatchTreeNode* node);
  void setupStructNode(MemWatchTreeNode* node);
  void addNodeToStructNodeMap(MemWatchTreeNode* node);
  void removeNodeFromStructNodeMap(MemWatchTreeNode* node, bool allEntries = false);
  void expandStructNode(MemWatchTreeNode* node);
  void collapseStructNode(MemWatchTreeNode* node);
  void setupArrayNode(MemWatchTreeNode* node);
  void expandArrayNode(MemWatchTreeNode* node);
  void collapseArrayNode(MemWatchTreeNode* node);
  int getTotalContainerLength(MemWatchEntry* entry);

  MemWatchTreeNode* m_rootNode;
  MemWatchEntry* m_placeholderEntry;
  QMap<QString, StructDef*> m_structDefMap{};
  QMap<QString, QVector<MemWatchTreeNode*>> m_structNodes{};
};
