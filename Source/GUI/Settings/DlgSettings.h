#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QSlider>
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

  QComboBox* m_cmbTheme;
  QSpinBox* m_spnWatcherUpdateTimerMs;
  QSpinBox* m_spnScannerUpdateTimerMs;
  QSpinBox* m_spnViewerUpdateTimerMs;
  QSpinBox* m_spnFreezeTimerMs;
  QComboBox* m_cmbViewerBytesSeparator;
  QDialogButtonBox* m_buttonsDlg;
  QSlider* m_sldMEM1Size;
  QSlider* m_sldMEM2Size;
  QLabel* m_lblMEM1Size;
  QLabel* m_lblMEM2Size;
};
