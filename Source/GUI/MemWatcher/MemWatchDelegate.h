#pragma once

#include <QStyledItemDelegate>

class MemWatchDelegate : public QStyledItemDelegate
{
public:
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};
