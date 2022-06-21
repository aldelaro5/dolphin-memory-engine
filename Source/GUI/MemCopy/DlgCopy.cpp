#include "DlgCopy.h"

#include "../../DolphinProcess/DolphinAccessor.h"
#include <QAbstractButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <sstream>
#include <utility>
#include <qmessagebox.h>
#include "../../Common/CommonUtils.h"

DlgCopy::DlgCopy(QWidget* parent) : QDialog(parent)
{
  QGroupBox* grbCopySettings = new QGroupBox();

  QVBoxLayout* entireCopyLayout = new QVBoxLayout;

  QFormLayout* copySettingsLayout = new QFormLayout();
  m_spnWatcherCopyAddress = new QLineEdit();
  m_spnWatcherCopyAddress->setMaxLength(8);
  copySettingsLayout->addRow("Base Address", m_spnWatcherCopyAddress);

  m_spnWatcherCopySize = new QLineEdit();
  copySettingsLayout->addRow("Byte Count", m_spnWatcherCopySize);
  copySettingsLayout->setLabelAlignment(Qt::AlignRight);
  entireCopyLayout->addLayout(copySettingsLayout);

  m_cmbViewerBytesSeparator = new QComboBox();
  m_cmbViewerBytesSeparator->addItem("Byte String", ByteStringFormats::ByteString);
  m_cmbViewerBytesSeparator->addItem("Byte String (No Spaces)", ByteStringFormats::ByteStringNoSpaces);
  m_cmbViewerBytesSeparator->addItem("Python Byte String", ByteStringFormats::PythonByteString);
  m_cmbViewerBytesSeparator->addItem("Python List", ByteStringFormats::PythonList);
  m_cmbViewerBytesSeparator->addItem("C Array", ByteStringFormats::CArray);
  copySettingsLayout->addRow("Byte Format", m_cmbViewerBytesSeparator);

  m_spnWatcherCopyOutput = new QTextEdit();
  m_spnWatcherCopyOutput->setWordWrapMode(QTextOption::WrapMode::WrapAnywhere);
  copySettingsLayout->addRow("Output", m_spnWatcherCopyOutput);
  
  grbCopySettings->setLayout(entireCopyLayout);

  m_buttonsDlg = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close);
  m_buttonsDlg->setStyleSheet("* { button-layout: 2 }");

  connect(m_buttonsDlg, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonsDlg, &QDialogButtonBox::clicked, this,
          [=](QAbstractButton* button)
          {
            auto role = m_buttonsDlg->buttonRole(button);
            if (role == QDialogButtonBox::ApplyRole)
            {
              if (DolphinComm::DolphinAccessor::getStatus() !=
                  DolphinComm::DolphinAccessor::DolphinStatus::hooked)
              {
                enablePage(false);
                return;
              }
              copyMemory();
            }
            else if (role == QDialogButtonBox::Close)
            {
              QDialog::close();
            }
          });

  connect(m_cmbViewerBytesSeparator, &QComboBox::currentTextChanged, this,
      [=](const QString& string)
      {
          updateMemoryText();
      });

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(grbCopySettings);
  mainLayout->addWidget(m_buttonsDlg);

  enablePage(DolphinComm::DolphinAccessor::getStatus() ==
             DolphinComm::DolphinAccessor::DolphinStatus::hooked);

  setLayout(mainLayout);

  setWindowTitle(tr("Copy Memory Range"));

  setDefaults();
}

DlgCopy::~DlgCopy()
{
  delete m_buttonsDlg;
}

void DlgCopy::setDefaults()
{
  m_spnWatcherCopyAddress->setText("");
  m_spnWatcherCopySize->setText("");
  m_cmbViewerBytesSeparator->setCurrentIndex(ByteStringFormats::ByteString);
}

void DlgCopy::enablePage(bool enable)
{
  m_cmbViewerBytesSeparator->setEnabled(enable);
  m_spnWatcherCopyAddress->setEnabled(enable);
  m_spnWatcherCopySize->setEnabled(enable);
  m_spnWatcherCopyOutput->setEnabled(enable);
  m_buttonsDlg->setEnabled(enable);
}

