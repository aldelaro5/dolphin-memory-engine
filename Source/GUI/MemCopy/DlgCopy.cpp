#include "DlgCopy.h"

#include <QAbstractButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <qmessagebox.h>

#include <algorithm>
#include <sstream>
#include <utility>
#include "../../Common/CommonUtils.h"
#include "../../DolphinProcess/DolphinAccessor.h"

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
  m_cmbViewerBytesSeparator->addItem("Byte String (No Spaces)",
                                     ByteStringFormats::ByteStringNoSpaces);
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
  connect(m_buttonsDlg, &QDialogButtonBox::clicked, this, [this](QAbstractButton* const button) {
    const auto role = m_buttonsDlg->buttonRole(button);
    if (role == QDialogButtonBox::ApplyRole)
    {
      copyMemory();
    }
    else if (role == QDialogButtonBox::RejectRole)
    {
      hide();
    }
  });

  connect(m_cmbViewerBytesSeparator, &QComboBox::currentTextChanged, this,
          [this](const QString&) { updateMemoryText(); });

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(grbCopySettings);
  mainLayout->addWidget(m_buttonsDlg);

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

bool DlgCopy::copyMemory()
{
  u32 address, count;
  QMessageBox* errorBox;

  if (!hexStringToU32(m_spnWatcherCopyAddress->text().toStdString(), address))
  {
    QString errorMsg = tr("The address you entered is invalid, make sure it is an "
                          "hexadecimal number between 0x%08X and 0x%08X")
                           .arg(Common::MEM1_START, Common::GetMEM1End() - 1);
    if (DolphinComm::DolphinAccessor::isMEM2Present())
      errorMsg.append(
          tr(" or between 0x%08X and 0x%08X").arg(Common::MEM2_START, Common::GetMEM2End() - 1));

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
  m_spnWatcherCopyOutput->setText(QString::fromStdString(charToHexString(
      m_Data.data(), m_Data.size(),
      static_cast<DlgCopy::ByteStringFormats>(m_cmbViewerBytesSeparator->currentIndex()))));
}

bool DlgCopy::isHexString(std::string_view str)
{
  if (str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
  {
    str = str.substr(2);
  }

  return std::ranges::all_of(str.cbegin(), str.cend(), [](const char c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
  });
}

bool DlgCopy::hexStringToU32(const std::string_view str, u32& output)
{
  // if (str.empty() || str.length() % 2 == 1)
  if (str.empty())
    return false;

  if (!isHexString(str))
    return false;

  std::stringstream ss;
  ss << str;

  ss >> std::hex;
  ss >> output;

  return true;
}

bool DlgCopy::isUnsignedIntegerString(const std::string_view str)
{
  return std::ranges::all_of(str.cbegin(), str.cend(),
                             [](const char c) { return '0' <= c && c <= '9'; });
}

bool DlgCopy::uintStringToU32(const std::string_view str, u32& output)
{
  if (!isUnsignedIntegerString(str) || str.empty() || str.length() > 10)
    return false;

  const auto u{static_cast<u64>(std::stoll(std::string{str}))};

  if (u > ULONG_MAX)
    return false;

  output = static_cast<u32>(u);
  return true;
}

std::string DlgCopy::charToHexString(const char* const input, const size_t count,
                                     const DlgCopy::ByteStringFormats format)
{
  std::stringstream ss;
  const char convert[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string beforeAll;
  std::string beforeByte;
  std::string betweenBytes;
  std::string afterAll;

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

  for (size_t i{0}; i < count; ++i)
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
