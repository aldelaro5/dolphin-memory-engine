#pragma once

#include <QComboBox>
#include <QDialog>
#include <QSpinBox>

class DlgChangeType : public QDialog
{
  Q_OBJECT

public:
  DlgChangeType(QWidget* parent, int typeIndex, size_t length, QVector<QString> structNames,
                QString curStructName);
  int getTypeIndex() const;
  size_t getLength() const;
  QString getStructName() const;
  void accept() override;
  void onTypeChange(int index);
  void onUpdateStructNames(QVector<QString> structNames);
  void onUpdateStructName(QString oldName, QString newName);

private:
  void initialiseWidgets();
  void makeLayouts();

  int m_typeIndex;
  size_t m_length;
  QComboBox* m_cmbTypes{};
  QSpinBox* m_spnLength{};
  QWidget* m_lengthSelection{};
  QComboBox* m_structSelect{};
  QVector<QString> m_structNames{};
};
