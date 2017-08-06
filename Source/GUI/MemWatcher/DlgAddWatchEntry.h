#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVector>

#include "../../MemoryWatch/MemoryWatch.h"

class DlgAddWatchEntry : public QDialog
{
public:
  DlgAddWatchEntry(MemWatchEntry* entry);
  void onTypeChange(int index);
  void accept();
  void onAddressChanged();
  void onIsPointerChanged();
  void onLengthChanged();
  void onOffsetChanged();
  MemWatchEntry* getEntry() const;

private:
  void updateValuePreview();
  bool validateAndSetAddress();
  bool validateAndSetOffset(int index);
  void addPointerOffset();
  void removePointerOffset();

  MemWatchEntry* m_entry;
  QLineEdit* m_txbAddress;
  QVector<QLineEdit*> m_offsetsWidgets;
  QFormLayout* m_offsetsLayout;
  QCheckBox* m_chkBoundToPointer;
  QLabel* m_lblValuePreview;
  QLineEdit* m_txbLabel;
  QComboBox* m_cmbTypes;
  QSpinBox* m_spnLength;
  QWidget* m_lengtWidget;
  QWidget* m_pointerWidget;
  QPushButton* m_btnAddOffset;
  QPushButton* m_btnRemoveOffset;
};