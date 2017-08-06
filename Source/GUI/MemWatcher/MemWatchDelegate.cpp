#include "MemWatchDelegate.h"

#include <QLineEdit>

#include "MemWatchModel.h"
#include "MemWatchTreeNode.h"

void MemWatchDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
  {
    node->setValueEditing(true);
  }
  else if (node->isGroup() && index.column() == MemWatchModel::WATCH_COL_LABEL)
  {
    QLineEdit* lineEditor = static_cast<QLineEdit*>(editor);
    lineEditor->setText(node->getGroupName());
  }
  else
  {
    QStyledItemDelegate::setEditorData(editor, index);
  }
}

void MemWatchDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const
{
  QStyledItemDelegate::setModelData(editor, model, index);
  MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
  if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
    node->setValueEditing(false);
}