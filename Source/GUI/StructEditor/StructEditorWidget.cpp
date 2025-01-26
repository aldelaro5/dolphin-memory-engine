#include "StructEditorWidget.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMenu>

#include "../../Common/MemoryCommon.h"
#include "../MemWatcher/Dialogs/DlgAddWatchEntry.h"

class LengthValidator : public QValidator
{
public:
  LengthValidator(QObject* parent) : QValidator(parent) {

  }
  QValidator::State validate(QString& input, int& pos) const override
  {
    if (input.isEmpty())
      return Intermediate;

    bool ok;
    int value = input.toInt(&ok, 16);

    if (!ok)
      return Invalid;

    if (value > fmax(Common::GetMEM1Size(), Common::GetMEM2Size()))
      return Invalid;

    return Acceptable;
  }
};

StructEditorWidget::StructEditorWidget(QWidget* parent)
{
  m_structDefs = new StructTreeNode(nullptr, nullptr);
  setAttribute(Qt::WA_AlwaysShowToolTips);
  setWindowTitle("DME - Struct Editor");
  initialiseWidgets();
  makeLayouts();
}

StructEditorWidget::~StructEditorWidget()
{
}

void StructEditorWidget::initialiseWidgets()
{
  // Initialize objects for the struct selector

  m_structSelectModel = new StructSelectModel(this, m_structDefs);
  connect(m_structSelectModel, &StructSelectModel::dataEdited, this,
          &StructEditorWidget::onSelectDataEdited);
  connect(m_structSelectModel, &StructSelectModel::dropSucceeded, this,
          &StructEditorWidget::onSelectDropSucceeded);
  connect(m_structSelectModel, &StructSelectModel::nameChangeFailed, this,
          &StructEditorWidget::nameChangeFailed);

  m_structSelectView = new QTreeView;
  m_structSelectView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_structSelectView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_structSelectView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_structSelectView, &QWidget::customContextMenuRequested, this,
          &StructEditorWidget::onSelectContextMenuRequested);
  m_structSelectView->setDragEnabled(true);
  m_structSelectView->setAcceptDrops(true);
  m_structSelectView->setDragDropMode(QAbstractItemView::InternalMove);
  m_structSelectView->setDropIndicatorShown(true);
  m_structSelectView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_structSelectView->setSelectionMode(QAbstractItemView::ContiguousSelection);
  m_structSelectView->setModel(m_structSelectModel);


  m_btnAddGroup = new QPushButton(tr("Add Struct Group"), this);
  connect(m_btnAddGroup, &QPushButton::clicked, this, &StructEditorWidget::onAddGroup);

  m_btnAddStruct = new QPushButton(tr("Add Struct"), this);
  connect(m_btnAddStruct, &QPushButton::clicked, this, &StructEditorWidget::onAddStruct);

  m_btnDeleteNodes = new QPushButton(tr("Delete"), this);
  connect(m_btnDeleteNodes, &QPushButton::clicked, this, &StructEditorWidget::onDeleteNodes);

  // Initialize objects for the Struct Detail editor

  m_structDetailModel = new StructDetailModel(this);
  connect(m_structDetailModel, &StructDetailModel::dataEdited, this,
          &StructEditorWidget::onDetailDataEdited);

  //connect as neeeded
  /* Example from MemWatchWidget
  connect(m_watchModel, &MemWatchModel::writeFailed, this, &MemWatchWidget::onValueWriteError);
  connect(m_watchModel, &MemWatchModel::readFailed, this, &MemWatchWidget::mustUnhook);
  connect(m_watchModel, &MemWatchModel::rowsInserted, this, &MemWatchWidget::onRowsInserted);
   */

  //Not sure if I will need this yet
  //m_detailDelegate() = new StructDetailDelegate();

  m_structDetailView = new QTableView;
  m_structDetailView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_structDetailView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_structDetailView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_structDetailView, &QWidget::customContextMenuRequested, this,
          &StructEditorWidget::onDetailContextMenuRequested);
  connect(m_structDetailView, &QAbstractItemView::doubleClicked, this,
          &StructEditorWidget::onDetailDoubleClicked);
  m_structDetailView->setDragEnabled(false);
  m_structDetailView->setAcceptDrops(false);
  m_structDetailView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_structDetailView->setSelectionMode(QAbstractItemView::ContiguousSelection);
  m_structDetailView->setModel(m_structDetailModel);
  //m_watchView->setItemDelegate(m_watchDelegate);

  m_btnSaveStructs = new QPushButton(tr("S"), this);
  connect(m_btnSaveStructs, &QPushButton::clicked, this, &StructEditorWidget::onSaveStruct);
  m_btnSaveStructs->setToolTip("Add fields and update struct length.");

  m_btnAddField = new QPushButton(tr("+"), this);
  connect(m_btnAddField, &QPushButton::clicked, this, &StructEditorWidget::onAddField);
  m_btnAddField->setToolTip("Add fields and update struct length.");

  m_btnDeleteFields = new QPushButton(tr("-"), this);
  connect(m_btnDeleteFields, &QPushButton::clicked, this, &StructEditorWidget::onDeleteFields);
  m_btnDeleteFields->setToolTip("Delete fields and update struct length.");

  m_btnClearFields = new QPushButton(tr("0"), this);
  connect(m_btnClearFields, &QPushButton::clicked, this, &StructEditorWidget::onClearFields);
  m_btnClearFields->setToolTip("Clear fields and replace with padding.");

  m_txtStructName = new QLineEdit(this);
  connect(m_txtStructName, &QLineEdit::editingFinished, this,
          &StructEditorWidget::onDetailNameChanged);
  m_txtStructName->setDisabled(true);
  
  m_txtStructLength = new QLineEdit(this);
  LengthValidator* hexValidator = new LengthValidator(this);
  m_txtStructLength->setValidator(hexValidator);
  connect(m_txtStructLength, &QLineEdit::editingFinished, this,
          &StructEditorWidget::onDetailLengthChanged);
  m_txtStructLength->setDisabled(true);
}
  

