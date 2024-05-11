#pragma once

#include <string_view>

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
  explicit DlgCopy(QWidget* parent = nullptr);
  ~DlgCopy() override;

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

  static bool hexStringToU32(std::string_view str, u32& output);
  static bool isHexString(std::string_view str);
  static bool isUnsignedIntegerString(std::string_view str);
  static bool uintStringToU32(std::string_view str, u32& output);
  static std::string charToHexString(const char* input, size_t count, ByteStringFormats format);

  QLineEdit* m_spnWatcherCopyAddress;
  QLineEdit* m_spnWatcherCopySize;
  QTextEdit* m_spnWatcherCopyOutput;
  QComboBox* m_cmbViewerBytesSeparator;
  QDialogButtonBox* m_buttonsDlg;

  std::vector<char> m_Data;
};
