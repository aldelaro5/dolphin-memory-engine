#include "MemScanWidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QRadioButton>
#include <QRegExp>
#include <QVBoxLayout>

#include "../GUICommon.h"

MemScanWidget::MemScanWidget(QWidget* parent) : QWidget(parent)
{
  m_memScanner = new MemScanner();

  m_resultsListModel = new ResultsListModel(this, m_memScanner);

  m_lblResultCount = new QLabel("");
  m_tblResulstList = new QTableView();
  m_tblResulstList->setModel(m_resultsListModel);

  m_tblResulstList->horizontalHeader()->setStretchLastSection(true);
  m_tblResulstList->horizontalHeader()->resizeSection(0, 100);
  m_tblResulstList->horizontalHeader()->setSectionResizeMode(ResultsListModel::RESULT_COL_ADDRESS,
                                                             QHeaderView::Fixed);
  m_tblResulstList->horizontalHeader()->resizeSection(ResultsListModel::RESULT_COL_SCANNED, 150);

  m_tblResulstList->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tblResulstList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(m_tblResulstList,
          static_cast<void (QAbstractItemView::*)(const QModelIndex&)>(
              &QAbstractItemView::doubleClicked),
          this,
          static_cast<void (MemScanWidget::*)(const QModelIndex&)>(
              &MemScanWidget::onResultListDoubleClicked));

  m_btnFirstScan = new QPushButton("First scan");
  m_btnNextScan = new QPushButton("Next scan");
  m_btnNextScan->hide();
  m_btnResetScan = new QPushButton("Reset scan");
  m_btnResetScan->hide();

  connect(m_btnFirstScan, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemScanWidget::onFirstScan);
  connect(m_btnNextScan, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemScanWidget::onNextScan);
  connect(m_btnResetScan, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemScanWidget::onResetScan);

  QHBoxLayout* buttons_layout = new QHBoxLayout();
  buttons_layout->addWidget(m_btnFirstScan);
  buttons_layout->addWidget(m_btnNextScan);
  buttons_layout->addWidget(m_btnResetScan);

  m_txbSearchTerm1 = new QLineEdit();
  QLabel* lblAnd = new QLabel("and");
  m_txbSearchTerm2 = new QLineEdit();

  QHBoxLayout* searchTerm2_layout = new QHBoxLayout();
  searchTerm2_layout->setContentsMargins(0, 0, 0, 0);
  searchTerm2_layout->addWidget(lblAnd);
  searchTerm2_layout->addWidget(m_txbSearchTerm2);

  m_searchTerm2Widget = new QWidget();
  m_searchTerm2Widget->setLayout(searchTerm2_layout);

  QHBoxLayout* searchTerms_layout = new QHBoxLayout();
  searchTerms_layout->addWidget(m_txbSearchTerm1);
  searchTerms_layout->addWidget(m_searchTerm2Widget);
  searchTerms_layout->setSizeConstraint(QLayout::SetMinimumSize);
  m_searchTerm2Widget->hide();

  m_cmbScanType = new QComboBox();
  m_cmbScanType->addItems(GUICommon::g_memTypeNames);
  m_cmbScanType->setCurrentIndex(0);
  connect(m_cmbScanType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &MemScanWidget::onScanMemTypeChanged);
  m_variableLengthType = false;

  m_cmbScanFilter = new QComboBox();
  updateScanFilterChoices();
  connect(m_cmbScanFilter, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &MemScanWidget::onScanFilterChanged);

  QRadioButton* rdbBaseDecimal = new QRadioButton("Decimal");
  QRadioButton* rdbBaseHexadecimal = new QRadioButton("Hexadecimal");
  QRadioButton* rdbBaseOctal = new QRadioButton("Octal");
  QRadioButton* rdbBaseBinary = new QRadioButton("Binary");

  m_btnGroupScanBase = new QButtonGroup();
  m_btnGroupScanBase->addButton(rdbBaseDecimal, 0);
  m_btnGroupScanBase->addButton(rdbBaseHexadecimal, 1);
  m_btnGroupScanBase->addButton(rdbBaseOctal, 2);
  m_btnGroupScanBase->addButton(rdbBaseBinary, 3);
  rdbBaseDecimal->setChecked(true);

  QHBoxLayout* layout_buttonsBase = new QHBoxLayout();
  layout_buttonsBase->addWidget(rdbBaseDecimal);
  layout_buttonsBase->addWidget(rdbBaseHexadecimal);
  layout_buttonsBase->addWidget(rdbBaseOctal);
  layout_buttonsBase->addWidget(rdbBaseBinary);

  m_groupScanBase = new QGroupBox("Base to use");
  m_groupScanBase->setLayout(layout_buttonsBase);

  m_chkSignedScan = new QCheckBox("Signed value scan");
  m_chkSignedScan->setChecked(false);

  QVBoxLayout* scannerParams_layout = new QVBoxLayout();
  scannerParams_layout->addLayout(buttons_layout);
  scannerParams_layout->addWidget(m_cmbScanType);
  scannerParams_layout->addWidget(m_cmbScanFilter);
  scannerParams_layout->addLayout(searchTerms_layout);
  scannerParams_layout->addWidget(m_groupScanBase);
  scannerParams_layout->addWidget(m_chkSignedScan);
  scannerParams_layout->addStretch();
  scannerParams_layout->setContentsMargins(0, 0, 0, 0);

  QWidget* scannerParamsWidget = new QWidget();
  scannerParamsWidget->setLayout(scannerParams_layout);
  scannerParamsWidget->setMinimumWidth(400);

  QHBoxLayout* scanner_layout = new QHBoxLayout();
  scanner_layout->addWidget(m_tblResulstList);
  scanner_layout->addWidget(scannerParamsWidget);

  QVBoxLayout* main_layout = new QVBoxLayout();
  main_layout->addWidget(m_lblResultCount);
  main_layout->addLayout(scanner_layout);
  main_layout->setContentsMargins(3, 0, 3, 0);

  setLayout(main_layout);

  m_currentValuesUpdateTimer = new QTimer(this);
  connect(m_currentValuesUpdateTimer, &QTimer::timeout, this,
          &MemScanWidget::onCurrentValuesUpdateTimer);
}

