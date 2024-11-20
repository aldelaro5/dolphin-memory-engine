#include "MemWatchDelegate.h"

#include <QApplication>
#include <QFontDatabase>
#include <QLineEdit>
#include <QPainter>

#include "../../MemoryWatch/MemWatchTreeNode.h"
#include "../GUICommon.h"
#include "MemWatchModel.h"

QWidget* MemWatchDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
  (void)option;

  MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
  QLineEdit* editor = new QLineEdit(parent);
  if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
  {
    editor->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
    node->setValueEditing(true);
  }
  GUICommon::g_valueEditing = true;
  return editor;
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
  MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
  if (index.column() == MemWatchModel::WATCH_COL_VALUE && !node->isGroup())
    node->setValueEditing(false);
  GUICommon::g_valueEditing = false;
  editor->deleteLater();
}

void MemWatchDelegate::paint(QPainter* const painter, const QStyleOptionViewItem& option_,
                             const QModelIndex& index) const
{
  QStyleOptionViewItem option = option_;
  initStyleOption(&option, index);

  const int column{index.column()};
  if (column == MemWatchModel::WATCH_COL_LOCK)
  {
    MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
    MemWatchEntry* const entry{node->getEntry()};
    if (entry && !GUICommon::isContainerType(entry->getType()))
    {
      QStyleOptionButton checkboxstyle;
      QRect checkbox_rect =
          QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &checkboxstyle);
      checkboxstyle.rect = option.rect;
      checkboxstyle.rect.setLeft(option.rect.x() + option.rect.width() / 2 -
                                 checkbox_rect.width() / 2);

      const bool checked{entry->isLocked()};
      checkboxstyle.state = checked ? QStyle::State_On : QStyle::State_Off;
      const bool enabled{static_cast<bool>(option.state & QStyle::State_Enabled)};
      if (enabled)
      {
        checkboxstyle.state |= QStyle::State_Enabled;
      }
      checkboxstyle.palette = option.palette;

      QStyledItemDelegate::paint(painter, option, index);
      QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkboxstyle, painter);
      return;
    }
  }

  QStyledItemDelegate::paint(painter, option, index);
}

bool MemWatchDelegate::editorEvent(QEvent* const event, QAbstractItemModel* const model,
                                   const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const QEvent::Type eventType{event->type()};

  const int column{index.column()};
  if (column == MemWatchModel::WATCH_COL_LOCK)
  {
    if (eventType == QEvent::MouseButtonDblClick)
      return true;  // Consume event

    if (eventType == QEvent::MouseButtonRelease)
    {
      MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
      MemWatchEntry* const entry{node->getEntry()};
      if (entry)
      {
        entry->setLock(!entry->isLocked());
        return true;
      }
    }
  }

  return QStyledItemDelegate::editorEvent(event, model, option, index);
}
