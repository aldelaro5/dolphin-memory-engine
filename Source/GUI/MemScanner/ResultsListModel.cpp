#include "ResultsListModel.h"

ResultsListModel::ResultsListModel(QObject* parent, MemScanner* scanner)
    : QAbstractTableModel(parent), m_scanner(scanner)
{
}

ResultsListModel::~ResultsListModel()
{
}

int ResultsListModel::columnCount(const QModelIndex& parent) const
{
  return RESULT_COL_NUM;
}

int ResultsListModel::rowCount(const QModelIndex& parent) const
{
  if (m_scanner->getResultCount() > 1000)
    return 0;
  return static_cast<int>(m_scanner->getResultCount());
}

QVariant ResultsListModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
    case RESULT_COL_ADDRESS:
    {
      return QString::number(m_scanner->getResultsConsoleAddr().at(index.row()), 16).toUpper();
      break;
    }
    case RESULT_COL_SCANNED:
    {
      return QString::fromStdString(m_scanner->getFormattedScannedValueAt(index.row()));
      break;
    }
    case RESULT_COL_CURRENT:
    {
      return QString::fromStdString(m_scanner->getFormattedCurrentValueAt(index.row()));
      break;
    }
    }
  }
  return QVariant();
}

u32 ResultsListModel::getResultAddress(const int row) const
{
  return m_scanner->getResultsConsoleAddr().at(row);
}

QVariant ResultsListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case RESULT_COL_ADDRESS:
      return QString("Address");
      break;
    case RESULT_COL_SCANNED:
      return QString("Scanned");
      break;
    case RESULT_COL_CURRENT:
      return QString("Current");
      break;
    }
  }
  return QVariant();
}

Common::MemOperationReturnCode ResultsListModel::updateScannerCurrentCache()
{
  Common::MemOperationReturnCode updateReturn = m_scanner->updateCurrentRAMCache();
  if (updateReturn == Common::MemOperationReturnCode::OK)
    emit layoutChanged();
  return updateReturn;
}

void ResultsListModel::updateAfterScannerReset()
{
  emit layoutChanged();
}