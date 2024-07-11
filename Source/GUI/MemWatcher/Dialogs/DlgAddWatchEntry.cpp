#include "DlgAddWatchEntry.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>
#include <sstream>

#include "../../../DolphinProcess/DolphinAccessor.h"
#include "../../GUICommon.h"

DlgAddWatchEntry::DlgAddWatchEntry(const bool newEntry, MemWatchEntry* const entry,
                                   QWidget* const parent)
    : QDialog(parent)
{
  setWindowTitle(newEntry ? "Add Watch" : "Edit Watch");
  initialiseWidgets();
  makeLayouts();
  fillFields(entry);

  // These are connected at the end to avoid triggering the events when initialising.
  connect(m_cmbTypes, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &DlgAddWatchEntry::onTypeChange);
  connect(m_spnLength, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &DlgAddWatchEntry::onLengthChanged);
}

DlgAddWatchEntry::~DlgAddWatchEntry()
{
  delete m_entry;
}

void DlgAddWatchEntry::initialiseWidgets()
{
  m_chkBoundToPointer = new QCheckBox("This is a pointer", this);
  connect(m_chkBoundToPointer, &QCheckBox::stateChanged, this,
          &DlgAddWatchEntry::onIsPointerChanged);

  m_lblValuePreview = new QLineEdit("", this);
  m_lblValuePreview->setReadOnly(true);

  m_txbAddress = new AddressInputWidget(this);
  connect(m_txbAddress, &QLineEdit::textEdited, this, &DlgAddWatchEntry::onAddressChanged);

  const QFont fixedFont{QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont)};
  m_lblValuePreview->setFont(fixedFont);

  m_offsetsLayout = new QGridLayout;

  m_offsets = QVector<QLineEdit*>();

  m_btnAddOffset = new QPushButton("Add offset");
  connect(m_btnAddOffset, &QPushButton::clicked, this, &DlgAddWatchEntry::addPointerOffset);

  m_btnRemoveOffset = new QPushButton("Remove offset");
  connect(m_btnRemoveOffset, &QPushButton::clicked, this, &DlgAddWatchEntry::removePointerOffset);

  m_pointerWidget = new QGroupBox("Offsets (in hex)", this);

  m_txbLabel = new QLineEdit(this);

  m_cmbTypes = new QComboBox(this);
  m_cmbTypes->addItems(GUICommon::g_memTypeNames);

  m_spnLength = new QSpinBox(this);
  m_spnLength->setPrefix("Length: ");
  m_spnLength->setMinimum(1);
  m_spnLength->setMaximum(9999);
}

void DlgAddWatchEntry::makeLayouts()
{
  QFormLayout* formLayout = new QFormLayout;
  formLayout->setLabelAlignment(Qt::AlignRight);
  formLayout->addRow("Preview:", m_lblValuePreview);
  formLayout->addRow("Address:", m_txbAddress);
  formLayout->addRow("Label:", m_txbLabel);

  QHBoxLayout* layout_type = new QHBoxLayout;
  layout_type->setContentsMargins(0, 0, 0, 0);
  layout_type->addWidget(m_cmbTypes, 1);
  layout_type->addWidget(m_spnLength);
  QWidget* widget_type = new QWidget;
  widget_type->setLayout(layout_type);
  widget_type->setContentsMargins(0, 0, 0, 0);
  formLayout->addRow("Type:", widget_type);

  QWidget* offsetsWidget = new QWidget();
  offsetsWidget->setLayout(m_offsetsLayout);
  offsetsWidget->setContentsMargins(0, 0, 0, 0);
  m_offsetsLayout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout* pointerButtons_layout = new QHBoxLayout;
  pointerButtons_layout->setContentsMargins(0, 0, 0, 0);
  pointerButtons_layout->addWidget(m_btnAddOffset);
  pointerButtons_layout->addWidget(m_btnRemoveOffset);
  QWidget* pointerButtons_widget = new QWidget();
  pointerButtons_widget->setContentsMargins(0, 0, 0, 0);
  pointerButtons_widget->setLayout(pointerButtons_layout);

  QVBoxLayout* pointerOffset_layout = new QVBoxLayout;
  pointerOffset_layout->addWidget(pointerButtons_widget);
  pointerOffset_layout->addWidget(offsetsWidget);
  pointerOffset_layout->addStretch();
  m_pointerWidget->setLayout(pointerOffset_layout);

  QDialogButtonBox* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  QVBoxLayout* main_layout = new QVBoxLayout;
  main_layout->addLayout(formLayout);
  main_layout->addWidget(m_chkBoundToPointer);
  main_layout->addWidget(m_pointerWidget);
  main_layout->addStretch();
  main_layout->addWidget(buttonBox);
  setLayout(main_layout);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &DlgAddWatchEntry::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &DlgAddWatchEntry::reject);
}

