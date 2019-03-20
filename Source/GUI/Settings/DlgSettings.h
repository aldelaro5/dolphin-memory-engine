#pragma once

#include <QDialog>
#include <QSpinBox>

#include <QDialogButtonBox>

class DlgSettings : public QDialog
{
public:
  DlgSettings(QWidget* parent = nullptr);
  ~DlgSettings();

private:
  void loadSettings();
  void saveSettings() const;

  QSpinBox* spnWatcherUpdateTimerMs;
  QSpinBox* spnScannerUpdateTimerMs;
  QSpinBox* spnViewerUpdateTimerMs;
  QSpinBox* spnFreezeTimerMs;
  QDialogButtonBox* m_buttonsDlg;
};
