#include "MainWindow.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenuBar>
#include <QShortcut>
#include <QString>
#include <QVBoxLayout>
#include <string>

#include "../DolphinProcess/DolphinAccessor.h"
#include "../MemoryWatch/MemWatchEntry.h"
#include "GUICommon.h"
#include "Settings/DlgSettings.h"
#include "Settings/SConfig.h"

MainWindow::MainWindow()
{
  setWindowTitle(QApplication::applicationName() + " " + QApplication::applicationVersion());
  setWindowIcon(QIcon(":/logo.svg"));
  initialiseWidgets();
  makeLayouts();
  makeMenus();
  DolphinComm::DolphinAccessor::init();
  makeMemViewer();

  m_autoHookTimer.setInterval(1000);
  connect(&m_autoHookTimer, &QTimer::timeout, this, &MainWindow::onHookIfNotHooked);

  if (SConfig::getInstance().getMainWindowGeometry().size())
    restoreGeometry(SConfig::getInstance().getMainWindowGeometry());
  if (SConfig::getInstance().getMainWindowState().size())
    restoreState(SConfig::getInstance().getMainWindowState());
#ifdef _WIN32
  GUICommon::changeApplicationStyle(SConfig::getInstance().getTheme());
#endif

  m_watcher->restoreWatchModel(SConfig::getInstance().getWatchModel());
  m_actAutoHook->setChecked(SConfig::getInstance().getAutoHook());

  if (m_actAutoHook->isChecked())
    onHookAttempt();
  else
    onUnhook();
}

MainWindow::~MainWindow()
{
  delete m_copier;
  delete m_viewer;
  delete m_watcher;
  DolphinComm::DolphinAccessor::free();
}

