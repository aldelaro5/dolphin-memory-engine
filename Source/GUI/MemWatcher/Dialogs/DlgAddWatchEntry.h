#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVector>

#include "../../../MemoryWatch/MemWatchEntry.h"
#include "../../Widgets/AddressInputWidget.h"

class DlgAddWatchEntry : public QDialog
{
public:
  DlgAddWatchEntry(bool newEntry, MemWatchEntry* entry, QVector<QString> structs, QWidget* parent,
                   bool isForStructField = false, int containerDepth = 0);
  ~DlgAddWatchEntry() override;

  DlgAddWatchEntry(const DlgAddWatchEntry&) = delete;
  DlgAddWatchEntry(DlgAddWatchEntry&&) = delete;
  DlgAddWatchEntry& operator=(const DlgAddWatchEntry&) = delete;
  DlgAddWatchEntry& operator=(DlgAddWatchEntry&&) = delete;

  void onTypeChange(int index);
  void accept() override;
  void onAddressChanged();
  void onIsPointerChanged();
  void onLengthChanged();
  void onOffsetChanged();
  MemWatchEntry* stealEntry();

private:
  void initialiseWidgets();
  void makeLayouts();
  void fillFields(MemWatchEntry* entry);

  void updatePreview();
  bool validateAndSetAddress();
  bool validateAndSetOffset(int index);
  void addPointerOffset();
  void removePointerOffset();
  void removeAllPointerOffset();
  void onPointerOffsetContextMenuRequested(const QPoint& pos);
  QString getContainerTypeText(MemWatchEntry* entry);
  void onSetupContainerContents();

  MemWatchEntry* m_entry{};
  AddressInputWidget* m_txbAddress{};
  QVector<QLineEdit*> m_offsets;
  QVector<QLabel*> m_addressPath;
  QGridLayout* m_offsetsLayout{};
  QCheckBox* m_chkBoundToPointer{};
  QLineEdit* m_lblValuePreview{};
  QLineEdit* m_txbLabel{};
  QComboBox* m_cmbTypes{};
  QSpinBox* m_spnLength{};
  QGroupBox* m_pointerWidget{};
  QPushButton* m_btnAddOffset{};
  QPushButton* m_btnRemoveOffset{};
  QComboBox* m_structSelect{};
  QVector<QString> m_structNames{};
  bool m_isForStructField;
  QSpinBox* m_spnContainerCount{};
  QPushButton* m_btnSetupContainerEntry{};
  QLabel* m_lblContainerType{};
  int m_curArrayDepth = 0;

  const QString noContainerSetText = "Container Type Not Set";
};
