#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QMainWindow>
#include <QMenu>

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"
#include "MemScanner/MemScanWidget.h"
#include "MemViewer/MemViewerWidget.h"
#include "MemWatcher/MemWatchWidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  void closeEvent(QCloseEvent* event) override;
  void addWatchRequested(u32 address, Common::MemType type, size_t length, bool isUnsigned,
                         Common::MemBase base);
  void addSelectedResultsToWatchList(Common::MemType type, size_t length, bool isUnsigned,
                                     Common::MemBase base);
  void addAllResultsToWatchList(Common::MemType type, size_t length, bool isUnsigned,
                                Common::MemBase base);
  void updateDolphinHookingStatus();
  void onHookAttempt();
  void onUnhook();
  void onAutoDetectMem2();
  void onToggleMem2();
  void onOpenMenViewer();
  void onOpenMemViewerWithAddress(u32 address);
  void updateMem2Status();

  void onOpenWatchFile();
  void onSaveWatchFile();
  void onSaveAsWatchFile();
  void onExportAsCSV();
  void onAbout();
  void onQuit();

private:
  QString* m_strMem2Info;

  MemWatchWidget* m_watcher;
  MemScanWidget* m_scanner;
  MemViewerWidget* m_viewer;

  QLabel* m_lblDolphinStatus;
  QPushButton* m_btnAttempHook;
  QPushButton* m_btnUnhook;
  QLabel* m_lblMem2Status;
  QPushButton* m_btnToggleMem2;
  QPushButton* m_btnMem2AutoDetect;
  QPushButton* m_btnOpenMemViewer;
  QWidget* m_mem2StatusWidget;

  QMenu* m_menuFile;
  QMenu* m_menuView;
  QMenu* m_menuHelp;
  QAction* m_actOpenWatchList;
  QAction* m_actSaveWatchList;
  QAction* m_actSaveAsWatchList;
  QAction* m_actExportAsCSV;
  QAction* m_actViewScanner;
  QAction* m_actQuit;
  QAction* m_actAbout;
};