void MainWindow::makeMenus()
{
  m_actOpenWatchList = new QAction(tr("&Open..."), this);
  m_actSaveWatchList = new QAction(tr("&Save"), this);
  m_actSaveAsWatchList = new QAction(tr("&Save as..."), this);
  m_actClearWatchList = new QAction(tr("&Clear the watch list"), this);
  m_actImportFromCT = new QAction(tr("&Import from Cheat Engine's CT file..."), this);
  m_actExportAsCSV = new QAction(tr("&Export as CSV..."), this);
  QAction* const actOpenConfigDir{new QAction(tr("Open Configuration Directory..."), this)};

  m_actOpenWatchList->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_O);
  m_actSaveWatchList->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_S);
  m_actSaveAsWatchList->setShortcut(Qt::Modifier::CTRL | Qt::Modifier::SHIFT | Qt::Key::Key_S);
  m_actImportFromCT->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_I);

  m_actSettings = new QAction(tr("&Settings"), this);

  m_actAutoHook = new QAction(tr("&Auto-hook"), this);
  m_actAutoHook->setCheckable(true);
  m_actHook = new QAction(tr("&Hook"), this);
  m_actUnhook = new QAction(tr("&Unhook"), this);

  m_actMemoryViewer = new QAction(tr("&Memory Viewer"), this);
  m_actCopyMemory = new QAction(tr("&Copy Memory Range"), this);
  m_actScanner = new QAction(tr("&Scanner"), this);
  m_actScanner->setShortcut(QKeySequence("F3"));
  m_actScanner->setCheckable(true);
  m_actScanner->setChecked(m_splitter->sizes()[0] > 0);

  m_actQuit = new QAction(tr("&Quit"), this);
  m_actAbout = new QAction(tr("&About"), this);
  connect(m_actOpenWatchList, &QAction::triggered, this, &MainWindow::onOpenWatchFile);
  connect(m_actSaveWatchList, &QAction::triggered, this, &MainWindow::onSaveWatchFile);
  connect(m_actSaveAsWatchList, &QAction::triggered, this, &MainWindow::onSaveAsWatchFile);
  connect(m_actClearWatchList, &QAction::triggered, this, &MainWindow::onClearWatchList);
  connect(m_actImportFromCT, &QAction::triggered, this, &MainWindow::onImportFromCT);
  connect(m_actExportAsCSV, &QAction::triggered, this, &MainWindow::onExportAsCSV);
  connect(actOpenConfigDir, &QAction::triggered, this, []() {
    const QString filepath{SConfig::getInstance().getSettingsFilepath()};
    const QFileInfo fileInfo{filepath};
    const QUrl url{QUrl::fromLocalFile(fileInfo.absolutePath())};
    QDesktopServices::openUrl(url);
  });

  connect(m_actSettings, &QAction::triggered, this, &MainWindow::onOpenSettings);

  connect(m_actAutoHook, &QAction::toggled, this, &MainWindow::onAutoHookToggled);
  connect(m_actHook, &QAction::triggered, this, &MainWindow::onHookAttempt);
  connect(m_actUnhook, &QAction::triggered, this, &MainWindow::onUnhook);

  connect(m_actMemoryViewer, &QAction::triggered, this, &MainWindow::onOpenMenViewer);
  connect(m_actCopyMemory, &QAction::triggered, this, &MainWindow::onCopyMemory);
  connect(m_actScanner, &QAction::toggled, this, &MainWindow::onScannerActionToggled);

  connect(m_actQuit, &QAction::triggered, this, &MainWindow::onQuit);
  connect(m_actAbout, &QAction::triggered, this, &MainWindow::onAbout);

  m_menuFile = menuBar()->addMenu(tr("&File"));
  m_menuFile->addAction(m_actOpenWatchList);
  m_menuFile->addAction(m_actSaveWatchList);
  m_menuFile->addAction(m_actSaveAsWatchList);
  m_menuFile->addAction(m_actClearWatchList);
  m_menuFile->addSeparator();
  m_menuFile->addAction(m_actImportFromCT);
  m_menuFile->addAction(m_actExportAsCSV);
  m_menuFile->addSeparator();
  m_menuFile->addAction(actOpenConfigDir);
  m_menuFile->addSeparator();
  m_menuFile->addAction(m_actQuit);

  m_menuEdit = menuBar()->addMenu(tr("&Edit"));
  m_menuEdit->addAction(m_actSettings);

  m_menuDolphin = menuBar()->addMenu(tr("&Dolphin"));
  m_menuDolphin->addAction(m_actAutoHook);
  m_menuDolphin->addSeparator();
  m_menuDolphin->addAction(m_actHook);
  m_menuDolphin->addAction(m_actUnhook);

  m_menuView = menuBar()->addMenu(tr("&View"));
  m_menuView->addAction(m_actMemoryViewer);
  m_menuView->addAction(m_actCopyMemory);
  m_menuView->addAction(m_actScanner);

  m_menuHelp = menuBar()->addMenu(tr("&Help"));
  m_menuHelp->addAction(m_actAbout);
}

void MainWindow::initialiseWidgets()
{
  m_scanner = new MemScanWidget();
  m_scanner->setShowThreshold(
      static_cast<size_t>(SConfig::getInstance().getScannerShowThreshold()));
  connect(m_scanner, &MemScanWidget::requestAddWatchEntry, this, &MainWindow::addWatchRequested);
  connect(m_scanner, &MemScanWidget::requestAddAllResultsToWatchList, this,
          &MainWindow::addAllResultsToWatchList);
  connect(m_scanner, &MemScanWidget::requestAddSelectedResultsToWatchList, this,
          &MainWindow::addSelectedResultsToWatchList);

  m_watcher = new MemWatchWidget(this);

  connect(m_scanner, &MemScanWidget::mustUnhook, this, &MainWindow::onUnhook);
  connect(m_watcher, &MemWatchWidget::mustUnhook, this, &MainWindow::onUnhook);

  m_copier = new DlgCopy(this);

  m_lblDolphinStatus = new QLabel("");
  m_lblDolphinStatus->setAlignment(Qt::AlignHCenter);

  m_lblMem2Status = new QLabel("");
  m_lblMem2Status->setAlignment(Qt::AlignHCenter);
}

void MainWindow::makeLayouts()
{
  QFrame* separatorline = new QFrame();
  separatorline->setFrameShape(QFrame::HLine);

  m_splitter = new QSplitter(Qt::Vertical);
  m_splitter->addWidget(m_scanner);
  m_splitter->addWidget(m_watcher);

  if (SConfig::getInstance().getSplitterState().size())
    m_splitter->restoreState(SConfig::getInstance().getSplitterState());

  connect(m_splitter, &QSplitter::splitterMoved, this, &MainWindow::onSplitterMoved);

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(m_lblDolphinStatus);
  mainLayout->addWidget(m_lblMem2Status);
  mainLayout->addWidget(separatorline);
  mainLayout->addWidget(m_splitter);

  QWidget* mainWidget = new QWidget();
  mainWidget->setLayout(mainLayout);
  setCentralWidget(mainWidget);
}