void StructEditorWidget::makeLayouts()
{
  QWidget* structDefButtons = new QWidget;
  QHBoxLayout* structDefButtonLayout = new QHBoxLayout;

  structDefButtonLayout->addWidget(m_btnAddGroup);
  structDefButtonLayout->addWidget(m_btnAddStruct);
  structDefButtonLayout->addWidget(m_btnDeleteNodes);
  structDefButtonLayout->setContentsMargins(0, 0, 0, 0);
  structDefButtons->setLayout(structDefButtonLayout);
  
  QWidget* structSelectPanel = new QWidget;
  QVBoxLayout* structSelectPanelLayout = new QVBoxLayout;

  structSelectPanelLayout->addWidget(m_structSelectView);
  structSelectPanelLayout->addWidget(structDefButtons);
  structSelectPanelLayout->setContentsMargins(0, 0, 0, 0);
  structSelectPanel->setLayout(structSelectPanelLayout);

  QWidget* structEditButtons = new QWidget;
  QHBoxLayout* structEditButtonLayout = new QHBoxLayout;

  structEditButtonLayout->addWidget(m_btnSaveStructs);
  structEditButtonLayout->addWidget(m_btnAddField);
  structEditButtonLayout->addWidget(m_btnDeleteFields);
  structEditButtonLayout->addWidget(m_btnClearFields);
  structEditButtons->setLayout(structEditButtonLayout);

  QFormLayout* structDetails = new QFormLayout;
  structDetails->addRow("Struct Name:", m_txtStructName);
  structDetails->addRow("Struct Length:  0x", m_txtStructLength);

  QWidget* structEditPanel = new QWidget;
  QVBoxLayout* structEditPanelLayout = new QVBoxLayout;

  structEditPanelLayout->addWidget(structEditButtons);
  structEditPanelLayout->addWidget(m_structDetailView);
  structEditPanelLayout->addLayout(structDetails);
  structEditPanelLayout->setContentsMargins(0, 0, 0, 0);
  structEditPanel->setLayout(structEditPanelLayout);
  
  QHBoxLayout* widgetLayout = new QHBoxLayout;
  widgetLayout->addWidget(structSelectPanel);
  widgetLayout->addWidget(structEditPanel);
  widgetLayout->setContentsMargins(3, 0, 3, 0);
  setLayout(widgetLayout);
}

void StructEditorWidget::onConvertPaddingToEntry(const QModelIndex& index)
{
  DlgAddWatchEntry dlg(true, nullptr, m_structDefs->getStructNames(), this);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_structDetailModel->updateFieldEntry(dlg.stealEntry(), index);
    m_unsavedChanges = true;
  }

  m_btnSaveStructs->setEnabled(true);
}

void StructEditorWidget::onDetailNameChanged()
{
  StructTreeNode* node = m_structDetailModel->getLoadedStructNode();

  if (!node->getParent()->isNameAvailable(m_txtStructName->text()))
  {
    return nameChangeFailed(node, m_txtStructName->text());
  }

  QString oldNameSpace = node->getParent()->getNameSpace();
  QString oldFullName = node->getNameSpace();
  m_structSelectModel->setNodeLabel(node, m_txtStructName->text());

  emit updateStructName(oldFullName, node->appendNameToNameSpace(oldNameSpace));
}

