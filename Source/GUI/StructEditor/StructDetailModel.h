#pragma once

#include <QAbstractListModel>
#include <QDropEvent>
#include <QFile>
#include "../../Structs/StructTreeNode.h"

class StructDetailModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum
  {
    STRUCT_COL_OFFSET = 0,
    STRUCT_COL_SIZE,
    STRUCT_COL_TYPE,
    STRUCT_COL_LABEL,
    STRUCT_COL_NUM
  };

  explicit StructDetailModel(QObject* parent);
  ~StructDetailModel() override;

  StructDetailModel(const StructDetailModel&) = delete;
  StructDetailModel(StructDetailModel&&) = delete;
  StructDetailModel& operator=(const StructDetailModel&) = delete;
  StructDetailModel& operator=(StructDetailModel&&) = delete;

  int columnCount(const QModelIndex& parent) const override;
  int rowCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  Qt::DropActions supportedDropActions() const override;
  Qt::DropActions supportedDragActions() const override;
  QStringList mimeTypes() const override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                    const QModelIndex& parent) override;

  void addField(const QModelIndex& index, FieldDef* field);
  void addPaddingFields(int count = 1, int start = -1);
  void removePaddingFields(int count, int start);
  void removeFields(QModelIndexList indices);
  void removeLastField();
  void clearFields(QModelIndexList indices);
  bool updateFieldEntry(MemWatchEntry* entry, const QModelIndex& index);
  int getTotalContainerLength(MemWatchEntry* entry);
  FieldDef* getFieldByRow(int row);
  QModelIndex getLastIndex(int col = 0);
  QModelIndex getIndexAt(int row, int col = 0);

  bool hasStructLoaded() const;
  StructTreeNode* getLoadedStructNode() const;
  void loadStruct(StructTreeNode* baseStruct);
  void saveStruct();
  void unloadStruct();

  bool willRemoveFields(u32 newLength);
  QString getRemovedFieldDescriptions(u32 newLength);
  void updateFieldsWithNewLength();
  void updateStructTypeLabel(QString oldName, QString newName);

signals:
  void dataEdited(const QModelIndex& index, const QVariant& value, int role);
  void lengthChanged(u32 newLength);
  void modifyStructReference(QString nodeName, QString target, bool addIt, bool& ok);
  void modifyStructPointerReference(QString nodeName, QString target, bool addIt);
  void getStructLength(const QString name, int& len);

private:
  QString getFieldDetails(FieldDef* field) const;
  void removeFields(int start, int count = 1);
  void updateFieldOffsets();
  void reduceIndicesToRows(QModelIndexList& indices);

  StructTreeNode* m_baseNode = nullptr;
  QVector<FieldDef*> m_fields{};
  u32 m_curSize = 0;
};