void DlgAddWatchEntry::fillFields(MemWatchEntry* entry)
{
  if (entry == nullptr)
  {
    m_entry = new MemWatchEntry();

    m_cmbTypes->setCurrentIndex(0);
    m_spnLength->setValue(1);
    m_spnLength->hide();
    m_lblValuePreview->setText("???");
    m_chkBoundToPointer->setChecked(false);
    m_pointerWidget->hide();
  }
  else
  {
    m_entry = entry;

    m_spnLength->setValue(static_cast<int>(m_entry->getLength()));
    m_cmbTypes->setCurrentIndex(static_cast<int>(m_entry->getType()));
    if (m_entry->getType() == Common::MemType::type_string ||
        m_entry->getType() == Common::MemType::type_byteArray)
      m_spnLength->show();
    else
      m_spnLength->hide();
    m_txbLabel->setText(m_entry->getLabel());
    std::stringstream ssAddress;
    ssAddress << std::hex << std::uppercase << m_entry->getConsoleAddress();
    m_txbAddress->setText(QString::fromStdString(ssAddress.str()));
    if (m_entry->isBoundToPointer())
    {
      m_pointerWidget->show();
      for (int i{0}; i < static_cast<int>(m_entry->getPointerLevel()); ++i)
      {
        std::stringstream ss;
        ss << std::hex << std::uppercase << m_entry->getPointerOffset(i);
        QLabel* lblLevel =
            new QLabel(QString::fromStdString("Level " + std::to_string(i + 1) + ":"));
        QLineEdit* txbOffset = new QLineEdit();
        txbOffset->setFixedWidth(100);
        txbOffset->setMaxLength(7);
        txbOffset->setText(QString::fromStdString(ss.str()));
        connect(txbOffset, &QLineEdit::textEdited, this, &DlgAddWatchEntry::onOffsetChanged);
        m_offsets.append(txbOffset);
        QLabel* lblAddressOfPath = new QLabel();
        lblAddressOfPath->setText(
            QString::fromStdString(" -> " + m_entry->getAddressStringForPointerLevel(i + 1)));
        lblAddressOfPath->setProperty("addr", m_entry->getAddressForPointerLevel(i + 1));
        lblAddressOfPath->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(lblAddressOfPath, &QWidget::customContextMenuRequested, this,
                &DlgAddWatchEntry::onPointerOffsetContextMenuRequested);
        m_addressPath.append(lblAddressOfPath);
        m_offsetsLayout->addWidget(lblLevel, i, 0);
        m_offsetsLayout->addWidget(txbOffset, i, 1);
        m_offsetsLayout->addWidget(lblAddressOfPath, i, 2);
      }
    }
    else
    {
      m_pointerWidget->hide();
    }

    m_chkBoundToPointer->setChecked(m_entry->isBoundToPointer());
    updatePreview();
  }
}

void DlgAddWatchEntry::addPointerOffset()
{
  int level = static_cast<int>(m_entry->getPointerLevel());
  QLabel* lblLevel = new QLabel(QString::fromStdString("Level " + std::to_string(level + 1) + ":"));
  QLineEdit* txbOffset = new QLineEdit();
  m_offsets.append(txbOffset);
  QLabel* lblAddressOfPath = new QLabel(" -> ");
  lblAddressOfPath->setText(QString::fromStdString(" -> " + m_entry->getAddressStringForPointerLevel(level + 1)));
  lblAddressOfPath->setProperty("addr", m_entry->getAddressForPointerLevel(level + 1));
  lblAddressOfPath->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(lblAddressOfPath, &QWidget::customContextMenuRequested, this,
          &DlgAddWatchEntry::onPointerOffsetContextMenuRequested);
  m_addressPath.append(lblAddressOfPath);
  m_offsetsLayout->addWidget(lblLevel, level, 0);
  m_offsetsLayout->addWidget(txbOffset, level, 1);
  m_offsetsLayout->addWidget(lblAddressOfPath, level, 2);
  m_entry->addOffset(0);
  connect(txbOffset, &QLineEdit::textEdited, this, &DlgAddWatchEntry::onOffsetChanged);
  if (m_entry->getPointerLevel() > 1)
    m_btnRemoveOffset->setEnabled(true);
  else
    m_btnRemoveOffset->setDisabled(true);
}

