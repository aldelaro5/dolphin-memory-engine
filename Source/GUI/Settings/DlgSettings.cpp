#include "DlgSettings.h"

#include <QAbstractButton>
#include <QApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include "../GUICommon.h"
#include "SConfig.h"

DlgSettings::DlgSettings(QWidget* parent) : QDialog(parent)
{
  QGroupBox* grbCoreSettings = new QGroupBox("Core settings");
  m_cmbTheme = new QComboBox();
  m_cmbTheme->addItem("System", static_cast<int>(GUICommon::ApplicationStyle::System));
  m_cmbTheme->addItem("Light", static_cast<int>(GUICommon::ApplicationStyle::Light));
  m_cmbTheme->addItem("Dark Gray", static_cast<int>(GUICommon::ApplicationStyle::DarkGray));
  m_cmbTheme->addItem("Dark", static_cast<int>(GUICommon::ApplicationStyle::Dark));
  connect(m_cmbTheme, &QComboBox::currentIndexChanged, this, [](const int index) {
    GUICommon::changeApplicationStyle(static_cast<GUICommon::ApplicationStyle>(index));
  });

  QFormLayout* coreSettingsInputLayout = new QFormLayout();
  coreSettingsInputLayout->addRow("Theme", m_cmbTheme);

  grbCoreSettings->setLayout(coreSettingsInputLayout);

  QGroupBox* grbTimerSettings = new QGroupBox("Timer settings");

  m_spnWatcherUpdateTimerMs = new QSpinBox();
  m_spnWatcherUpdateTimerMs->setMinimum(1);
  m_spnWatcherUpdateTimerMs->setMaximum(10000);
  m_spnScannerUpdateTimerMs = new QSpinBox();
  m_spnScannerUpdateTimerMs->setMinimum(1);
  m_spnScannerUpdateTimerMs->setMaximum(10000);
  m_spnViewerUpdateTimerMs = new QSpinBox();
  m_spnViewerUpdateTimerMs->setMinimum(1);
  m_spnViewerUpdateTimerMs->setMaximum(10000);
  m_spnFreezeTimerMs = new QSpinBox();
  m_spnFreezeTimerMs->setMinimum(1);
  m_spnFreezeTimerMs->setMaximum(10000);

  QFormLayout* timerSettingsInputLayout = new QFormLayout();
  timerSettingsInputLayout->addRow("Watcher update timer (ms)", m_spnWatcherUpdateTimerMs);
  timerSettingsInputLayout->addRow("Scanner results update timer (ms)", m_spnScannerUpdateTimerMs);
  timerSettingsInputLayout->addRow("Memory viewer update timer (ms)", m_spnViewerUpdateTimerMs);
  timerSettingsInputLayout->addRow("Address value lock timer (ms)", m_spnFreezeTimerMs);
  timerSettingsInputLayout->setLabelAlignment(Qt::AlignRight);

  QLabel* lblTimerSettingsDescription = new QLabel(
      "These settings changes the time in miliseconds it takes for updates to be fetched from "
      "Dolphin. The lower these values are, the more frequant updates will happen, but the more "
      "likely it will increase lag in the program especially on large watches list. For the "
      "address value lock timer, it sets how long it will take before settings the value in "
      "Dolphin.");
  lblTimerSettingsDescription->setWordWrap(true);

  QVBoxLayout* timerSettingsLayout = new QVBoxLayout;
  timerSettingsLayout->addWidget(lblTimerSettingsDescription);
  timerSettingsLayout->addLayout(timerSettingsInputLayout);

  grbTimerSettings->setLayout(timerSettingsLayout);

  QGroupBox* grbScannerSettings = new QGroupBox("Scanner settings");

  m_spnScannerShowThreshold = new QSpinBox(this);
  m_spnScannerShowThreshold->setMinimum(1);
  m_spnScannerShowThreshold->setMaximum(100000);

  QFormLayout* scannerSettingsInputLayout = new QFormLayout(grbScannerSettings);
  scannerSettingsInputLayout->addRow("Show Threshold", m_spnScannerShowThreshold);

  QGroupBox* grbViewerSettings = new QGroupBox("Viewer settings");

  m_cmbViewerBytesSeparator = new QComboBox();
  m_cmbViewerBytesSeparator->addItem("No separator", 0);
  m_cmbViewerBytesSeparator->addItem("Separate every byte", 1);
  m_cmbViewerBytesSeparator->addItem("Separate every 2 bytes", 2);
  m_cmbViewerBytesSeparator->addItem("Separate every 4 bytes", 4);
  m_cmbViewerBytesSeparator->addItem("Separate every 8 bytes", 8);

  QFormLayout* viewerSettingsInputLayout = new QFormLayout();
  viewerSettingsInputLayout->addRow("Bytes separators setting", m_cmbViewerBytesSeparator);

  QVBoxLayout* viewerSettingsLayout = new QVBoxLayout;
  viewerSettingsLayout->addLayout(viewerSettingsInputLayout);

  grbViewerSettings->setLayout(viewerSettingsLayout);

  m_buttonsDlg = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  connect(m_buttonsDlg, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonsDlg, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button) {
    if (m_buttonsDlg->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
      saveSettings();
      QDialog::accept();
    }
  });

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(grbCoreSettings);
  mainLayout->addWidget(grbTimerSettings);
  mainLayout->addWidget(grbScannerSettings);
  mainLayout->addWidget(grbViewerSettings);
  mainLayout->addWidget(m_buttonsDlg);
  setLayout(mainLayout);

  setWindowTitle(tr("Settings"));

  loadSettings();
}

DlgSettings::~DlgSettings()
{
  delete m_buttonsDlg;
}

void DlgSettings::loadSettings()
{
  m_spnWatcherUpdateTimerMs->setValue(SConfig::getInstance().getWatcherUpdateTimerMs());
  m_spnScannerUpdateTimerMs->setValue(SConfig::getInstance().getScannerUpdateTimerMs());
  m_spnViewerUpdateTimerMs->setValue(SConfig::getInstance().getViewerUpdateTimerMs());
  m_spnFreezeTimerMs->setValue(SConfig::getInstance().getFreezeTimerMs());
  m_spnScannerShowThreshold->setValue(SConfig::getInstance().getScannerShowThreshold());
  m_cmbViewerBytesSeparator->setCurrentIndex(
      m_cmbViewerBytesSeparator->findData(SConfig::getInstance().getViewerNbrBytesSeparator()));
  m_cmbTheme->setCurrentIndex(m_cmbTheme->findData(SConfig::getInstance().getTheme()));
}

void DlgSettings::saveSettings() const
{
  SConfig::getInstance().setWatcherUpdateTimerMs(m_spnWatcherUpdateTimerMs->value());
  SConfig::getInstance().setScannerUpdateTimerMs(m_spnScannerUpdateTimerMs->value());
  SConfig::getInstance().setViewerUpdateTimerMs(m_spnViewerUpdateTimerMs->value());
  SConfig::getInstance().setFreezeTimerMs(m_spnFreezeTimerMs->value());
  SConfig::getInstance().setScannerShowThreshold(m_spnScannerShowThreshold->value());
  SConfig::getInstance().setViewerNbrBytesSeparator(
      m_cmbViewerBytesSeparator->currentData().toInt());
  SConfig::getInstance().setTheme(m_cmbTheme->currentData().toInt());
}
