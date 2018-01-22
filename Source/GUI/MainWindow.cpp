#include "MainWindow.h"

#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QString>
#include <QVBoxLayout>
#include <string>

#include "../DolphinProcess/DolphinAccessor.h"
#include "../MemoryWatch/MemWatchEntry.h"

MainWindow::MainWindow()
{
  initialiseWidgets();
  makeLayouts();
  makeMenus();
  DolphinComm::DolphinAccessor::init();
  makeMemViewer();
  firstHookAttempt();
}

MainWindow::~MainWindow()
{
  delete m_viewer;
  delete m_scanner;
  delete m_watcher;
}

void MainWindow::makeMenus()
{
  m_actOpenWatchList = new QAction("&Open...", this);
  m_actSaveWatchList = new QAction("&Save", this);
  m_actSaveAsWatchList = new QAction("&Save as...", this);
  m_actClearWatchList = new QAction("&Clear the watch list", this);
  m_actImportFromCT = new QAction("&Import from Cheat Engine's CT file...", this);
  m_actExportAsCSV = new QAction("&Export as CSV...", this);

  m_actViewScanner = new QAction("&Scanner", this);
  m_actViewScanner->setCheckable(true);
  m_actViewScanner->setChecked(true);

  m_actQuit = new QAction("&Quit", this);
  m_actAbout = new QAction("&About", this);
  connect(m_actOpenWatchList, &QAction::triggered, this, &MainWindow::onOpenWatchFile);
  connect(m_actSaveWatchList, &QAction::triggered, this, &MainWindow::onSaveWatchFile);
  connect(m_actSaveAsWatchList, &QAction::triggered, this, &MainWindow::onSaveAsWatchFile);
  connect(m_actClearWatchList, &QAction::triggered, this, &MainWindow::onClearWatchList);
  connect(m_actImportFromCT, &QAction::triggered, this, &MainWindow::onImportFromCT);
  connect(m_actExportAsCSV, &QAction::triggered, this, &MainWindow::onExportAsCSV);

  connect(m_actViewScanner, static_cast<void (QAction::*)(bool)>(&QAction::toggled), this, [=] {
    if (m_actViewScanner->isChecked())
      m_scanner->show();
    else
      m_scanner->hide();
  });

  connect(m_actQuit, &QAction::triggered, this, &MainWindow::onQuit);
  connect(m_actAbout, &QAction::triggered, this, &MainWindow::onAbout);

  m_menuFile = menuBar()->addMenu("&File");
  m_menuFile->addAction(m_actOpenWatchList);
  m_menuFile->addAction(m_actSaveWatchList);
  m_menuFile->addAction(m_actSaveAsWatchList);
  m_menuFile->addAction(m_actClearWatchList);
  m_menuFile->addAction(m_actImportFromCT);
  m_menuFile->addAction(m_actExportAsCSV);
  m_menuFile->addAction(m_actQuit);

  m_menuView = menuBar()->addMenu("&View");
  m_menuView->addAction(m_actViewScanner);

  m_menuHelp = menuBar()->addMenu("&Help");
  m_menuHelp->addAction(m_actAbout);
}

void MainWindow::initialiseWidgets()
{
  m_scanner = new MemScanWidget(this);
  connect(m_scanner,
          static_cast<void (MemScanWidget::*)(u32 address, Common::MemType type, size_t length,
                                              bool isUnsigned, Common::MemBase base)>(
              &MemScanWidget::requestAddWatchEntry),
          this,
          static_cast<void (MainWindow::*)(u32 address, Common::MemType type, size_t length,
                                           bool isUnsigned, Common::MemBase base)>(
              &MainWindow::addWatchRequested));
  connect(m_scanner,
          static_cast<void (MemScanWidget::*)(Common::MemType type, size_t length, bool isUnsigned,
                                              Common::MemBase base)>(
              &MemScanWidget::requestAddAllResultsToWatchList),
          this,
          static_cast<void (MainWindow::*)(Common::MemType type, size_t length, bool isUnsigned,
                                           Common::MemBase base)>(
              &MainWindow::addAllResultsToWatchList));
  connect(m_scanner,
          static_cast<void (MemScanWidget::*)(Common::MemType type, size_t length, bool isUnsigned,
                                              Common::MemBase base)>(
              &MemScanWidget::requestAddSelectedResultsToWatchList),
          this,
          static_cast<void (MainWindow::*)(Common::MemType type, size_t length, bool isUnsigned,
                                           Common::MemBase base)>(
              &MainWindow::addSelectedResultsToWatchList));

  m_watcher = new MemWatchWidget(this);

  connect(m_scanner, &MemScanWidget::mustUnhook, this, &MainWindow::onUnhook);
  connect(m_watcher, &MemWatchWidget::mustUnhook, this, &MainWindow::onUnhook);

  m_btnAttempHook = new QPushButton("Hook");
  m_btnUnhook = new QPushButton("Unhook");
  connect(m_btnAttempHook, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MainWindow::onHookAttempt);
  connect(m_btnUnhook, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MainWindow::onUnhook);

  m_lblDolphinStatus = new QLabel("");
  m_lblDolphinStatus->setAlignment(Qt::AlignHCenter);

  m_lblMem2Status = new QLabel("");
  m_lblMem2Status->setAlignment(Qt::AlignHCenter);

  m_btnOpenMemViewer = new QPushButton("Open memory viewer");
  connect(m_btnOpenMemViewer, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MainWindow::onOpenMenViewer);
}