void DlgAddWatchEntry::removePointerOffset()
{
  int level = static_cast<int>(m_entry->getPointerLevel());

  QLayoutItem* lblLevelItem = m_offsetsLayout->takeAt(
      m_offsetsLayout->indexOf(m_offsetsLayout->itemAtPosition(level - 1, 0)->widget()));
  QLayoutItem* txbOffsetItem = m_offsetsLayout->takeAt(
      m_offsetsLayout->indexOf(m_offsetsLayout->itemAtPosition(level - 1, 1)->widget()));
  QLayoutItem* lblAddressOfPathItem = m_offsetsLayout->takeAt(
      m_offsetsLayout->indexOf(m_offsetsLayout->itemAtPosition(level - 1, 2)->widget()));

  delete lblLevelItem->widget();
  delete txbOffsetItem->widget();
  delete lblAddressOfPathItem->widget();

  m_offsetsLayout->setRowMinimumHeight(level - 1, 0);
  m_offsetsLayout->setRowStretch(level - 1, 0);

  m_offsets.removeLast();
  m_addressPath.removeLast();
  m_entry->removeOffset();
  updatePreview();
  if (m_entry->getPointerLevel() == 1)
  {
    m_btnRemoveOffset->setDisabled(true);
    m_btnAddOffset->setFocus();
  }
}

void DlgAddWatchEntry::removeAllPointerOffset()
{
  int level = static_cast<int>(m_entry->getPointerLevel());
  while (level != 0)
  {
    QLayoutItem* lblLevelItem = m_offsetsLayout->takeAt(
        m_offsetsLayout->indexOf(m_offsetsLayout->itemAtPosition(level - 1, 0)->widget()));
    QLayoutItem* txbOffsetItem = m_offsetsLayout->takeAt(
        m_offsetsLayout->indexOf(m_offsetsLayout->itemAtPosition(level - 1, 1)->widget()));
    QLayoutItem* lblAddressOfPathItem = m_offsetsLayout->takeAt(
        m_offsetsLayout->indexOf(m_offsetsLayout->itemAtPosition(level - 1, 2)->widget()));

    delete lblLevelItem->widget();
    delete txbOffsetItem->widget();
    delete lblAddressOfPathItem->widget();

    m_offsets.removeLast();
    m_addressPath.removeLast();
    m_entry->removeOffset();

    level--;
  }

  updatePreview();
}

void DlgAddWatchEntry::onOffsetChanged()
{
  QLineEdit* theLineEdit = static_cast<QLineEdit*>(sender());
  int index = 0;
  // Dummy variable for getItemPosition
  int column{};
  int rowSpan{};
  int columnSpan{};
  m_offsetsLayout->getItemPosition(m_offsetsLayout->indexOf(theLineEdit), &index, &column, &rowSpan,
                                   &columnSpan);
  if (validateAndSetOffset(index))
    updatePreview();
}

void DlgAddWatchEntry::onTypeChange(int index)
{
  Common::MemType theType = static_cast<Common::MemType>(index);
  if (theType == Common::MemType::type_string || theType == Common::MemType::type_byteArray)
    m_spnLength->show();
  else
    m_spnLength->hide();
  m_entry->setTypeAndLength(theType, m_spnLength->value());
  if (validateAndSetAddress())
    updatePreview();
}

