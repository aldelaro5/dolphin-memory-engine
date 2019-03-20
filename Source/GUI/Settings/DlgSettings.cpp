#include "DlgSettings.h"

#include <QAbstractButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include "SConfig.h"

DlgSettings::DlgSettings(QWidget* parent) : QDialog(parent)
{
  QGroupBox* grbTimerSettings = new QGroupBox("Timer settings");

  spnWatcherUpdateTimerMs = new QSpinBox();
  spnWatcherUpdateTimerMs->setMinimum(1);
  spnWatcherUpdateTimerMs->setMaximum(10000);
  spnScannerUpdateTimerMs = new QSpinBox();
  spnScannerUpdateTimerMs->setMinimum(1);
  spnScannerUpdateTimerMs->setMaximum(10000);
  spnViewerUpdateTimerMs = new QSpinBox();
  spnViewerUpdateTimerMs->setMinimum(1);
  spnViewerUpdateTimerMs->setMaximum(10000);
  spnFreezeTimerMs = new QSpinBox();
  spnFreezeTimerMs->setMinimum(1);
  spnFreezeTimerMs->setMaximum(10000);

  QFormLayout* timerSettingsInputLayout = new QFormLayout();
  timerSettingsInputLayout->addRow("Watcher update timer (ms)", spnWatcherUpdateTimerMs);
  timerSettingsInputLayout->addRow("Scanner results update timer (ms)", spnScannerUpdateTimerMs);
  timerSettingsInputLayout->addRow("Memory viewer update timer (ms)", spnViewerUpdateTimerMs);
  timerSettingsInputLayout->addRow("Address value lock timer (ms)", spnFreezeTimerMs);
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

  m_buttonsDlg = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  connect(m_buttonsDlg, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonsDlg, &QDialogButtonBox::clicked, this, [=](QAbstractButton* button) {
    if (m_buttonsDlg->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
      saveSettings();
      QDialog::accept();
    }
  });

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(grbTimerSettings);
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
  spnWatcherUpdateTimerMs->setValue(SConfig::getInstance().getWatcherUpdateTimerMs());
  spnScannerUpdateTimerMs->setValue(SConfig::getInstance().getScannerUpdateTimerMs());
  spnViewerUpdateTimerMs->setValue(SConfig::getInstance().getViewerUpdateTimerMs());
  spnFreezeTimerMs->setValue(SConfig::getInstance().getFreezeTimerMs());
}

void DlgSettings::saveSettings() const
{
  SConfig::getInstance().setWatcherUpdateTimerMs(spnWatcherUpdateTimerMs->value());
  SConfig::getInstance().setScannerUpdateTimerMs(spnScannerUpdateTimerMs->value());
  SConfig::getInstance().setViewerUpdateTimerMs(spnViewerUpdateTimerMs->value());
  SConfig::getInstance().setFreezeTimerMs(spnFreezeTimerMs->value());
}
