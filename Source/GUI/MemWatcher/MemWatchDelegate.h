#pragma once

#include <QStyledItemDelegate>

class MemWatchDelegate : public QStyledItemDelegate
{
public:
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};