bool DlgCopy::copyMemory()
{
  u32 address, count;
  QMessageBox* errorBox;

  if (!hexStringToU32(m_spnWatcherCopyAddress->text().toStdString(), address))
  {
    QString errorMsg =
        tr("The address you entered is invalid, make sure it is an "
           "hexadecimal number between 0x80000000 and 0x817FFFFF");
    if (DolphinComm::DolphinAccessor::isMEM2Present())
      errorMsg.append(tr(" or between 0x90000000 and 0x93FFFFFF"));

    errorBox = new QMessageBox(QMessageBox::Critical, tr("Invalid address"), errorMsg,
                                            QMessageBox::Ok, nullptr);
    errorBox->exec();

    return false;
  }

  if (!uintStringToU32(m_spnWatcherCopySize->text().toStdString(), count))
  {
    if (!hexStringToU32(m_spnWatcherCopySize->text().toStdString(), count))
    {
      errorBox = new QMessageBox(
          QMessageBox::Critical, tr("Invalid value"),
          tr("Please make sure the byte count is a valid number in base10 or hexadecimal.\n"
             "i.e. 5, 10, 12, 16, 0x05, 0xf1, 0x100, 0xab12\n"),
          QMessageBox::Ok, nullptr);
      errorBox->exec();

      return false;
    }
  }

  if (!DolphinComm::DolphinAccessor::isValidConsoleAddress(address) ||
      !DolphinComm::DolphinAccessor::isValidConsoleAddress(address + count))
  {
    errorBox =
        new QMessageBox(QMessageBox::Critical, tr("Error reading bytes"),
                        tr("The suggested range of bytes is invalid."), QMessageBox::Ok, nullptr);
    errorBox->exec();

    return false;
  }
    
  std::vector<char> newData(count);
  
  if (!DolphinComm::DolphinAccessor::readFromRAM(
          Common::dolphinAddrToOffset(address, DolphinComm::DolphinAccessor::isARAMAccessible()),
          newData.data(), newData.size(), false))
  {
    errorBox = new QMessageBox(QMessageBox::Critical, tr("Error reading bytes"),
                               tr("Dolphin was unable to read the bytes from the suggested range."),
                               QMessageBox::Ok, nullptr);
    errorBox->exec();

    return false;
  }
  
  m_Data = newData;

  updateMemoryText();

  return true;
}

void DlgCopy::updateMemoryText()
{
  m_spnWatcherCopyOutput->setText(QString::fromStdString(charToHexString(m_Data.data(), m_Data.size(),
                      (DlgCopy::ByteStringFormats)m_cmbViewerBytesSeparator->currentIndex())));
}

bool DlgCopy::isHexString(std::string str)
{
  if (str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
  {
    str = str.substr(2);
  }

  for (char c : str)
  {
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
    {
      continue;
    }
    return false;
  }
  return true;
}

bool DlgCopy::hexStringToU32(std::string str, u32& output)
{
  // if (str.empty() || str.length() % 2 == 1)
  if (str.empty())
    return false;

  if (!isHexString(str))
    return false;

  std::stringstream ss(str);

  ss >> std::hex;
  ss >> output;

  return true;
}

bool DlgCopy::isUnsignedIntegerString(std::string str)
{
  for (char c: str)
  {
    if (c >= '0' && c <= '9')
    {
      continue;
    }
    return false;
  }
  return true;
}

bool DlgCopy::uintStringToU32(std::string str, u32& output)
{
  if (!isUnsignedIntegerString(str) || str.empty() || str.length() > 10)
    return false;

  u64 u = std::stoll(str);

  if (u > ULONG_MAX)
    return false;

  output = (u32)u;
  return true;
}

std::string DlgCopy::charToHexString(char* input, size_t count, DlgCopy::ByteStringFormats format)
{
  std::stringstream ss;
  const char convert[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string beforeAll = "";
  std::string beforeByte = "";
  std::string betweenBytes = "";
  std::string afterAll = "";
  
  switch (format)
  {
  case ByteString:
    beforeAll = "";
    beforeByte = "";
    betweenBytes = " ";
    afterAll = "";
    break;
  case ByteStringNoSpaces:
    beforeAll = "";
    beforeByte = "";
    betweenBytes = "";
    afterAll = "";
    break;
  case PythonByteString:
    beforeAll = "b\'";
    beforeByte = "\\x";
    betweenBytes = "";
    afterAll = "\'";
    break;
  case PythonList:
    beforeAll = "[ ";
    beforeByte = "0x";
    betweenBytes = ", ";
    afterAll = " ]";
    break;
  case CArray:
    beforeAll = "{ ";
    beforeByte = "0x";
    betweenBytes = ", ";
    afterAll = " }";
    break;
  default:
    return "";
  }
  
  ss << beforeAll;

  for (int i = 0; i < count; i++)
  {
    ss << beforeByte << convert[(input[i] >> 4) & 0xf] << convert[input[i] & 0xf];
    
    if (i != count - 1)
    {
      ss << betweenBytes;
    }
  }

  ss << afterAll;

  return ss.str();
}
