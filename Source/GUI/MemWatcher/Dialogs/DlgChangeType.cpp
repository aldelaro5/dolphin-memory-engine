#include "DlgChangeType.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "../../GUICommon.h"
#include "DlgAddWatchEntry.h"

DlgChangeType::DlgChangeType(QWidget* parent, const int typeIndex, const size_t length,
                             QVector<QString> structNames, QString curStructName,
                             size_t containerCount, MemWatchEntry* containerEntry)
    : QDialog(parent), m_typeIndex(typeIndex), m_length(length), m_containerCount(containerCount),
      m_structNames(structNames)
{
  m_structNames.push_front(QString());
  setWindowTitle("Change Type");
  initialiseWidgets();
  makeLayouts();

  if (m_structNames.contains(curStructName))
    m_structSelect->setCurrentIndex(static_cast<int>(m_structNames.indexOf(curStructName)));

  if (containerEntry)
    m_containerEntry = new MemWatchEntry(containerEntry);
}

void DlgChangeType::initialiseWidgets()
{
  m_cmbTypes = new QComboBox(this);
  m_cmbTypes->addItems(GUICommon::g_memTypeNames);
  m_cmbTypes->setCurrentIndex(m_typeIndex);

  m_spnLength = new QSpinBox(this);
  m_spnLength->setPrefix("Length: ");
  m_spnLength->setMinimum(1);
  m_spnLength->setMaximum(9999);
  m_spnLength->setValue(static_cast<int>(m_length));

  m_structSelect = new QComboBox(this);
  m_structSelect->addItems(m_structNames);
  m_structSelect->setCurrentIndex(0);

  m_spnContainerCount = new QSpinBox(this);
  m_spnContainerCount->setPrefix("");
  m_spnContainerCount->setMinimum(1);
  m_spnContainerCount->setMaximum(9999);
  m_spnContainerCount->setValue(static_cast<int>(m_containerCount));

  m_btnSetupContainerEntry = new QPushButton("Setup Contents");
  connect(m_btnSetupContainerEntry, &QPushButton::clicked, this,
          &DlgChangeType::onSetupContainerContents);

  connect(m_cmbTypes, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &DlgChangeType::onTypeChange);
}

void DlgChangeType::makeLayouts()
{
  QHBoxLayout* layout_type = new QHBoxLayout;
  layout_type->setSpacing(5);
  layout_type->setContentsMargins(0, 0, 0, 0);
  layout_type->addWidget(m_cmbTypes, 1);
  layout_type->addWidget(m_spnLength);
  layout_type->addWidget(m_structSelect);
  layout_type->addWidget(m_spnContainerCount);

  QVBoxLayout* super_layout_type = new QVBoxLayout;
  super_layout_type->addLayout(layout_type);
  super_layout_type->addWidget(m_btnSetupContainerEntry);

  QWidget* widget_type = new QWidget;
  widget_type->setLayout(super_layout_type);
  widget_type->setContentsMargins(0, 0, 0, 0);

  QFormLayout* formLayout = new QFormLayout;
  formLayout->setLabelAlignment(Qt::AlignRight);
  formLayout->addRow("New type:", widget_type);

  Common::MemType theType = static_cast<Common::MemType>(m_typeIndex);
  if (theType != Common::MemType::type_string && theType != Common::MemType::type_byteArray)
    m_spnLength->hide();
  if (theType != Common::MemType::type_struct)
    m_structSelect->hide();
  if (theType != Common::MemType::type_array)
  {
    m_btnSetupContainerEntry->hide();
    m_spnContainerCount->hide();
  }

  QDialogButtonBox* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &DlgChangeType::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &DlgChangeType::reject);

  QVBoxLayout* main_layout = new QVBoxLayout;
  main_layout->addLayout(formLayout);
  main_layout->addStretch();
  main_layout->addWidget(buttonBox);
  setLayout(main_layout);
}

void DlgChangeType::onSetupContainerContents()
{
  MemWatchEntry* curEntry = m_containerEntry;
  bool isNewEntry = curEntry == nullptr;

  DlgAddWatchEntry dlg(isNewEntry, curEntry, m_structNames, this, true, m_curArrayDepth + 1);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_containerEntry = dlg.stealEntry();
  }
}

int DlgChangeType::getTypeIndex() const
{
  return m_typeIndex;
}

size_t DlgChangeType::getLength() const
{
  return m_length;
}

QString DlgChangeType::getStructName() const
{
  return m_structNames[m_structSelect->currentIndex()];
}

size_t DlgChangeType::getContainerCount() const
{
  return m_containerCount;
}

MemWatchEntry* DlgChangeType::getContainerEntry()
{
  MemWatchEntry* entry{m_containerEntry};
  m_containerEntry = nullptr;
  return entry;
}

void DlgChangeType::accept()
{
  m_typeIndex = m_cmbTypes->currentIndex();
  m_length = m_spnLength->value();
  m_containerCount = m_spnContainerCount->value();
  setResult(QDialog::Accepted);
  hide();
}

void DlgChangeType::onTypeChange(int index)
{
  Common::MemType theType = static_cast<Common::MemType>(index);
  if (theType == Common::MemType::type_string || theType == Common::MemType::type_byteArray)
  {
    m_structSelect->hide();
    m_spnLength->show();
    m_btnSetupContainerEntry->hide();
    m_spnContainerCount->hide();
  }
  else if (theType == Common::MemType::type_struct)
  {
    m_structSelect->show();
    m_spnLength->hide();
    m_btnSetupContainerEntry->hide();
    m_spnContainerCount->hide();
  }
  else if (theType == Common::MemType::type_array)
  {
    m_structSelect->hide();
    m_spnLength->hide();
    m_btnSetupContainerEntry->show();
    m_spnContainerCount->show();
  }
  else
  {
    m_structSelect->hide();
    m_spnLength->hide();
    m_btnSetupContainerEntry->hide();
    m_spnContainerCount->hide();
  }
  adjustSize();
}