void MainWindow::makeMemViewer()
{
  m_viewer = new MemViewerWidget(nullptr);
  connect(m_viewer, &MemViewerWidget::mustUnhook, this, &MainWindow::onUnhook);
  connect(m_viewer, &MemViewerWidget::addWatchRequested, m_watcher, &MemWatchWidget::addWatchEntry);
  connect(m_watcher, &MemWatchWidget::goToAddressInViewer, this,
          &MainWindow::onOpenMemViewerWithAddress);
}

void MainWindow::addSelectedResultsToWatchList(Common::MemType type, size_t length, bool isUnsigned,
                                               Common::MemBase base)
{
  QModelIndexList selection = m_scanner->getSelectedResults();
  for (int i = 0; i < selection.count(); i++)
  {
    u32 address = m_scanner->getResultListModel()->getResultAddress(selection.at(i).row());
    MemWatchEntry* newEntry =
        new MemWatchEntry(tr("No label"), address, type, base, isUnsigned, length);
    m_watcher->addWatchEntry(newEntry);
  }
}

void MainWindow::addAllResultsToWatchList(Common::MemType type, size_t length, bool isUnsigned,
                                          Common::MemBase base)
{
  for (auto item : m_scanner->getAllResults())
  {
    MemWatchEntry* newEntry =
        new MemWatchEntry(tr("No label"), item, type, base, isUnsigned, length);
    m_watcher->addWatchEntry(newEntry);
  }
}

void MainWindow::addWatchRequested(u32 address, Common::MemType type, size_t length,
                                   bool isUnsigned, Common::MemBase base)
{
  MemWatchEntry* newEntry =
      new MemWatchEntry(tr("No label"), address, type, base, isUnsigned, length);
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
  QString strMem2 = QString();
  bool mem2 = DolphinComm::DolphinAccessor::isMEM2Present();
  if (mem2)
    strMem2 = tr("The extended Wii-only memory is present");
  else
    strMem2 = tr("The extended Wii-only memory is absent");

  QString strAram = QString();
  if (!mem2)
  {
    if (DolphinComm::DolphinAccessor::isARAMAccessible())
      strAram = tr(", the ARAM is accessible");
    else
      strAram = tr(", the ARAM is inaccessible, turn off MMU to use it");
  }
  m_lblMem2Status->setText(strMem2 + strAram);
  m_viewer->onMEM2StatusChanged(DolphinComm::DolphinAccessor::isMEM2Present());
}

