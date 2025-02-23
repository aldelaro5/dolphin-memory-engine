#pragma once

#include <QPushButton>
#include <QTimer>
#include <QTreeView>

#include "MemWatchDelegate.h"
#include "MemWatchModel.h"

class MemWatchWidget : public QWidget
{
  Q_OBJECT

public:
  explicit MemWatchWidget(QWidget* parent);
  ~MemWatchWidget() override;

  MemWatchWidget(const MemWatchWidget&) = delete;
  MemWatchWidget(MemWatchWidget&&) = delete;
  MemWatchWidget& operator=(const MemWatchWidget&) = delete;
  MemWatchWidget& operator=(MemWatchWidget&&) = delete;

  void onMemWatchContextMenuRequested(const QPoint& pos);
  void onDataEdited(const QModelIndex& index, const QVariant& value, int role);
  void onValueWriteError(const QModelIndex& index, Common::MemOperationReturnCode writeReturn);
  void onWatchDoubleClicked(const QModelIndex& index);
  void onAddGroup();
  void onAddWatchEntry();
  void addWatchEntry(MemWatchEntry* entry);
  void onLockSelection(bool lockStatus);
  void onDeleteSelection();
  void onDropSucceeded();
  void onRowsInserted(const QModelIndex& parent, int first, int last);
  void onCollapsed(const QModelIndex& index);
  void onExpanded(const QModelIndex& index);
  void openWatchFile(const QString& fileName);
  void setSelectedWatchesBase(MemWatchEntry* entry, Common::MemBase base);
  void groupCurrentSelection();
  void copySelectedWatchesToClipBoard();
  void cutSelectedWatchesToClipBoard();
  void pasteWatchFromClipBoard(const QModelIndex& referenceIndex);
  bool saveWatchFile();
  bool saveAsWatchFile();
  void clearWatchList();
  void importFromCTFile();
  void exportWatchListAsCSV();
  QTimer* getUpdateTimer() const;
  QTimer* getFreezeTimer() const;
  bool warnIfUnsavedChanges();
  void restoreWatchModel(const QString& json);
  QString saveWatchModel();
  QString m_watchListFile;

signals:
  void mustUnhook();
  void goToAddressInViewer(u32 address);

private:
  void initialiseWidgets();
  void makeLayouts();
  void updateExpansionState(const MemWatchTreeNode* node = nullptr);

  QTreeView* m_watchView{};
  MemWatchModel* m_watchModel{};
  MemWatchDelegate* m_watchDelegate{};
  QPushButton* m_btnAddGroup{};
  QPushButton* m_btnAddWatchEntry{};
  QTimer* m_updateTimer{};
  QTimer* m_freezeTimer{};
  bool m_hasUnsavedChanges = false;

  bool isAnyAncestorSelected(const QModelIndex& index) const;
  QModelIndexList simplifySelection() const;
};
