#pragma once

#include <QComboBox>
#include <QDialog>
#include <QSpinBox>

#include <MemoryWatch/MemWatchEntry.h>

class DlgChangeType : public QDialog
{
  Q_OBJECT

public:
  DlgChangeType(QWidget* parent, int typeIndex, size_t length, QVector<QString> structNames,
                QString curStructName, size_t containerCount, MemWatchEntry* containerEntry);
  int getTypeIndex() const;
  size_t getLength() const;
  QString getStructName() const;
  size_t getContainerCount() const;
  MemWatchEntry* getContainerEntry();
  void accept() override;
  void onTypeChange(int index);

private:
  void initialiseWidgets();
  void makeLayouts();
  void onSetupContainerContents();

  int m_typeIndex;
  size_t m_length;
  size_t m_containerCount;
  MemWatchEntry* m_containerEntry{};
  QComboBox* m_cmbTypes{};
  QSpinBox* m_spnLength{};
  QWidget* m_lengthSelection{};
  QComboBox* m_structSelect{};
  QVector<QString> m_structNames{};
  QPushButton* m_btnSetupContainerEntry{};
  QSpinBox* m_spnContainerCount{};
  int m_curArrayDepth = 0;
};
