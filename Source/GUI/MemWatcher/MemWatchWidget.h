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
  MemWatchWidget(QWidget* parent);

  void onMemWatchContextMenuRequested(const QPoint& pos);
  void onValueWriteError(const QModelIndex& index, Common::MemOperationReturnCode writeReturn);
  void onWatchDoubleClicked(const QModelIndex& index);
  void onAddGroup();
  void onAddWatchEntry();
  void addWatchEntry(MemWatchEntry* entry);
  void onDeleteNode();
  void onDropSucceeded();
  void openWatchFile();
  void saveWatchFile();
  void saveAsWatchFile();
  void exportWatchListAsCSV();
  QTimer* getUpdateTimer() const;
  QTimer* getFreezeTimer() const;
  bool warnIfUnsavedChanges();

signals:
  void mustUnhook();
  void goToAddressInViewer(u32 address);

private:
  QTreeView* m_watchView;
  MemWatchModel* m_watchModel;
  MemWatchDelegate* m_watchDelegate;
  QPushButton* m_btnAddGroup;
  QPushButton* m_btnAddWatchEntry;
  QTimer* m_updateTimer;
  QTimer* m_freezeTimer;

  QString m_watchListFile = "";
  bool m_hasUnsavedChanges = false;
};