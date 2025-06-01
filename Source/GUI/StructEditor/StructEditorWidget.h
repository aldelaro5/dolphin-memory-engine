#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableView>
#include <QTreeView>
#include <QWidget>

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
  void onSelectDataEdited(const QModelIndex& index, const QVariant& oldNamespace, int role);
  void onSelectDropSucceeded(StructTreeNode* oldParent, StructTreeNode* movedNode);

  void onDetailContextMenuRequested(const QPoint& pos);
  void onDetailDoubleClicked(const QModelIndex& index);
  void onDetailDataEdited(const QModelIndex& index, const QVariant& value, int role);

  void onAddGroup();
  void onAddStruct();
  void onDuplicateStruct();
  void onDeleteNodes();
  void onEditStruct(StructTreeNode* node);
  void onUnloadStruct();
  bool unsavedStructDetails();
  void unloadStruct();
  void nameCollisionOnMove(QStringList collisions);

  StructTreeNode* getStructDefs();
  void readStructTreeFromFile(const QJsonObject& json, QMap<QString, QString>& map, bool clearTree);
  void writeStructDefTreeToJson(QJsonObject& json);
  void restoreStructTreeFromSettings(const QString& json);
  QString saveStructTreeToSettings();
  QMap<QString, StructDef*> getStructMap();

signals:
  void structAddedRemoved(
      QString fullName,
      StructDef* structDef = nullptr);  // If a struct is added, it includes a pointer to the
                                        // structDef, if it is removed, it is just a nullptr.
  void updateStructName(QString old_name, QString new_name);
  void updateStructDetails(QString fullName);

private:
  void initialiseWidgets();
  void makeLayouts();

  bool createNewFieldEntry(const QModelIndex& index);
  void editFieldEntry(const QModelIndex& index);

  void onDetailNameChanged();
  void updateChildStructNames(StructTreeNode* node, QString oldNameSpace);
  void onDetailLengthChanged();
  void onAddPaddingField(bool setSaveState = true);
  void onAddField();
  void onDuplicateField(const QModelIndex& index);
  void onDeleteFields();
  void onClearFields();
  void onSaveStruct();
  void nameChangeFailed(StructTreeNode* node, QString name);
  QString getNextAvailableName(StructTreeNode* parent, QString curName);
  void onLengthChange(u32 newLength);
  void onModifyStructReference(QString nodeName, QString target, bool addIt, bool& ok);
  void onModifyStructPointerReference(QString nodeName, QString target, bool addIt);
  void setupStructReferences();
  void updateStructReferenceNames(QString old_name, QString new_name);
  void updateStructReferenceFieldSize(StructTreeNode* node);
  void checkStructDetailSave();
  void getStructLength(QString name, int& len);

  void readStructDefTreeFromJson(const QJsonObject& json, QMap<QString, QString>& map);

  QStringList checkForMapCycles(QMap<QString, QStringList> map, QString curName = nullptr,
                                QString origName = nullptr);

  bool isAnyAncestorSelected(const QModelIndex& index) const;
  QModelIndexList simplifiedSelection() const;

  StructTreeNode* m_structRootNode;
  StructTreeNode* m_nodeInDetailEditor{};
  QMap<QString, QVector<QString>> m_structReferences{};
  QMap<QString, QVector<QString>> m_structPointerReferences{};

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
  QPushButton* m_btnUnloadStructDetails{};
  QPushButton* m_btnSaveStructDetails{};
  QPushButton* m_btnAddField{};
  QPushButton* m_btnDeleteFields{};
  QPushButton* m_btnClearFields{};
  QLineEdit* m_txtStructName{};
  QLineEdit* m_txtStructLength{};

  QRegularExpression m_numAppend = QRegularExpression("\\(\\d+\\)$");
};
