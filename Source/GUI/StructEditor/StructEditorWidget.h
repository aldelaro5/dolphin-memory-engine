#pragma once

#include <QWidget>
#include <QTableView>
#include <QTreeView>
#include <QPushButton>
#include <QLineEdit>

#include "StructDetailModel.h"
#include "StructSelectModel.h"

class StructEditorWidget : public QWidget
{
  Q_OBJECT

public:
  explicit StructEditorWidget(QWidget* parent);
  ~StructEditorWidget() override;

  StructEditorWidget(const StructEditorWidget&) = delete;
  StructEditorWidget(StructEditorWidget&&) = delete;
  StructEditorWidget& operator=(const StructEditorWidget&) = delete;
  StructEditorWidget& operator=(StructEditorWidget&&) = delete;

  void onSelectContextMenuRequested(const QPoint& pos);
  void onSelectDataEdited(const QModelIndex& index, const QVariant& value, int role);
  void onSelectDropSucceeded();

  void onDetailContextMenuRequested(const QPoint& pos);
  void onDetailDoubleClicked(const QModelIndex& index);
  void onDetailDataEdited(const QModelIndex& index, const QVariant& value, int role);

  void onAddGroup();
  void onAddStruct();
  void onDeleteNodes();
  void onEditStruct(StructTreeNode* node);

  StructTreeNode* getStructDefs();
  void restoreStructDefs(const QString& json);
  QString saveStructDefs();

signals:
  void structAddedRemoved(QString fullName, StructDef* structDef);
  void updateDlgStructList(QVector<QString> structs);
  void updateStructName(QString old_name, QString new_name);
  void updateStructDetails(QString fullName);

private:
  void initialiseWidgets();
  void makeLayouts();

  void onConvertPaddingToEntry(const QModelIndex& index);

  void onDetailNameChanged();
  void updateChildStructNames(StructTreeNode* node, QString oldNameSpace);
  void onDetailLengthChanged();
  void onAddField();
  void onDeleteFields();
  void onClearFields();
  void onSaveStruct();

  bool isAnyAncestorSelected(const QModelIndex& index) const;
  QModelIndexList simplifiedSelection() const;

  StructTreeNode* m_structDefs;
  bool m_unsavedChanges = false;


  StructDetailModel* m_structDetailModel{};
  QTableView* m_structDetailView;

  StructSelectModel* m_structSelectModel{};
  QTreeView* m_structSelectView;

  // For Struct Selector
  QPushButton* m_btnAddStruct{};
  QPushButton* m_btnAddGroup{};
  QPushButton* m_btnDeleteNodes{};

  // For Struct Details
  QPushButton* m_btnSaveStructs{};
  QPushButton* m_btnAddField{};
  QPushButton* m_btnDeleteFields{};
  QPushButton* m_btnClearFields{};
  QLineEdit* m_txtStructName{};
  QLineEdit* m_txtStructLength{};
};
