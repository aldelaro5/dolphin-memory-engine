#pragma once

#include <QAbstractListModel>
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
    STRUCT_COL_LABEL,
    STRUCT_COL_DETAIL,
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

  bool editData(const QModelIndex& index, const QVariant& value, int role, bool emitEdit = false);
  void addPaddingFields(int count = 1, int start = -1);
  void removeFields(QModelIndexList indices);
  void removeLastField();
  void clearFields(QModelIndexList indices);
  void updateFieldEntry(MemWatchEntry* entry, const QModelIndex& index);
  FieldDef* getFieldByRow(int row);

  bool hasStructLoaded() const;
  StructTreeNode* getLoadedStructNode() const;
  void loadStruct(StructTreeNode* baseStruct);
  void saveStruct();
  void unloadStruct();

  bool willRemoveFields(u32 newLength);
  QString getRemovedFieldDescriptions(u32 newLength);
  void updateFieldsWithNewLength();

signals:
  void dataEdited(const QModelIndex& index, const QVariant& value, int role);

private:
  QString getFieldDetails(FieldDef* field) const;
  void removeFields(int start, int count = 1);
  void updateFieldOffsets();

  StructTreeNode* m_baseNode = nullptr;
  QVector<FieldDef*> m_fields{};
  u32 m_curSize = 0;
};
