#pragma once

#include <QAbstractTableModel>

#include "../../MemoryScanner/MemoryScanner.h"

class ResultsListModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  enum
  {
    RESULT_COL_ADDRESS = 0,
    RESULT_COL_SCANNED,
    RESULT_COL_CURRENT,
    RESULT_COL_NUM
  };

  ResultsListModel(QObject* parent, MemScanner* scanner);
  ~ResultsListModel() override;

  int columnCount(const QModelIndex& parent) const override;
  int rowCount(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  bool removeRows(int row, int count, const QModelIndex& parent) override;

  u32 getResultAddress(const int row) const;
  void updateScanner();
  void updateAfterScannerReset();
  void setShowThreshold(size_t showThreshold);

private:
  MemScanner* m_scanner;
  size_t m_showThreshold{};
};
