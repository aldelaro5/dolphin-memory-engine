#include "ResultsListModel.h"

ResultsListModel::ResultsListModel(QObject* parent, MemScanner* scanner)
    : QAbstractTableModel(parent), m_scanner(scanner)
{
}

ResultsListModel::~ResultsListModel() = default;

int ResultsListModel::columnCount(const QModelIndex& parent) const
{
  (void)parent;

  return RESULT_COL_NUM;
}

int ResultsListModel::rowCount(const QModelIndex& parent) const
{
  (void)parent;

  if (m_scanner->getResultCount() > m_showThreshold)
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
      return QString::number(m_scanner->getResultsConsoleAddr().at(index.row()), 16).toUpper();
    case RESULT_COL_SCANNED:
      return QString::fromStdString(m_scanner->getFormattedScannedValueAt(index.row()));
    case RESULT_COL_CURRENT:
      return QString::fromStdString(m_scanner->getFormattedCurrentValueAt(index.row()));
    default:
      break;
    }
  }
  return QVariant();
}

bool ResultsListModel::removeRows(int row, int count, const QModelIndex& parent)
{
  beginRemoveRows(parent, row, row + count - 1);
  for (int i = 0; i < count; i++)
    m_scanner->removeResultAt(i + row);
  endRemoveRows();

  return true;
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
      return tr("Address");
    case RESULT_COL_SCANNED:
      return tr("Scanned");
    case RESULT_COL_CURRENT:
      return tr("Current");
    default:
      break;
    }
  }
  return QVariant();
}

void ResultsListModel::updateScanner()
{
  emit layoutChanged();
}

void ResultsListModel::updateAfterScannerReset()
{
  emit layoutChanged();
}

void ResultsListModel::setShowThreshold(const size_t showThreshold)
{
  m_showThreshold = showThreshold;
  if (showThreshold < m_scanner->getResultCount())
  {
    emit layoutChanged();
  }
}
