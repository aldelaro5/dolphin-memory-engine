#include "DlgChangeType.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "../../GUICommon.h"

DlgChangeType::DlgChangeType(QWidget* parent, const int typeIndex, const size_t length)
    : QDialog(parent), m_typeIndex(typeIndex), m_length(length)
{
  initialiseWidgets();
  makeLayouts();
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
  QWidget* widget_type = new QWidget;
  widget_type->setLayout(layout_type);
  widget_type->setContentsMargins(0, 0, 0, 0);

  QFormLayout* formLayout = new QFormLayout;
  formLayout->setLabelAlignment(Qt::AlignRight);
  formLayout->addRow("New type:", widget_type);

  Common::MemType theType = static_cast<Common::MemType>(m_typeIndex);
  if (theType != Common::MemType::type_string && theType != Common::MemType::type_byteArray)
    m_spnLength->hide();

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

int DlgChangeType::getTypeIndex() const
{
  return m_typeIndex;
}

size_t DlgChangeType::getLength() const
{
  return m_length;
}

void DlgChangeType::accept()
{
  m_typeIndex = m_cmbTypes->currentIndex();
  m_length = m_spnLength->value();
  setResult(QDialog::Accepted);
  hide();
}

void DlgChangeType::onTypeChange(int index)
{
  Common::MemType theType = static_cast<Common::MemType>(index);
  if (theType == Common::MemType::type_string || theType == Common::MemType::type_byteArray)
    m_spnLength->show();
  else
    m_spnLength->hide();
  adjustSize();
}