MemScanWidget::~MemScanWidget()
{
  delete m_memScanner;
}

MemScanner::ScanFiter MemScanWidget::getSelectedFilter() const
{
  int index =
      GUICommon::g_memScanFilter.indexOf(QRegExp("^" + m_cmbScanFilter->currentText() + "$"));
  return static_cast<MemScanner::ScanFiter>(index);
}

void MemScanWidget::updateScanFilterChoices()
{
  Common::MemType newType = static_cast<Common::MemType>(m_cmbScanType->currentIndex());
  m_cmbScanFilter->clear();
  if (newType == Common::MemType::type_byteArray || newType == Common::MemType::type_string)
  {
    m_cmbScanFilter->addItem(
        GUICommon::g_memScanFilter.at(static_cast<int>(MemScanner::ScanFiter::exact)));
  }
  else if (m_memScanner->hasScanStarted())
  {
    m_cmbScanFilter->addItems(GUICommon::g_memScanFilter);
    m_cmbScanFilter->removeItem(static_cast<int>(MemScanner::ScanFiter::unknownInitial));
  }
  else
  {
    m_cmbScanFilter->addItem(
        GUICommon::g_memScanFilter.at(static_cast<int>(MemScanner::ScanFiter::exact)));
    m_cmbScanFilter->addItem(
        GUICommon::g_memScanFilter.at(static_cast<int>(MemScanner::ScanFiter::between)));
    m_cmbScanFilter->addItem(
        GUICommon::g_memScanFilter.at(static_cast<int>(MemScanner::ScanFiter::biggerThan)));
    m_cmbScanFilter->addItem(
        GUICommon::g_memScanFilter.at(static_cast<int>(MemScanner::ScanFiter::smallerThan)));
    m_cmbScanFilter->addItem(
        GUICommon::g_memScanFilter.at(static_cast<int>(MemScanner::ScanFiter::unknownInitial)));
  }
  m_cmbScanFilter->setCurrentIndex(0);
}

void MemScanWidget::updateTypeAdditionalOptions()
{
  if (m_memScanner->typeSupportsAdditionalOptions(
          static_cast<Common::MemType>(m_cmbScanType->currentIndex())))
  {
    m_chkSignedScan->show();
    m_groupScanBase->show();
  }
  else
  {
    m_chkSignedScan->hide();
    m_groupScanBase->hide();
  }
}

void MemScanWidget::onScanFilterChanged()
{
  MemScanner::ScanFiter theFilter = getSelectedFilter();
  int numTerms = m_memScanner->getTermsNumForFilter(theFilter);
  switch (numTerms)
  {
  case 0:
    m_txbSearchTerm1->hide();
    m_searchTerm2Widget->hide();
    m_chkSignedScan->hide();
    m_groupScanBase->hide();
    break;
  case 1:
    m_txbSearchTerm1->show();
    m_searchTerm2Widget->hide();
    updateTypeAdditionalOptions();
    break;
  case 2:
    m_txbSearchTerm1->show();
    m_searchTerm2Widget->show();
    updateTypeAdditionalOptions();
    break;
  }
}