void StructEditorWidget::onDetailLengthChanged()
{
  StructTreeNode* node = m_structDetailModel->getLoadedStructNode();
  u32 old_length = node->getStructDef()->getLength();
  bool ok;
  u32 new_length = m_txtStructLength->text().toInt(&ok, 16);

  if (new_length == old_length)
    return;

  if (new_length < old_length && m_structDetailModel->willRemoveFields(new_length))
  {
    QString msg =
        "The new length of this struct is smaller and will remove the following fields:\n" +
        m_structDetailModel->getRemovedFieldDescriptions(new_length) +
        "\n\nWould you like to continue?";
    if (QMessageBox::No == QMessageBox::warning(this, "New Length will remove fields", msg,
                                                QMessageBox::Yes, QMessageBox::No))
    {
      m_txtStructLength->setText(QString::number(old_length, 16));
      return;
    }
  }
  m_structDetailModel->updateFieldsWithNewLength();
  m_btnSaveStructs->setEnabled(true);
}

void StructEditorWidget::onAddField()
{
  u32 cur_length = m_structDetailModel->getLoadedStructNode()->getStructDef()->getLength();
  m_structDetailModel->getLoadedStructNode()->getStructDef()->setLength(cur_length + 1);
  m_structDetailModel->updateFieldsWithNewLength();
  m_txtStructLength->setText(QString::number(cur_length + 1, 16));

  m_btnSaveStructs->setEnabled(true);
}

void StructEditorWidget::onDeleteFields()
{
  const QModelIndexList selection = m_structDetailView->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
  {
    m_structDetailModel->removeLastField();
  }
  else
  {
    m_structDetailModel->removeFields(selection);
  }

  m_txtStructLength->setText(
      QString::number(m_structDetailModel->getLoadedStructNode()->getStructDef()->getLength(), 16));

  m_btnSaveStructs->setEnabled(true);
}

void StructEditorWidget::onClearFields()
{
  const QModelIndexList selection = m_structDetailView->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
    return;
  m_structDetailModel->clearFields(selection);
  m_btnSaveStructs->setEnabled(true);
}

void StructEditorWidget::onSaveStruct()
{
  bool lengthSet = false;
  m_structDetailModel->getLoadedStructNode()
      ->getStructDef()
      ->setLength(m_txtStructLength->text().toUInt(&lengthSet, 16));
  m_structDetailModel->saveStruct();
  emit updateStructDetails(m_structDetailModel->getLoadedStructNode()->getNameSpace());
  m_btnSaveStructs->setDisabled(true);
}

void StructEditorWidget::nameChangeFailed(StructTreeNode* node, QString name)
{
  QString msg = "There is already a node named " + m_txtStructName->text() + " in the namespace " +
                node->getParent()->getNameSpace() + ". Reverting name to " + node->getName();
  m_txtStructName->setText(node->getName());
  QMessageBox::critical(this, "Name in Use!", msg);
  return;
}

void StructEditorWidget::onSelectContextMenuRequested(const QPoint& pos)
{
  QModelIndex index = m_structSelectView->indexAt(pos);
  QMenu* contextMenu = new QMenu(this);
  StructTreeNode* node = nullptr;
  if (index != QModelIndex())
  {
    node = StructSelectModel::getTreeNodeFromIndex(index);
    if (!node->isGroup())
    {
      QAction* const editStruct{new QAction(tr("&Edit Struct"), this)};
      connect(editStruct, &QAction::triggered, this, [this, node] { StructEditorWidget::onEditStruct(node); } );
      contextMenu->addAction(editStruct);
      contextMenu->addSeparator();
    }
  }

  if (!node || node->isGroup())
  {
    QAction* const addGroup{new QAction(tr("Add group"), this)};
    connect(addGroup, &QAction::triggered, this, &StructEditorWidget::onAddGroup);
    contextMenu->addAction(addGroup);
    QAction* const addStruct{new QAction(tr("Add struct"), this)};
    connect(addStruct, &QAction::triggered, this, &StructEditorWidget::onAddStruct);
    contextMenu->addAction(addStruct);
    contextMenu->addSeparator();
  }

  if (node != nullptr && simplifiedSelection().count() > 0)
  {
    QAction* const deleteNodes{new QAction(tr("Delete selection"), this)};
    connect(deleteNodes, &QAction::triggered, this, &StructEditorWidget::onDeleteNodes);
    contextMenu->addAction(deleteNodes);
  }
  
  contextMenu->popup(m_structSelectView->viewport()->mapToGlobal(pos));
}