void MainWindow::updateDolphinHookingStatus()
{
  switch (DolphinComm::DolphinAccessor::getStatus())
  {
  case DolphinComm::DolphinAccessor::DolphinStatus::hooked:
  {
    m_lblDolphinStatus->setText(
        tr("Hooked successfully to Dolphin, current start address: ") +
        QString::number(DolphinComm::DolphinAccessor::getEmuRAMAddressStart(), 16).toUpper());
    m_scanner->setEnabled(true);
    m_watcher->setEnabled(true);
    m_copier->setEnabled(true);
    m_actMemoryViewer->setEnabled(true);
    m_actCopyMemory->setEnabled(true);
    m_actHook->setEnabled(false);
    m_actUnhook->setEnabled(!m_actAutoHook->isChecked());
    break;
  }
  case DolphinComm::DolphinAccessor::DolphinStatus::notRunning:
  {
    m_lblDolphinStatus->setText(tr("Cannot hook to Dolphin, the process is not running"));
    m_scanner->setDisabled(true);
    m_watcher->setDisabled(true);
    m_copier->setDisabled(true);
    m_actMemoryViewer->setDisabled(true);
    m_actCopyMemory->setDisabled(true);
    m_actHook->setEnabled(!m_actAutoHook->isChecked());
    m_actUnhook->setEnabled(false);
    break;
  }
  case DolphinComm::DolphinAccessor::DolphinStatus::noEmu:
  {
    m_lblDolphinStatus->setText(
        tr("Cannot hook to Dolphin, the process is running, but no emulation has been started"));
    m_scanner->setDisabled(true);
    m_watcher->setDisabled(true);
    m_copier->setDisabled(true);
    m_actMemoryViewer->setDisabled(true);
    m_actCopyMemory->setDisabled(true);
    m_actHook->setEnabled(!m_actAutoHook->isChecked());
    m_actUnhook->setEnabled(false);
    break;
  }
  case DolphinComm::DolphinAccessor::DolphinStatus::unHooked:
  {
    m_lblDolphinStatus->setText(tr("Unhooked, press \"Hook\" to hook to Dolphin again"));
    m_scanner->setDisabled(true);
    m_watcher->setDisabled(true);
    m_copier->setDisabled(true);
    m_actMemoryViewer->setDisabled(true);
    m_actCopyMemory->setDisabled(true);
    m_actHook->setEnabled(!m_actAutoHook->isChecked());
    m_actUnhook->setEnabled(false);
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
    m_scanner->getUpdateTimer()->start(SConfig::getInstance().getScannerUpdateTimerMs());
    m_watcher->getUpdateTimer()->start(SConfig::getInstance().getWatcherUpdateTimerMs());
    m_watcher->getFreezeTimer()->start(SConfig::getInstance().getFreezeTimerMs());
    m_viewer->getUpdateTimer()->start(SConfig::getInstance().getViewerUpdateTimerMs());
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

void MainWindow::onAutoHookToggled(const bool checked)
{
  if (checked)
  {
    m_autoHookTimer.start();
    onHookAttempt();
  }
  else
  {
    m_autoHookTimer.stop();
  }

  updateDolphinHookingStatus();
}

void MainWindow::onHookIfNotHooked()
{
  if (DolphinComm::DolphinAccessor::getStatus() !=
      DolphinComm::DolphinAccessor::DolphinStatus::hooked)
  {
    onHookAttempt();
  }
}

void MainWindow::onSplitterMoved(const int pos, const int index)
{
  (void)pos;
  (void)index;

  SConfig::getInstance().setSplitterState(m_splitter->saveState());

  const QList<int> currentSizes{m_splitter->sizes()};
  const int totalSize{std::accumulate(currentSizes.begin(), currentSizes.end(), 0)};
  const double scannerSize{static_cast<double>(currentSizes[0])};
  const bool scannerVisible{scannerSize > 0};
  if (scannerVisible)
  {
    const double scannerFactor{scannerSize / static_cast<double>(totalSize)};
    m_splitter->setProperty("previous_scanner_factor", scannerFactor);
  }

  {
    QSignalBlocker signalBlocker(m_actScanner);
    m_actScanner->setChecked(scannerVisible);
  }
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

void MainWindow::onCopyMemory()
{
  m_copier->show();
  m_copier->raise();
}

void MainWindow::onScannerActionToggled(const bool checked)
{
  const QList<int> currentSizes{m_splitter->sizes()};
  const int totalSize{std::accumulate(currentSizes.begin(), currentSizes.end(), 0)};

  QList<int> sizes;
  if (!checked)
  {
    const double scannerSize{static_cast<double>(currentSizes[0])};
    const double scannerFactor{scannerSize / static_cast<double>(totalSize)};
    m_splitter->setProperty("previous_scanner_factor", scannerFactor);

    sizes << 0 << totalSize;
  }
  else
  {
    const QVariant scannerFactorVariant{m_splitter->property("previous_scanner_factor")};
    const double scannerFactor{scannerFactorVariant.isValid() ? scannerFactorVariant.toDouble() :
                                                                0.5};
    const int scannerSize{static_cast<int>(std::round(scannerFactor * totalSize))};

    sizes << scannerSize << totalSize - scannerSize;
  }

  m_splitter->setSizes(sizes);

  SConfig::getInstance().setSplitterState(m_splitter->saveState());
}

void MainWindow::onOpenSettings()
{
  DlgSettings* dlg = new DlgSettings(this);
  int dlgResult = dlg->exec();
  delete dlg;
  if (dlgResult == QDialog::Accepted)
  {
    m_scanner->getUpdateTimer()->stop();
    m_watcher->getUpdateTimer()->stop();
    m_watcher->getFreezeTimer()->stop();
    m_viewer->getUpdateTimer()->stop();
    if (DolphinComm::DolphinAccessor::getStatus() ==
        DolphinComm::DolphinAccessor::DolphinStatus::hooked)
    {
      m_scanner->getUpdateTimer()->start(SConfig::getInstance().getScannerUpdateTimerMs());
      m_watcher->getUpdateTimer()->start(SConfig::getInstance().getWatcherUpdateTimerMs());
      m_watcher->getFreezeTimer()->start(SConfig::getInstance().getFreezeTimerMs());
      m_viewer->getUpdateTimer()->start(SConfig::getInstance().getViewerUpdateTimerMs());
    }
    m_scanner->setShowThreshold(
        static_cast<size_t>(SConfig::getInstance().getScannerShowThreshold()));
  }
}

void MainWindow::onAbout()
{
  const int fontHeight{fontMetrics().height()};

  QLabel* const logo_label{new QLabel};
  logo_label->setPixmap(windowIcon().pixmap(fontHeight * 10, fontHeight * 10));
  logo_label->setContentsMargins(fontHeight, 0, fontHeight, 0);

  QLabel* const text_label{new QLabel(
      QStringLiteral(R"(
<p><font size="+3">%APPLICATION%</font></p>

<p><small>%QT_VERSION%</small></p>

<br/>

<p>%ABOUT_DESC%</p>
<p>%LICENSE_DESC%</p>

<br/>

<p>
<a href='%URL%'>%SOURCE_CODE%</a> |
<a href='%URL%/blob/master/LICENSE'>%LICENSE%</a> |
<a href='%URL%/graphs/contributors'>%CONTRIBUTORS%</a> |
<a href='%URL%/releases'>%UPDATES%</a>
</p>
)")
          .replace("%APPLICATION%",
                   QApplication::applicationName() + " " + QApplication::applicationVersion())
          .replace("%QT_VERSION%", tr("Built with Qt %1").arg(QT_VERSION_STR))
          .replace("%ABOUT_DESC%",
                   tr("A RAM search made to facilitate research and reverse engineering of "
                      "GameCube and Wii games using the Dolphin Emulator."))
          .replace("%LICENSE_DESC%",
                   tr("This program is licensed under the MIT license. You should have received a "
                      "copy of the MIT license along with this program."))
          .replace("%URL%", "https://github.com/aldelaro5/dolphin-memory-engine")
          .replace("%SOURCE_CODE%", tr("Source Code"))
          .replace("%LICENSE%", tr("License"))
          .replace("%CONTRIBUTORS%", tr("Contributors"))
          .replace("%UPDATES%", tr("Updates")))};
  text_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  text_label->setOpenExternalLinks(true);
  text_label->setWordWrap(true);

  QLabel* const copyright_label{new QLabel(
      QStringLiteral("<small>\u00A9 2017+ %1 Team</small>").arg(QApplication::applicationName()))};
  copyright_label->setAlignment(Qt::AlignCenter);
  copyright_label->setContentsMargins(0, fontHeight * 2, 0, 0);

  QVBoxLayout* const main_layout{new QVBoxLayout};
  QHBoxLayout* const horizontal_layout{new QHBoxLayout};
  main_layout->addLayout(horizontal_layout);
  main_layout->addWidget(copyright_label);
  horizontal_layout->setAlignment(Qt::AlignLeft);
  horizontal_layout->addWidget(logo_label);
  horizontal_layout->addWidget(text_label);

  QDialog dialog(this);
  dialog.setWindowTitle(tr("About %1").arg(QApplication::applicationName()));
  dialog.setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  dialog.setLayout(main_layout);
  dialog.exec();
}

void MainWindow::onQuit()
{
  close();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  SConfig::getInstance().setAutoHook(m_actAutoHook->isChecked());
  SConfig::getInstance().setWatchModel(m_watcher->saveWatchModel());
  SConfig::getInstance().setMainWindowGeometry(saveGeometry());
  SConfig::getInstance().setMainWindowState(saveState());
  m_viewer->close();
  event->accept();
}
