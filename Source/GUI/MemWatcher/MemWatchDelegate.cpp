#include "MemWatchDelegate.h"

#include <QLineEdit>

#include "../GUICommon.h"
#include "MemWatchModel.h"
#include "MemWatchTreeNode.h"

QWidget* MemWatchDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
  const MemWatchModel* model = static_cast<const MemWatchModel*>(index.model());
  MemWatchTreeNode* node = model->getTreeNodeFromIndex(index);
  if ((index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup()) ||
      (node->isGroup() && index.column() == MemWatchModel::WATCH_COL_LABEL))
  {
    QLineEdit* editor = new QLineEdit(parent);
    editor->setFrame(false);
    if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
    {
      GUICommon::g_valueEditing = true;
      node->setValueEditing(true);
    }
    return editor;
  }
  return 0;
}

void MemWatchDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  const MemWatchModel* model = static_cast<const MemWatchModel*>(index.model());
  MemWatchTreeNode* node = model->getTreeNodeFromIndex(index);
  QLineEdit* lineEditor = static_cast<QLineEdit*>(editor);
  if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
    lineEditor->setText(QString::fromStdString(node->getEntry()->getStringFromMemory()));
  else if (node->isGroup() && index.column() == MemWatchModel::WATCH_COL_LABEL)
    lineEditor->setText(node->getGroupName());
  else
    QStyledItemDelegate::setEditorData(editor, index);
}

void MemWatchDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const
{
  QLineEdit* lineEditor = static_cast<QLineEdit*>(editor);
  if (!lineEditor->text().isEmpty())
    QStyledItemDelegate::setModelData(editor, model, index);
}

void MemWatchDelegate::destroyEditor(QWidget* editor, const QModelIndex& index) const
{
  const MemWatchModel* model = static_cast<const MemWatchModel*>(index.model());
  MemWatchTreeNode* node = model->getTreeNodeFromIndex(index);
  if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
  {
    GUICommon::g_valueEditing = false;
    node->setValueEditing(false);
  }
  editor->deleteLater();
}