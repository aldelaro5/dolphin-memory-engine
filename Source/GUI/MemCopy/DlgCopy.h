#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <qlineedit.h>
#include <qtextedit.h>

#include "../GUICommon.h"

class DlgCopy : public QDialog
{
public:
  DlgCopy(QWidget* parent = nullptr);
  ~DlgCopy();

private:
  enum ByteStringFormats
  {
    ByteString = 0,
    ByteStringNoSpaces = 1,
    PythonByteString = 2,
    PythonList = 3,
    CArray = 4
  };

  void setDefaults();
  void enablePage(bool enable);
  bool copyMemory();
  void updateMemoryText();

  static bool hexStringToU32(std::string str, u32& output);
  static bool isHexString(std::string str);
  static bool isUnsignedIntegerString(std::string str);
  static bool uintStringToU32(std::string str, u32& output);
  static std::string charToHexString(char* input, size_t count, ByteStringFormats format);

  QLineEdit* m_spnWatcherCopyAddress;
  QLineEdit* m_spnWatcherCopySize;
  QTextEdit* m_spnWatcherCopyOutput;
  QComboBox* m_cmbViewerBytesSeparator;
  QDialogButtonBox* m_buttonsDlg;

  std::vector<char> m_Data;
};