void MainWindow::makeLayouts()
{
  QHBoxLayout* dolphinHookButtons_layout = new QHBoxLayout();
  dolphinHookButtons_layout->addWidget(m_btnAttempHook);
  dolphinHookButtons_layout->addWidget(m_btnUnhook);

  QFrame* separatorline = new QFrame();
  separatorline->setFrameShape(QFrame::HLine);

  QVBoxLayout* main_layout = new QVBoxLayout;
  main_layout->addWidget(m_lblDolphinStatus);
  main_layout->addLayout(dolphinHookButtons_layout);
  main_layout->addWidget(m_lblMem2Status);
  main_layout->addWidget(separatorline);
  main_layout->addWidget(m_scanner);
  main_layout->addSpacing(5);
  main_layout->addWidget(m_btnOpenMemViewer);
  main_layout->addSpacing(5);
  main_layout->addWidget(m_watcher);

  QWidget* main_widget = new QWidget(this);
  main_widget->setLayout(main_layout);
  setCentralWidget(main_widget);

  setMinimumWidth(800);
}

void MainWindow::makeMemViewer()
{
  m_viewer = new MemViewerWidget(nullptr, Common::MEM1_START);
  connect(m_viewer, &MemViewerWidget::mustUnhook, this, &MainWindow::onUnhook);
  connect(m_watcher,
          static_cast<void (MemWatchWidget::*)(u32)>(&MemWatchWidget::goToAddressInViewer), this,
          static_cast<void (MainWindow::*)(u32)>(&MainWindow::onOpenMemViewerWithAddress));
}

void MainWindow::firstHookAttempt()
{
  onHookAttempt();
  if (DolphinComm::DolphinAccessor::getStatus() ==
      DolphinComm::DolphinAccessor::DolphinStatus::hooked)
    updateMem2Status();
}

void MainWindow::addSelectedResultsToWatchList(Common::MemType type, size_t length, bool isUnsigned,
                                               Common::MemBase base)
{
  QModelIndexList selection = m_scanner->getSelectedResults();
  for (int i = 0; i < selection.count(); i++)
  {
    u32 address = m_scanner->getResultListModel()->getResultAddress(i);
    MemWatchEntry* newEntry =
        new MemWatchEntry("No label", address, type, base, isUnsigned, length);
    m_watcher->addWatchEntry(newEntry);
  }
}

void MainWindow::addAllResultsToWatchList(Common::MemType type, size_t length, bool isUnsigned,
                                          Common::MemBase base)
{
  for (auto item : m_scanner->getAllResults())
  {
    MemWatchEntry* newEntry = new MemWatchEntry("No label", item, type, base, isUnsigned, length);
    m_watcher->addWatchEntry(newEntry);
  }
}

void MainWindow::addWatchRequested(u32 address, Common::MemType type, size_t length,
                                   bool isUnsigned, Common::MemBase base)
{
  MemWatchEntry* newEntry = new MemWatchEntry("No label", address, type, base, isUnsigned, length);
  m_watcher->addWatchEntry(newEntry);
}

void MainWindow::onOpenMenViewer()
{
  m_viewer->show();
  m_viewer->raise();
}

void MainWindow::onOpenMemViewerWithAddress(u32 address)
{
  m_viewer->goToAddress(address);
  m_viewer->show();
}

