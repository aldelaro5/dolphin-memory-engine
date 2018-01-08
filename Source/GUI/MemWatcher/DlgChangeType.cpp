#include "DlgChangeType.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "../GUICommon.h"

DlgChangeType::DlgChangeType(QWidget* parent, const int typeIndex, const size_t length)
    : QDialog(parent), m_typeIndex(typeIndex), m_length(length)
{
  QLabel* lblType = new QLabel(QString("New type: "), this);
  m_cmbTypes = new QComboBox(this);
  m_cmbTypes->addItems(GUICommon::g_memTypeNames);
  m_cmbTypes->setCurrentIndex(typeIndex);

  QLabel* lblLength = new QLabel(QString("Length: "), this);
  m_spnLength = new QSpinBox(this);
  m_spnLength->setMinimum(1);
  m_spnLength->setValue(static_cast<int>(length));

  QWidget* typeSelection = new QWidget(this);
  QHBoxLayout* typeSelectionLayout = new QHBoxLayout;
  typeSelectionLayout->addWidget(lblType);
  typeSelectionLayout->addWidget(m_cmbTypes);
  typeSelection->setLayout(typeSelectionLayout);

  m_lengthSelection = new QWidget;
  QHBoxLayout* lengthSelectionLayout = new QHBoxLayout;
  lengthSelectionLayout->addWidget(lblLength);
  lengthSelectionLayout->addWidget(m_spnLength);
  m_lengthSelection->setLayout(lengthSelectionLayout);

  QDialogButtonBox* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  QVBoxLayout* main_layout = new QVBoxLayout;

  main_layout->addWidget(typeSelection);
  main_layout->addWidget(m_lengthSelection);
  main_layout->addWidget(buttonBox);
  main_layout->setSpacing(1);
  setLayout(main_layout);

  Common::MemType theType = static_cast<Common::MemType>(typeIndex);
  if (theType != Common::MemType::type_string && theType != Common::MemType::type_byteArray)
    m_lengthSelection->hide();

  connect(m_cmbTypes, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &DlgChangeType::onTypeChange);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &DlgChangeType::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &DlgChangeType::reject);
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
    m_lengthSelection->show();
  else
    m_lengthSelection->hide();
  adjustSize();
}
