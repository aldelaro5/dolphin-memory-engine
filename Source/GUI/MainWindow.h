#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QMainWindow>
#include <QMenu>

#include "../Common/CommonTypes.h"
#include "../Common/MemoryCommon.h"
#include "MemCopy/DlgCopy.h"
#include "MemScanner/MemScanWidget.h"
#include "MemViewer/MemViewerWidget.h"
#include "MemWatcher/MemWatchWidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();
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
  void onOpenMenViewer();
  void onOpenMemViewerWithAddress(u32 address);
  void updateMem2Status();

  void onOpenWatchFile();
  void onSaveWatchFile();
  void onSaveAsWatchFile();
  void onClearWatchList();
  void onOpenSettings();
  void onImportFromCT();
  void onExportAsCSV();
  void onCopyMemory();
  void onAbout();
  void onQuit();

private:
  void makeMenus();
  void initialiseWidgets();
  void makeLayouts();
  void makeMemViewer();
  void firstHookAttempt();

  MemWatchWidget* m_watcher;
  MemScanWidget* m_scanner;
  MemViewerWidget* m_viewer;
  DlgCopy* m_copier;

  QLabel* m_lblDolphinStatus;
  QLabel* m_lblMem2Status;

  QMenu* m_menuFile;
  QMenu* m_menuEdit;
  QMenu* m_menuDolphin;
  QMenu* m_menuView;
  QMenu* m_menuHelp;
  QAction* m_actOpenWatchList;
  QAction* m_actSaveWatchList;
  QAction* m_actSaveAsWatchList;
  QAction* m_actClearWatchList;
  QAction* m_actImportFromCT;
  QAction* m_actExportAsCSV;
  QAction* m_actSettings;
  QAction* m_actHook;
  QAction* m_actUnhook;
  QAction* m_actMemoryViewer;
  QAction* m_actCopyMemory;
  QAction* m_actQuit;
  QAction* m_actAbout;
};