void DlgAddWatchEntry::accept()
{
  if (!validateAndSetAddress())
  {
    QString errorMsg = tr("The address you entered is invalid, make sure it is an "
                          "hexadecimal number between 0x%1 and 0x%2")
                           .arg(Common::MEM1_START, 8, 16)
                           .arg(Common::GetMEM1End() - 1, 8, 16);
    if (DolphinComm::DolphinAccessor::isMEM2Present())
      errorMsg.append(tr(" or between 0x%1 and 0x%2")
                          .arg(Common::MEM2_START, 8, 16)
                          .arg(Common::GetMEM2End() - 1, 8, 16));
    QMessageBox* errorBox = new QMessageBox(QMessageBox::Critical, tr("Invalid address"), errorMsg,
                                            QMessageBox::Ok, this);
    errorBox->exec();
  }
  else
  {
    if (m_chkBoundToPointer->isChecked())
    {
      bool allOffsetsValid = true;
      int i = 0;
      for (; i < m_offsets.count(); ++i)
      {
        allOffsetsValid = validateAndSetOffset(i);
        if (!allOffsetsValid)
          break;
      }
      if (!allOffsetsValid)
      {
        QMessageBox* errorBox = new QMessageBox(
            QMessageBox::Critical, "Invalid offset",
            QString::fromStdString("The offset you entered for the level " + std::to_string(i + 1) +
                                   " is invalid, make sure it is an "
                                   "hexadecimal number"),
            QMessageBox::Ok, this);
        errorBox->exec();
        return;
      }
    }
    if (m_txbLabel->text() == "")
      m_entry->setLabel("No label");
    else
      m_entry->setLabel(m_txbLabel->text());
    m_entry->setBase(Common::MemBase::base_decimal);
    m_entry->setSignedUnsigned(false);
    setResult(QDialog::Accepted);
    hide();
  }
}

bool DlgAddWatchEntry::validateAndSetAddress()
{
  std::string addressStr = m_txbAddress->text().toStdString();
  std::stringstream ss(addressStr);
  ss >> std::hex;
  u32 address = 0;
  ss >> address;
  if (!ss.fail())
  {
    if (DolphinComm::DolphinAccessor::isValidConsoleAddress(address))
    {
      m_entry->setConsoleAddress(address);
      return true;
    }
  }
  return false;
}

bool DlgAddWatchEntry::validateAndSetOffset(int index)
{
  std::string offsetStr = m_offsets[index]->text().toStdString();
  std::stringstream ss(offsetStr);
  ss >> std::hex;
  int offset = 0;
  ss >> offset;
  if (!ss.fail())
  {
    m_entry->setPointerOffset(offset, index);
    return true;
  }
  return false;
}

void DlgAddWatchEntry::onAddressChanged()
{
  if (validateAndSetAddress())
    updatePreview();
  else
    m_lblValuePreview->setText("???");
}

void DlgAddWatchEntry::updatePreview()
{
  m_entry->readMemoryFromRAM();
  m_lblValuePreview->setText(QString::fromStdString(m_entry->getStringFromMemory()));
  if (m_entry->isBoundToPointer())
  {
    const int level{static_cast<int>(m_entry->getPointerLevel())};
    for (int i = 0; i < level; ++i)
    {
      QLabel* lblAddressOfPath =
          static_cast<QLabel*>(m_offsetsLayout->itemAtPosition(i, 2)->widget());
      lblAddressOfPath->setText(
          QString::fromStdString(" -> " + m_entry->getAddressStringForPointerLevel(i + 1)));
      lblAddressOfPath->setProperty("addr", m_entry->getAddressForPointerLevel(i + 1));
    }
  }
}

void DlgAddWatchEntry::onLengthChanged()
{
  Common::MemType theType = static_cast<Common::MemType>(m_cmbTypes->currentIndex());
  m_entry->setTypeAndLength(theType, m_spnLength->value());
  if (validateAndSetAddress())
    updatePreview();
}

void DlgAddWatchEntry::onIsPointerChanged()
{
  if (m_chkBoundToPointer->isChecked())
  {
    m_pointerWidget->show();
    if (m_entry->getPointerLevel() == 0)
      addPointerOffset();
  }
  else
  {
    m_pointerWidget->hide();
    removeAllPointerOffset();
  }
  adjustSize();
  m_entry->setBoundToPointer(m_chkBoundToPointer->isChecked());
  updatePreview();
}

MemWatchEntry* DlgAddWatchEntry::stealEntry()
{
  MemWatchEntry* entry{m_entry};
  m_entry = nullptr;
  return entry;
}

void DlgAddWatchEntry::onPointerOffsetContextMenuRequested(const QPoint& pos)
{
  QLabel* const lbl = static_cast<QLabel*>(sender());

  QMenu* contextMenu = new QMenu(this);
  QAction* copyAddr = new QAction(tr("&Copy Address"), this);

  const QString text{QString::number(lbl->property("addr").toUInt(), 16).toUpper()};
  connect(copyAddr, &QAction::triggered, this,
    [text] { QApplication::clipboard()->setText(text); });
  contextMenu->addAction(copyAddr);

  if (!lbl->property("addr").toUInt())
    {
        copyAddr->setEnabled(false);
      }

  contextMenu->popup(lbl->mapToGlobal(pos));
}