void StructEditorWidget::onSelectDataEdited(const QModelIndex& index, const QVariant& value,
                                            int role)
{
  if (role != Qt::EditRole)
  {
    return;
  }

  if (index.column() == StructSelectModel::WATCH_COL_LABEL)
  {
    StructTreeNode* node = static_cast<StructTreeNode*>(index.internalPointer());
    QString oldNameSpace = node->getParent()->getNameSpace();
    QString oldFullName = node->getNameSpace();

    if (!node->isGroup())
    {
      emit updateStructName(oldFullName, node->appendNameToNameSpace(oldNameSpace));
    }
    else
    {
      updateChildStructNames(node, node->appendNameToNameSpace(oldNameSpace));
    }
  }
}

void StructEditorWidget::updateChildStructNames(StructTreeNode* node, QString oldNameSpace)
{
  QString newNameSpace = QString();
  for (StructTreeNode* child : node->getChildren())
  {
    if (child->isGroup())
      updateChildStructNames(child, child->appendNameToNameSpace(oldNameSpace));
    else
    {
      if (newNameSpace.isEmpty())
        newNameSpace = node->getNameSpace();
      emit updateStructName(child->appendNameToNameSpace(oldNameSpace),
                            child->appendNameToNameSpace(newNameSpace));
    }
  }
}

void StructEditorWidget::onSelectDropSucceeded()
{
}

void StructEditorWidget::onDetailContextMenuRequested(const QPoint& pos)
{
  QModelIndex index = m_structDetailView->indexAt(pos);
  QMenu* contextMenu = new QMenu(this);
  
  QAction* const addField{new QAction(tr("Add field"), this)};
  connect(addField, &QAction::triggered, this, &StructEditorWidget::onAddField);
  contextMenu->addAction(addField);
  if (index != QModelIndex())
  {
    FieldDef* node = static_cast<FieldDef*>(index.internalPointer());
    if (node == nullptr)
      node = m_structDetailModel->getFieldByRow(index.row());
    if (node != nullptr)
    {
      if (node->isPadding())
      {
        QAction* const createEntry{new QAction(tr("Create field entry"), this)};
        connect(createEntry, &QAction::triggered, this,
                [this, index] { onConvertPaddingToEntry(index); });
        contextMenu->addAction(createEntry);
      }
      else
      {
        QAction* const clearField{new QAction(tr("Clear field"), this)};
        connect(clearField, &QAction::triggered, this, &StructEditorWidget::onClearFields);
        contextMenu->addAction(clearField);
      }
    }
  }

  contextMenu->popup(m_structDetailView->viewport()->mapToGlobal(pos));
}

void StructEditorWidget::onDetailDoubleClicked(const QModelIndex& index)
{
  FieldDef* field = static_cast<FieldDef*>(index.internalPointer());

  if (field->isPadding())
    return onConvertPaddingToEntry(index);

  DlgAddWatchEntry dlg(false, field->getEntry(), m_structDefs->getStructNames(), this);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_structDetailModel->updateFieldEntry(dlg.stealEntry(), index);
    m_unsavedChanges = true;
  }

  m_btnSaveStructs->setEnabled(true);
}

void StructEditorWidget::onDetailDataEdited(const QModelIndex& index, const QVariant& value,
                                            int role)
{
  if (role != Qt::EditRole)
    return;

  if (index.column() == StructDetailModel::STRUCT_COL_LABEL)
  {
    m_unsavedChanges = true;
  }
}

void StructEditorWidget::onAddGroup()
{
  const QModelIndexList selectedIndexes{m_structSelectView->selectionModel()->selectedIndexes()};
  const QModelIndex lastIndex = selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back();
  const StructTreeNode* parentNode =  selectedIndexes.empty() ? m_structDefs : static_cast<StructTreeNode*>(lastIndex.internalPointer());

  bool ok = false;
  QString textIn;
  while (true)
  {
    textIn = QInputDialog::getText(this, "Add a struct group",
                                         "Enter the group name:", QLineEdit::Normal, "", &ok);

    if (!ok || textIn.isEmpty())
      return;

    if (parentNode->isNameAvailable(textIn))
      break;

    QString msg = "Cannot use this name for a group. It is already in use:\n" + textIn +
                  "\n\nPlease enter a different name.";
    QMessageBox::critical(this, "Error - duplicate name", msg);
  }

  // May want to enclose the selected indices in a new node... In this case, we would need to emit name changes for any struct that was moved...
  m_structSelectModel->addGroup(textIn, lastIndex);
  m_unsavedChanges = true;
}