void MainWindow::updateMem2Status()
{
  if (DolphinComm::DolphinAccessor::isMEM2Present())
    m_lblMem2Status->setText("The extended Wii-only memory is present");
  else
    m_lblMem2Status->setText("The extended Wii-only memory is absent");
  m_viewer->onMEM2StatusChanged(DolphinComm::DolphinAccessor::isMEM2Present());
}

void MainWindow::updateDolphinHookingStatus()
{
  switch (DolphinComm::DolphinAccessor::getStatus())
  {
  case DolphinComm::DolphinAccessor::DolphinStatus::hooked:
  {
    m_lblDolphinStatus->setText(
        "Hooked successfully to Dolphin, current start address: " +
        QString::number(DolphinComm::DolphinAccessor::getEmuRAMAddressStart(), 16).toUpper());
    m_scanner->setEnabled(true);
    m_watcher->setEnabled(true);
    m_btnOpenMemViewer->setEnabled(true);
    m_btnAttempHook->hide();
    m_btnUnhook->show();
    break;
  }
  case DolphinComm::DolphinAccessor::DolphinStatus::notRunning:
  {
    m_lblDolphinStatus->setText("Cannot hook to Dolphin, the process is not running");
    m_scanner->setDisabled(true);
    m_watcher->setDisabled(true);
    m_btnOpenMemViewer->setDisabled(true);
    m_btnAttempHook->show();
    m_btnUnhook->hide();
    break;
  }
  case DolphinComm::DolphinAccessor::DolphinStatus::noEmu:
  {
    m_lblDolphinStatus->setText(
        "Cannot hook to Dolphin, the process is running, but no emulation has been started");
    m_scanner->setDisabled(true);
    m_watcher->setDisabled(true);
    m_btnOpenMemViewer->setDisabled(true);
    m_btnAttempHook->show();
    m_btnUnhook->hide();
    break;
  }
  case DolphinComm::DolphinAccessor::DolphinStatus::unHooked:
  {
    m_lblDolphinStatus->setText("Unhooked, press \"Hook\" to hook to Dolphin again");
    m_scanner->setDisabled(true);
    m_watcher->setDisabled(true);
    m_btnOpenMemViewer->setDisabled(true);
    m_btnAttempHook->show();
    m_btnUnhook->hide();
    break;
  }
  }
}

void MainWindow::onHookAttempt()
{
  DolphinComm::DolphinAccessor::hook();
  updateDolphinHookingStatus();
  if (DolphinComm::DolphinAccessor::getStatus() ==
      DolphinComm::DolphinAccessor::DolphinStatus::hooked)
  {
    m_scanner->getUpdateTimer()->start(100);
    m_watcher->getUpdateTimer()->start(10);
    m_watcher->getFreezeTimer()->start(10);
    m_viewer->getUpdateTimer()->start(100);
    m_viewer->hookStatusChanged(true);
    updateMem2Status();
  }
}

void MainWindow::onUnhook()
{
  m_scanner->getUpdateTimer()->stop();
  m_watcher->getUpdateTimer()->stop();
  m_watcher->getFreezeTimer()->stop();
  m_viewer->getUpdateTimer()->stop();
  m_viewer->hookStatusChanged(false);
  m_lblMem2Status->setText(QString(""));
  DolphinComm::DolphinAccessor::unHook();
  updateDolphinHookingStatus();
}

void MainWindow::onOpenWatchFile()
{
  if (m_watcher->warnIfUnsavedChanges())
    m_watcher->openWatchFile();
}

void MainWindow::onSaveWatchFile()
{
  m_watcher->saveWatchFile();
}

void MainWindow::onSaveAsWatchFile()
{
  m_watcher->saveAsWatchFile();
}

void MainWindow::onClearWatchList()
{
  m_watcher->clearWatchList();
}

void MainWindow::onImportFromCT()
{
  m_watcher->importFromCTFile();
}

void MainWindow::onExportAsCSV()
{
  m_watcher->exportWatchListAsCSV();
}

void MainWindow::onAbout()
{
  QMessageBox::about(this, "About Dolphin memory engine",
                     "Beta version 0.4\n\nA RAM search made to facilitate research and "
                     "reverse engineering of GameCube and Wii games using the Dolphin "
                     "emulator.\n\nThis program is licensed under the MIT license. You "
                     "should have received a copy of the MIT license along with this program");
}

void MainWindow::onQuit()
{
  close();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (m_watcher->warnIfUnsavedChanges())
  {
    m_viewer->close();
    event->accept();
  }
  else
  {
    event->ignore();
  }
}