void MemScanWidget::onScanMemTypeChanged()
{
  Common::MemType newType = static_cast<Common::MemType>(m_cmbScanType->currentIndex());
  if (!m_variableLengthType &&
      (newType == Common::MemType::type_string || newType == Common::MemType::type_byteArray))
  {
    updateScanFilterChoices();
    m_variableLengthType = true;
  }
  else if (m_variableLengthType && newType != Common::MemType::type_string &&
           newType != Common::MemType::type_byteArray)
  {
    updateScanFilterChoices();
    m_variableLengthType = false;
  }

  updateTypeAdditionalOptions();
}

void MemScanWidget::onFirstScan()
{
  m_memScanner->setType(static_cast<Common::MemType>(m_cmbScanType->currentIndex()));
  m_memScanner->setIsSigned(m_chkSignedScan->isChecked());
  m_memScanner->setBase(static_cast<Common::MemBase>(m_btnGroupScanBase->checkedId()));
  Common::MemOperationReturnCode scannerReturn =
      m_memScanner->firstScan(getSelectedFilter(), m_txbSearchTerm1->text().toStdString(),
                              m_txbSearchTerm2->text().toStdString());
  if (scannerReturn != Common::MemOperationReturnCode::OK)
  {
    handleScannerErrors(scannerReturn);
  }
  else
  {
    m_lblResultCount->setText(
        QString::number(m_memScanner->getResultCount()).append(" result(s) found"));
    m_btnFirstScan->hide();
    m_btnNextScan->show();
    m_btnResetScan->show();
    m_cmbScanType->setDisabled(true);
    m_chkSignedScan->setDisabled(true);
    m_groupScanBase->setDisabled(true);
    updateScanFilterChoices();
  }
}

void MemScanWidget::onNextScan()
{
  Common::MemOperationReturnCode scannerReturn =
      m_memScanner->nextScan(getSelectedFilter(), m_txbSearchTerm1->text().toStdString(),
                             m_txbSearchTerm2->text().toStdString());
  if (scannerReturn != Common::MemOperationReturnCode::OK)
  {
    handleScannerErrors(scannerReturn);
  }
  else
  {
    m_lblResultCount->setText(
        QString::number(m_memScanner->getResultCount()).append(" result(s) found"));
  }
}

void MemScanWidget::onResetScan()
{
  m_memScanner->reset();
  m_lblResultCount->setText("");
  m_btnFirstScan->show();
  m_btnNextScan->hide();
  m_btnResetScan->hide();
  m_cmbScanType->setEnabled(true);
  m_chkSignedScan->setEnabled(true);
  m_groupScanBase->setEnabled(true);
  m_resultsListModel->updateAfterScannerReset();
  updateScanFilterChoices();
}

void MemScanWidget::handleScannerErrors(const Common::MemOperationReturnCode errorCode)
{
  if (errorCode == Common::MemOperationReturnCode::invalidInput)
  {
    QMessageBox* errorBox =
        new QMessageBox(QMessageBox::Critical, "Invalid term(s)",
                        QString("The search term(s) you entered for the type " +
                                m_cmbScanType->currentText() + " is/are invalid"),
                        QMessageBox::Ok, this);
    errorBox->exec();
  }
  else if (errorCode == Common::MemOperationReturnCode::operationFailed)
  {
    emit mustUnhook();
  }
}

void MemScanWidget::onCurrentValuesUpdateTimer()
{
  if (m_memScanner->getResultCount() > 0 && m_memScanner->getResultCount() <= 1000)
  {
    Common::MemOperationReturnCode updateReturn = m_resultsListModel->updateScannerCurrentCache();
    if (updateReturn != Common::MemOperationReturnCode::OK)
    {
      handleScannerErrors(updateReturn);
    }
  }
}

QTimer* MemScanWidget::getUpdateTimer() const
{
  return m_currentValuesUpdateTimer;
}

void MemScanWidget::onResultListDoubleClicked(const QModelIndex& index)
{
  if (index != QVariant())
  {
    emit requestAddWatchEntry(m_resultsListModel->getResultAddress(index.row()),
                              m_memScanner->getType(), m_memScanner->getLength(),
                              m_memScanner->getIsUnsigned(), m_memScanner->getBase());
  }
}