void StructEditorWidget::onAddStruct()
{
  const QModelIndexList selectedIndexes{m_structSelectView->selectionModel()->selectedIndexes()};
  const QModelIndex lastIndex = selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back();
  const StructTreeNode* parentNode = selectedIndexes.empty() ? m_structDefs : static_cast<StructTreeNode*>(lastIndex.internalPointer());

  bool ok = false;
  QString text;
  while (true)
  {
    text = QInputDialog::getText(this, "Add a struct",
                                         "Enter the struct name:", QLineEdit::Normal, "", &ok);

    if (!ok || text.isEmpty())
      return;

    if (parentNode->isNameAvailable(text))
      break;

    QString msg = "This struct name is in use: " + text + ".\nPlease enter a different name.";
    QMessageBox::critical(this, "Error - duplicate struct name", msg);
  }

  StructTreeNode* addedNode = m_structSelectModel->addStruct(text, lastIndex);
  m_unsavedChanges = true;

  emit structAddedRemoved(addedNode->getNameSpace(), addedNode->getStructDef());
  emit updateDlgStructList(m_structDefs->getStructNames());

}

bool StructEditorWidget::isAnyAncestorSelected(const QModelIndex& index) const
{
  if (m_structSelectModel->parent(index) == QModelIndex())
    return false;
  if (m_structSelectView->selectionModel()->isSelected(index.parent()))
    return true;
  return isAnyAncestorSelected(index.parent());
}

QModelIndexList StructEditorWidget::simplifiedSelection() const
{
  QModelIndexList simplifiedSelection;
  QModelIndexList selection = m_structSelectView->selectionModel()->selectedRows();

  // Discard all indexes whose parent is selected already
  for (int i = 0; i < selection.count(); ++i)
  {
    const QModelIndex index = selection.at(i);
    if (!isAnyAncestorSelected(index))
    {
      simplifiedSelection.append(index);
    }
  }
  return simplifiedSelection;
}


void StructEditorWidget::onDeleteNodes()
{
  const QModelIndexList selection = m_structSelectView->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
    return;

  for (const auto& index : simplifiedSelection())
  {
    StructTreeNode* curNode = m_structSelectModel->getTreeNodeFromIndex(index);
    if (curNode->isGroup())
    {
      QVector<StructTreeNode*> queue = curNode->getChildren();

      while (!queue.isEmpty())
      {
        curNode = queue.takeFirst();

        if (curNode->isGroup())
          for (StructTreeNode* node : curNode->getChildren())
            queue.push_front(node);
        else
          emit structAddedRemoved(curNode->getNameSpace());
      }
    }
    else
    {
      emit structAddedRemoved(curNode->getNameSpace());
    }

    m_structSelectModel->deleteNode(index);
  }

  emit updateDlgStructList(m_structDefs->getStructNames());
}

void StructEditorWidget::onEditStruct(StructTreeNode* node)
{
  if (node->isGroup())
    return;
  if (m_structDetailModel->hasStructLoaded() && m_unsavedChanges)
  {
    QMessageBox::StandardButton response = QMessageBox::question(this, "Save Changes?", "You have unsaved changes to this struct.\nWould you like to save them?");
    if (response == QMessageBox::StandardButton::Yes)
    {
      onSaveStruct();
    }
  }
  m_structDetailModel->loadStruct(node);

  m_txtStructName->setEnabled(true);
  m_txtStructName->setText(node->getName());

  m_txtStructLength->setEnabled(true);
  m_txtStructLength->setText(QString::number(node->getStructDef()->getLength(), 16).toUpper());
}

void StructEditorWidget::restoreStructDefs(const QString& json)
{
  const QJsonDocument loadDoc(QJsonDocument::fromJson(json.toUtf8()));
  m_structDefs->readFromJson(loadDoc.object());
}

QString StructEditorWidget::saveStructDefs()
{
  QJsonObject root;
  m_structDefs->writeToJson(root);
  QJsonDocument saveDoc(root);
  return saveDoc.toJson();
}

QMap<QString, StructDef*> StructEditorWidget::getStructMap()
{

  return m_structSelectModel->getStructMap();
}

StructTreeNode* StructEditorWidget::getStructDefs()
{
  return m_structDefs;
}
