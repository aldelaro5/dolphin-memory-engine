#include "StructEditorWidget.h"

#include <QCoreApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QMessageBox>

#include "../../Common/MemoryCommon.h"
#include "../MemWatcher/Dialogs/DlgAddWatchEntry.h"

class LengthValidator : public QValidator
{
public:
  LengthValidator(QObject* parent) : QValidator(parent) {}
  QValidator::State validate(QString& input, int& pos) const override
  {
    (void)pos;
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

StructEditorWidget::StructEditorWidget(QWidget* const parent) : QWidget(parent)
{
  m_structRootNode = new StructTreeNode(nullptr, nullptr);
  setAttribute(Qt::WA_AlwaysShowToolTips);
  setWindowTitle("DME - Struct Editor");
  initialiseWidgets();
  makeLayouts();
  adjustSize();
}

StructEditorWidget::~StructEditorWidget()
{
}

void StructEditorWidget::initialiseWidgets()
{
  // Initialize objects for the struct selector

  m_structSelectModel = new StructSelectModel(this, m_structRootNode);
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
  m_structSelectView->setMinimumSize(0, 50);

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
  connect(m_structDetailModel, &StructDetailModel::lengthChanged, this,
          &StructEditorWidget::onLengthChange);
  connect(m_structDetailModel, &StructDetailModel::modifyStructReference, this,
          &StructEditorWidget::onModifyStructReference);

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
  m_structDetailView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_structDetailView->setModel(m_structDetailModel);
  m_structDetailView->verticalHeader()->setDefaultSectionSize(15);
  m_structDetailView->verticalHeader()->setMinimumSectionSize(15);
  m_structDetailView->setDisabled(true);

  QHeaderView* header = m_structDetailView->horizontalHeader();
  header->setStretchLastSection(true);
  header->resizeSection(StructDetailModel::STRUCT_COL_OFFSET, 80);
  header->resizeSection(StructDetailModel::STRUCT_COL_SIZE, 50);
  header->resizeSection(StructDetailModel::STRUCT_COL_LABEL, 150);
  header->resizeSection(StructDetailModel::STRUCT_COL_DETAIL, 150);

  m_btnUnloadStructDetails = new QPushButton(tr("Close"), this);
  connect(m_btnUnloadStructDetails, &QPushButton::clicked, this,
          &StructEditorWidget::onUnloadStruct);
  m_btnUnloadStructDetails->setToolTip("Close struct.");
  m_btnUnloadStructDetails->setDisabled(true);
  m_btnUnloadStructDetails->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_btnSaveStructDetails = new QPushButton(tr("Save"), this);
  connect(m_btnSaveStructDetails, &QPushButton::clicked, this, &StructEditorWidget::onSaveStruct);
  m_btnSaveStructDetails->setToolTip("Save struct details.");
  m_btnSaveStructDetails->setDisabled(true);
  m_btnSaveStructDetails->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_btnAddField = new QPushButton(tr("Add"), this);
  m_btnAddField->setToolTip("Add field and update struct length.");
  m_btnAddField->setDisabled(true);
  m_btnAddField->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QMenu* menu = new QMenu(this);
  menu->addAction("Field", this, &StructEditorWidget::onAddField);
  menu->addAction("Padding", this, &StructEditorWidget::onAddPaddingField);
  m_btnAddField->setMenu(menu);

  m_btnDeleteFields = new QPushButton(tr("Delete"), this);
  connect(m_btnDeleteFields, &QPushButton::clicked, this, &StructEditorWidget::onDeleteFields);
  m_btnDeleteFields->setToolTip("Delete fields and update struct length.");
  m_btnDeleteFields->setDisabled(true);
  m_btnDeleteFields->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_btnClearFields = new QPushButton(tr("Clear"), this);
  connect(m_btnClearFields, &QPushButton::clicked, this, &StructEditorWidget::onClearFields);
  m_btnClearFields->setToolTip("Clear fields and replace with padding.");
  m_btnClearFields->setDisabled(true);
  m_btnClearFields->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

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

  QSizePolicy selectTreePolicy = QSizePolicy();
  selectTreePolicy.setHorizontalStretch(0);
  selectTreePolicy.setHorizontalPolicy(QSizePolicy::Policy::Minimum);
  selectTreePolicy.setVerticalStretch(1);
  selectTreePolicy.setVerticalPolicy(QSizePolicy::Policy::Ignored);
  m_structSelectView->setSizePolicy(selectTreePolicy);
  structSelectPanelLayout->addWidget(m_structSelectView);
  structSelectPanelLayout->addWidget(structDefButtons);
  structSelectPanelLayout->setContentsMargins(3, 3, 0, 3);
  structSelectPanel->setLayout(structSelectPanelLayout);

  QHBoxLayout* structDetailButtonLayout = new QHBoxLayout;
  structDetailButtonLayout->addWidget(m_btnUnloadStructDetails);
  structDetailButtonLayout->addWidget(m_btnSaveStructDetails);
  structDetailButtonLayout->addStretch();
  structDetailButtonLayout->addWidget(m_btnAddField);
  structDetailButtonLayout->addWidget(m_btnDeleteFields);
  structDetailButtonLayout->addWidget(m_btnClearFields);
  structDetailButtonLayout->setContentsMargins(0, 0, 0, 0);

  QFormLayout* structDetails = new QFormLayout;
  structDetails->addRow("Struct Name:", m_txtStructName);
  structDetails->addRow("Struct Length:  0x", m_txtStructLength);

  QWidget* structEditPanel = new QWidget;
  QVBoxLayout* structEditPanelLayout = new QVBoxLayout;

  structEditPanelLayout->addLayout(structDetailButtonLayout);
  structEditPanelLayout->addWidget(m_structDetailView);
  structEditPanelLayout->addLayout(structDetails);
  structEditPanelLayout->setContentsMargins(0, 3, 3, 3);
  structEditPanel->setLayout(structEditPanelLayout);

  QHBoxLayout* widgetLayout = new QHBoxLayout;
  widgetLayout->addWidget(structSelectPanel);
  widgetLayout->addWidget(structEditPanel);
  widgetLayout->setContentsMargins(3, 3, 3, 3);
  setLayout(widgetLayout);
}

bool StructEditorWidget::createNewFieldEntry(const QModelIndex& index)
{
  DlgAddWatchEntry dlg(true, nullptr, m_structRootNode->getStructNames(), this, true);
  if (dlg.exec() == QDialog::Accepted)
  {
    if (!m_structDetailModel->updateFieldEntry(new MemWatchEntry(dlg.stealEntry()), index))
      return false;
  }
  else
    return false;

  m_btnSaveStructDetails->setEnabled(true);
  return true;
}

void StructEditorWidget::editFieldEntry(const QModelIndex& index)
{
  FieldDef* field = m_structDetailModel->getFieldByRow(index.row());

  DlgAddWatchEntry dlg(false, new MemWatchEntry(field->getEntry()),
                       m_structRootNode->getStructNames(), this, true);
  if (dlg.exec() == QDialog::Accepted)
  {
    if (!m_structDetailModel->updateFieldEntry(new MemWatchEntry(dlg.stealEntry()), index))
      return;
  }

  m_btnSaveStructDetails->setEnabled(true);
}

void StructEditorWidget::onDetailNameChanged()
{
  if (m_nodeInDetailEditor->getName() == m_txtStructName->text())
  {
    m_txtStructName->clearFocus();
    return;
  }

  StructTreeNode* node = m_structDetailModel->getLoadedStructNode();

  if (!node->getParent()->isNameAvailable(m_txtStructName->text()))
  {
    m_txtStructName->clearFocus();
    return nameChangeFailed(node, m_txtStructName->text());
  }

  QString oldFullName = node->getNameSpace();
  m_nodeInDetailEditor->setName(m_txtStructName->text());
  node->setName(m_txtStructName->text());

  QString newFullName = node->getNameSpace();

  m_structDetailModel->updateStructTypeLabel(oldFullName, newFullName);
  node->getStructDef()->updateStructTypeLabel(oldFullName, newFullName);
  updateStructReferenceNames(oldFullName, newFullName);
  emit updateStructName(oldFullName, newFullName);
  m_txtStructName->clearFocus();
}

void StructEditorWidget::onDetailLengthChanged()
{
  StructTreeNode* node = m_structDetailModel->getLoadedStructNode();
  u32 old_length = node->getStructDef()->getLength();
  bool ok;
  u32 new_length = m_txtStructLength->text().toInt(&ok, 16);

  if (new_length == old_length)
  {
    m_txtStructLength->clearFocus();
    return;
  }

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
      m_txtStructLength->clearFocus();
      return;
    }
  }

  node->getStructDef()->setLength(new_length);
  m_structDetailModel->updateFieldsWithNewLength();
  m_btnSaveStructDetails->setEnabled(true);
  m_txtStructLength->clearFocus();
}

void StructEditorWidget::onAddPaddingField(bool setSaveState)
{
  QModelIndex targetIndex = QModelIndex();

  const QModelIndexList selection = m_structDetailView->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
    m_structDetailModel->addPaddingFields(1);
  else
  {
    int targetRow = selection.last().row() + 1;
    m_structDetailModel->addPaddingFields(1, targetRow);
    targetIndex = m_structDetailModel->getIndexAt(targetRow);
  }

  if (!targetIndex.isValid())
    targetIndex = m_structDetailModel->getLastIndex();

  m_structDetailView->selectionModel()->clearSelection();
  m_structDetailView->selectionModel()->select(targetIndex, QItemSelectionModel::Select |
                                                                QItemSelectionModel::Rows);
  QCoreApplication::processEvents();

  if (setSaveState)
    m_btnSaveStructDetails->setEnabled(true);
}

void StructEditorWidget::onAddField()
{
  onAddPaddingField(false);

  bool ok;
  const QModelIndexList selection = m_structDetailView->selectionModel()->selectedIndexes();

  Q_ASSERT(!selection.isEmpty());
  ok = createNewFieldEntry(m_structDetailModel->getIndexAt(selection.last().row()));

  if (!ok)
    onDeleteFields();
  else
    m_btnSaveStructDetails->setEnabled(true);
}

void StructEditorWidget::onDuplicateField(const QModelIndex& index)
{
  FieldDef* newField = new FieldDef(m_structDetailModel->getFieldByRow(index.row()));
  QModelIndex insertIndex = index.siblingAtRow(index.row() + 1);
  m_structDetailModel->addField(insertIndex, newField);
}

void StructEditorWidget::onDeleteFields()
{
  const QModelIndexList selection = m_structDetailView->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
    m_structDetailModel->removeLastField();
  else
    m_structDetailModel->removeFields(selection);

  m_txtStructLength->setText(
      QString::number(m_structDetailModel->getLoadedStructNode()->getStructDef()->getLength(), 16));

  m_btnSaveStructDetails->setEnabled(true);
}

void StructEditorWidget::onClearFields()
{
  const QModelIndexList selection = m_structDetailView->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
    return;
  m_structDetailModel->clearFields(selection);
  m_btnSaveStructDetails->setEnabled(true);
}

void StructEditorWidget::onSaveStruct()
{
  m_structDetailModel->saveStruct();
  m_nodeInDetailEditor->setStructDef(
      new StructDef(m_structDetailModel->getLoadedStructNode()->getStructDef()));
  emit structAddedRemoved(m_nodeInDetailEditor->getNameSpace(),
                          m_nodeInDetailEditor->getStructDef());
  updateStructReferenceFieldSize(m_nodeInDetailEditor);
  m_btnSaveStructDetails->setDisabled(true);
}

void StructEditorWidget::nameChangeFailed(StructTreeNode* node, QString name)
{
  QString msg = "There is already a node named " + name + " in the namespace " +
                node->getParent()->getNameSpace() + ". Reverting name to " + node->getName();
  m_txtStructName->setText(node->getName());
  QMessageBox::critical(this, "Name in Use!", msg);
  return;
}

void StructEditorWidget::onLengthChange(u32 newLength)
{
  m_txtStructLength->setText(QString::number(newLength, 16));
}

void StructEditorWidget::onModifyStructReference(QString nodeName, QString target, bool addRef,
                                                 bool& ok)
{
  if (addRef)
  {
    if (!m_structReferences.contains(target))
      m_structReferences.insert(target, {nodeName});
    else if (!m_structReferences[target].contains(nodeName))
      m_structReferences[target].push_back(nodeName);
    else
      return;

    QStringList cycle = checkForMapCycles(m_structReferences);
    ok = cycle.isEmpty();
    if (ok)
      return;

    bool _;
    onModifyStructReference(nodeName, target, false, _);
    cycle.push_back(QString(" -- ") + cycle.takeLast());
    cycle.push_front(QString(" -> ") + cycle.takeFirst());
    QString msg = "Cyclic struct reference found:\n\n" + cycle.join("\n |\t|\n |\tV\n |  ") +
                  "\n\nUnable to set field of " + nodeName + " to " + target;
    QMessageBox::warning(this, "Struct - Cyclic Reference", msg);
  }
  else if (m_structReferences.contains(target))
  {
    if (m_structReferences[target].contains(nodeName))
      m_structReferences[target].removeAt(m_structReferences[target].indexOf(nodeName));
    if (m_structReferences[target].isEmpty())
      m_structReferences.remove(target);
    ok = true;
  }
}

void StructEditorWidget::onModifyStructPointerReference(QString nodeName, QString target,
                                                        bool addRef)
{
  if (addRef)
  {
    if (!m_structPointerReferences.contains(target))
      m_structPointerReferences.insert(target, {nodeName});
    else if (!m_structPointerReferences[target].contains(nodeName))
      m_structPointerReferences[target].push_back(nodeName);
  }
  else if (m_structPointerReferences.contains(target))
  {
    if (m_structPointerReferences[target].contains(nodeName))
      m_structPointerReferences[target].removeAt(
          m_structPointerReferences[target].indexOf(nodeName));
    if (m_structPointerReferences[target].isEmpty())
      m_structPointerReferences.remove(target);
  }
}

void StructEditorWidget::setupStructReferences()
{
  m_structPointerReferences.clear();
  m_structReferences.clear();
  QVector<StructTreeNode*> queue{m_structRootNode};
  QMap<QString, StructDef*> knownStructs = getStructMap();

  while (!queue.isEmpty())
  {
    StructTreeNode* curNode = queue.takeFirst();
    if (!curNode->getChildren().isEmpty())
      queue.append(curNode->getChildren());

    if (curNode->getStructDef() != nullptr && curNode->getStructDef()->getFields().count() > 0)
    {
      QString nameSpace = curNode->getNameSpace();

      for (FieldDef* field : curNode->getStructDef()->getFields())
      {
        if (field->isPadding())
          continue;

        if (field->getEntry()->getType() == Common::MemType::type_struct)
        {
          if (!knownStructs.keys().contains(field->getEntry()->getStructName()))
          {
            // This is an error, should be reported to the user, maybe they can select another
            // struct to use in this field?
            printf("error");
            continue;
          }

          if (field->getEntry()->isBoundToPointer())
            onModifyStructPointerReference(nameSpace, field->getEntry()->getStructName(), true);

          else
          {
            bool ok;
            onModifyStructReference(nameSpace, field->getEntry()->getStructName(), true, ok);
            if (!ok)
            {
              // This is an error, should be reported to the user. field should be cleared to
              // padding in this case.
            }
          }
        }
      }
    }
  }
}

void StructEditorWidget::updateStructReferenceNames(QString old_name, QString new_name)
{
  if (m_structReferences.contains(old_name))
  {
    for (QString target : m_structReferences[old_name])
    {
      m_structRootNode->findNode(target)->getStructDef()->updateStructTypeLabel(old_name, new_name);
      if (m_nodeInDetailEditor != nullptr)
        m_structDetailModel->updateStructTypeLabel(old_name, new_name);
    }
    QStringList refs = m_structReferences.take(old_name);
    m_structReferences.insert(new_name, refs);
    for (QString key : m_structReferences.keys())
      if (m_structReferences[key].contains(old_name))
        m_structReferences[key].replace(m_structReferences[key].indexOf(old_name), new_name);
  }

  if (m_structPointerReferences.contains(old_name))
  {
    for (QString target : m_structPointerReferences[old_name])
    {
      if (target != old_name)
        m_structRootNode->findNode(target)->getStructDef()->updateStructTypeLabel(old_name,
                                                                                  new_name);
      if (m_nodeInDetailEditor != nullptr)
        m_structDetailModel->updateStructTypeLabel(old_name, new_name);
    }
    QStringList refs = m_structPointerReferences.take(old_name);
    m_structPointerReferences.insert(new_name, refs);
    for (QString key : m_structPointerReferences.keys())
      if (m_structPointerReferences[key].contains(old_name))
        m_structPointerReferences[key].replace(m_structPointerReferences[key].indexOf(old_name),
                                               new_name);
  }
}

void StructEditorWidget::updateStructReferenceFieldSize(StructTreeNode* node)
{
  u32 structLength = node->getStructDef()->getLength();
  QString keyNameSpace = node->getNameSpace();

  if (!m_structReferences.contains(keyNameSpace))
    return;

  for (QString target : m_structReferences[keyNameSpace])
  {
    m_structRootNode->findNode(target)->getStructDef()->updateStructFieldSize(keyNameSpace,
                                                                              structLength);
    emit updateStructDetails(target);
  }
}

QStringList StructEditorWidget::checkForMapCycles(QMap<QString, QStringList> map, QString curName,
                                                  QString origName)
{
  QStringList cycle;

  if (origName == nullptr)
  {
    for (QString name : map.keys())
    {
      cycle = checkForMapCycles(map, name, name);
      if (!cycle.isEmpty())
        return cycle;
    }
    return cycle;
  }

  if (!map.contains(curName))
    return cycle;

  for (QString name : map[curName])
  {
    if (name == origName)
      return {name};

    cycle = checkForMapCycles(map, name, origName);
    if (!cycle.isEmpty())
    {
      cycle.push_front(name);
      return cycle;
    }
  }
  return cycle;
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
      connect(editStruct, &QAction::triggered, this,
              [this, node] { StructEditorWidget::onEditStruct(node); });
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

void StructEditorWidget::onSelectDataEdited(const QModelIndex& index, const QVariant& oldNamespace,
                                            int role)
{
  if (role != Qt::EditRole)
  {
    return;
  }

  if (index.column() == StructSelectModel::WATCH_COL_LABEL)
  {
    StructTreeNode* node = static_cast<StructTreeNode*>(index.internalPointer());
    QString oldFullName = oldNamespace.toString();

    if (!node->isGroup())
    {
      node->getStructDef()->updateStructTypeLabel(oldFullName, node->getNameSpace());
      updateStructReferenceNames(oldFullName, node->getNameSpace());
      emit updateStructName(oldFullName, node->getNameSpace());
      if (m_nodeInDetailEditor != nullptr)
      {
        if (oldFullName == m_structDetailModel->getLoadedStructNode()->getNameSpace())
          m_structDetailModel->getLoadedStructNode()->setName(node->getName());
        m_structDetailModel->updateStructTypeLabel(oldFullName, node->getNameSpace());
      }
    }
    else
      updateChildStructNames(node, oldFullName);
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

      QString oldName =
          oldNameSpace.isEmpty() ? child->getName() : child->appendNameToNameSpace(oldNameSpace);
      QString newName =
          newNameSpace.isEmpty() ? child->getName() : child->appendNameToNameSpace(newNameSpace);

      updateStructReferenceNames(oldName, newName);
      emit updateStructName(oldName, newName);
    }
  }
}

void StructEditorWidget::onSelectDropSucceeded(StructTreeNode* oldParent, StructTreeNode* newParent)
{
  updateChildStructNames(newParent, oldParent->getNameSpace());
}

void StructEditorWidget::onDetailContextMenuRequested(const QPoint& pos)
{
  if (m_nodeInDetailEditor == nullptr)
    return;

  QModelIndex index = m_structDetailView->indexAt(pos);
  QMenu* contextMenu = new QMenu(this);

  QAction* const addField{new QAction(tr("Add field"), this)};
  connect(addField, &QAction::triggered, this, &StructEditorWidget::onAddField);
  contextMenu->addAction(addField);
  if (index != QModelIndex())
  {
    FieldDef* node = m_structDetailModel->getFieldByRow(index.row());
    if (node != nullptr)
    {
      if (node->isPadding())
      {
        QAction* const createEntry{new QAction(tr("Create field entry"), this)};
        connect(createEntry, &QAction::triggered, this,
                [this, index] { createNewFieldEntry(index); });
        contextMenu->addAction(createEntry);
        contextMenu->addSeparator();
      }
      else
      {
        QAction* const dupField{new QAction(tr("Duplicate field"), this)};
        connect(dupField, &QAction::triggered, this, [this, index] { onDuplicateField(index); });
        contextMenu->addAction(dupField);

        QAction* const editField{new QAction(tr("Edit field entry"), this)};
        connect(editField, &QAction::triggered, this, [this, index] { editFieldEntry(index); });
        contextMenu->addAction(editField);

        contextMenu->addSeparator();

        QAction* const clearField{new QAction(tr("Clear field"), this)};
        connect(clearField, &QAction::triggered, this, &StructEditorWidget::onClearFields);
        contextMenu->addAction(clearField);
      }

      QAction* const deleteField{new QAction(tr("Delete field(s)"), this)};
      connect(deleteField, &QAction::triggered, this, &StructEditorWidget::onDeleteFields);
      contextMenu->addAction(deleteField);
    }
  }

  contextMenu->popup(m_structDetailView->viewport()->mapToGlobal(pos));
}

void StructEditorWidget::onDetailDoubleClicked(const QModelIndex& index)
{
  FieldDef* field = m_structDetailModel->getFieldByRow(index.row());

  if (field->isPadding())
    createNewFieldEntry(index);
  else if (index.column() == StructDetailModel::STRUCT_COL_LABEL)
    m_structDetailView->edit(index);
  else
    editFieldEntry(index);
  m_btnSaveStructDetails->setEnabled(true);
}

void StructEditorWidget::onDetailDataEdited(const QModelIndex& index, const QVariant& value,
                                            int role)
{
  (void)value;
  if (role != Qt::EditRole)
    return;

  if (index.column() == StructDetailModel::STRUCT_COL_LABEL)
  {
    m_btnSaveStructDetails->setEnabled(true);
  }
}

void StructEditorWidget::onAddGroup()
{
  const QModelIndexList selectedIndexes{m_structSelectView->selectionModel()->selectedIndexes()};
  const QModelIndex lastIndex = selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back();
  const StructTreeNode* parentNode = selectedIndexes.empty() ?
                                         m_structRootNode :
                                         static_cast<StructTreeNode*>(lastIndex.internalPointer());

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

  // May want to enclose the selected indices in a new node... In this case, we would need to emit
  // name changes for any struct that was moved...
  m_structSelectModel->addGroup(textIn, lastIndex);
  m_unsavedChanges = true;
}

void StructEditorWidget::onAddStruct()
{
  const QModelIndexList selectedIndexes{m_structSelectView->selectionModel()->selectedIndexes()};
  const QModelIndex lastIndex = selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back();
  const StructTreeNode* parentNode = selectedIndexes.empty() ?
                                         m_structRootNode :
                                         static_cast<StructTreeNode*>(lastIndex.internalPointer());

  bool ok = false;
  QString text;
  while (true)
  {
    text = QInputDialog::getText(this, "Add a struct", "Enter the struct name:", QLineEdit::Normal,
                                 "", &ok);

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

  onEditStruct(addedNode);
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
        {
          emit structAddedRemoved(curNode->getNameSpace());
          if (curNode == m_nodeInDetailEditor)
            unloadStruct();
        }
      }
    }
    else
    {
      emit structAddedRemoved(curNode->getNameSpace());
      if (curNode == m_nodeInDetailEditor)
        unloadStruct();
    }

    m_structSelectModel->deleteNode(index);
  }
}

void StructEditorWidget::onEditStruct(StructTreeNode* node)
{
  if (node->isGroup())
    return;
  if (m_structDetailModel->hasStructLoaded() && unsavedStructDetails())
  {
    QMessageBox::StandardButton response = QMessageBox::question(
        this, "Save Changes?",
        "You have unsaved changes to this struct.\nWould you like to save them?");
    if (response == QMessageBox::StandardButton::Yes)
    {
      onSaveStruct();
    }
  }
  m_nodeInDetailEditor = node;
  StructTreeNode* nodeForDetailEditor = new StructTreeNode(node);
  m_structDetailModel->loadStruct(nodeForDetailEditor);

  m_btnUnloadStructDetails->setEnabled(true);
  m_btnAddField->setEnabled(true);
  m_btnDeleteFields->setEnabled(true);
  m_btnClearFields->setEnabled(true);
  m_structDetailView->setEnabled(true);

  m_txtStructName->blockSignals(true);
  m_txtStructName->setEnabled(true);
  m_txtStructName->setText(nodeForDetailEditor->getName());
  m_txtStructName->blockSignals(false);

  m_txtStructLength->blockSignals(true);
  m_txtStructLength->setEnabled(true);
  m_txtStructLength->setText(
      QString::number(nodeForDetailEditor->getStructDef()->getLength(), 16).toUpper());
  m_txtStructLength->blockSignals(false);
}

void StructEditorWidget::onUnloadStruct()
{
  if (m_structDetailModel->hasStructLoaded() && unsavedStructDetails())
  {
    QMessageBox::StandardButton response = QMessageBox::question(
        this, "Save Changes?",
        "You have unsaved changes to the current struct.\nWould you like to save them?");
    if (response == QMessageBox::StandardButton::Yes)
    {
      onSaveStruct();
    }
  }

  unloadStruct();
}

bool StructEditorWidget::unsavedStructDetails()
{
  return m_btnSaveStructDetails->isEnabled();
}

void StructEditorWidget::unloadStruct()
{
  m_structDetailModel->unloadStruct();
  m_nodeInDetailEditor = nullptr;
  m_btnUnloadStructDetails->setDisabled(true);
  m_btnSaveStructDetails->setDisabled(true);
  m_btnAddField->setDisabled(true);
  m_btnDeleteFields->setDisabled(true);
  m_btnClearFields->setDisabled(true);
  m_structDetailView->setDisabled(true);

  m_txtStructName->blockSignals(true);
  m_txtStructName->setText({});
  m_txtStructName->setDisabled(true);
  m_txtStructName->blockSignals(false);

  m_txtStructLength->blockSignals(true);
  m_txtStructLength->setText({});
  m_txtStructLength->setDisabled(true);
  m_txtStructLength->blockSignals(false);
}

void StructEditorWidget::readStructDefMapFromJson(const QJsonObject& json,
                                                  QMap<QString, QString>& map)
{
  QMap<QString, StructDef*> newStructDefs{};
  QMap<QString, StructDef*> replacementStructDefs{};

  QJsonArray newStructArray = json["structDefs"].toArray();

  for (auto obj : newStructArray)
  {
    QJsonObject structDefObj = obj.toObject();
    QString structName = structDefObj["name"].toString();
    StructDef* def = new StructDef();
    def->readFromJson(structDefObj["def"].toObject());

    StructTreeNode* equivalentNode = m_structRootNode->findNode(structName);
    if (equivalentNode != nullptr)
    {
      if (equivalentNode->isGroup())
      {
        // ask user if they want to rename incoming struct or the group

        StructTreeNode* nodeParent = equivalentNode->getParent();
        int i = 0;
        QString oldPartialName = structName.split("::").last();
        QString newPartialName = oldPartialName + QString("(%1)").arg(i);
        while (!nodeParent->isNameAvailable(newPartialName))
        {
          i++;
          newPartialName = oldPartialName + QString("(%1)").arg(i);
        }
        QString newName = (nodeParent->getNameSpace().isEmpty() ? "" : "::") + newPartialName;

        QString msg = QString("There is already a group with the same name as the struct on file: "
                              "%1.\nHow would you like to resolve this?")
                          .arg(structName);
        QMessageBox msgBox =
            QMessageBox(QMessageBox::Icon::Question, "Group and struct with same name detected!",
                        msg, QMessageBox::StandardButton::NoButton, this);
        QPushButton* changeGroup = msgBox.addButton(tr("Rename Group"), QMessageBox::AcceptRole);
        QPushButton* changeStruct = msgBox.addButton(tr("Rename Struct"), QMessageBox::AcceptRole);
        msgBox.setDefaultButton(changeStruct);
        msgBox.exec();

        if (msgBox.clickedButton() == changeStruct)
        {
          map.insert(structName, newName);
          QString msg = QString("Struct %1 renamed to %2").arg(structName).arg(newName);
          QMessageBox::warning(this, "Struct Renamed", msg);
          structName = newName;
          def->setLabel(newName);
        }
        else if (msgBox.clickedButton() == changeGroup)
        {
          m_structSelectModel->setData(m_structSelectModel->getIndexFromTreeNode(equivalentNode),
                                       newPartialName, Qt::EditRole);
          QString msg = QString("Group %1 renamed to %2").arg(structName).arg(newName);
          QMessageBox::warning(this, "Group Renamed", msg);
        }
      }
      else if (equivalentNode->getStructDef()->isSame(
                   def))  // if structs are the same, no need to alert user and just use the one we
                          // have
        continue;
      else
      {
        // ask user if they want to just use the current struct, overwrite the current struct, or
        // create a new struct
        QString msg = QString("There is already a different struct named %1").arg(structName);
        QMessageBox msgBox = QMessageBox(QMessageBox::Icon::Question, "Duplicate struct detected!",
                                         msg, QMessageBox::StandardButton::NoButton, this);
        msgBox.setDetailedText(equivalentNode->getStructDef()->getDiffString(def));
        QPushButton* useOld = msgBox.addButton(tr("Use Current Struct"), QMessageBox::AcceptRole);
        QPushButton* useNew = msgBox.addButton(tr("Use File Struct"), QMessageBox::AcceptRole);
        QPushButton* newName = msgBox.addButton(tr("Rename File Struct"), QMessageBox::AcceptRole);
        msgBox.setDefaultButton(newName);
        msgBox.exec();
        if (msgBox.clickedButton() == useOld)
          continue;
        else if (msgBox.clickedButton() == useNew)
        {
          replacementStructDefs.insert(structName, def);
        }
        else if (msgBox.clickedButton() == newName)
        {
          StructTreeNode* nodeParent = equivalentNode->getParent();
          int i = 0;
          QString oldPartialName = structName.split("::").last();
          QString newPartialName = oldPartialName + QString("(%1)").arg(i);
          while (!nodeParent->isNameAvailable(newPartialName))
          {
            i++;
            newPartialName = oldPartialName + QString("(%1)").arg(i);
          }
          QString newName = (nodeParent->getNameSpace().isEmpty() ? "" : "::") + newPartialName;
          map.insert(structName, newName);
          QString msg = QString("%1 from file renamed to %2").arg(structName).arg(newName);
          QMessageBox::warning(this, "Struct Renamed", msg);
          structName = newName;
          def->setLabel(newName);
        }
      }
    }
    newStructDefs.insert(structName, def);
  }

  for (QString key : replacementStructDefs.keys())
  {
    m_structSelectModel->replaceDef(key, replacementStructDefs[key]);
    emit structAddedRemoved(key, replacementStructDefs[key]);
  }

  for (QString key : newStructDefs.keys())
  {
    m_structSelectModel->insertNewDef(key, newStructDefs[key]);
    emit structAddedRemoved(key, newStructDefs[key]);
  }
  emit m_structSelectModel->layoutChanged();
}

void StructEditorWidget::writeStructDefMapToJson(QJsonObject& json,
                                                 const QStringList desiredStructs) const
{
  QStringList structsWritten{};
  QJsonArray structs;

  QStringList queue{desiredStructs};
  while (!queue.isEmpty())
  {
    QString nextStruct = queue.takeFirst();
    if (structsWritten.contains(nextStruct))
      continue;

    StructTreeNode* node = m_structRootNode->findNode(nextStruct);
    if (node == nullptr)
      continue;

    QJsonObject structDefObject{};
    QJsonObject structDef;
    node->getStructDef()->writeToJson(structDef);

    structDefObject["name"] = nextStruct;
    structDefObject["def"] = structDef;
    structs.append(structDefObject);

    structsWritten.push_back(nextStruct);
    for (QString key : m_structReferences.keys())
      if (m_structReferences[key].contains(nextStruct))
        queue.append(key);
    for (QString key : m_structPointerReferences.keys())
      if (m_structPointerReferences[key].contains(nextStruct))
        queue.append(key);
  }

  json["structDefs"] = structs;

  QJsonDocument saveDoc(json);
  auto doc = saveDoc.toJson();
}

void StructEditorWidget::writeStructDefsToJson(QJsonObject& json) const
{
  m_structRootNode->writeToJson(json);
}

void StructEditorWidget::restoreStructTree(const QString& json)
{
  const QJsonDocument loadDoc(QJsonDocument::fromJson(json.toUtf8()));
  m_structRootNode->readFromJson(loadDoc.object());  // may need to reset model here?

  setupStructReferences();
}

QString StructEditorWidget::saveStructTree()
{
  if (m_structDetailModel->hasStructLoaded() && unsavedStructDetails())
  {
    QMessageBox::StandardButton response = QMessageBox::question(
        this, "Save Changes?",
        "You have unsaved changes to the current struct.\nWould you like to save them?");
    if (response == QMessageBox::StandardButton::Yes)
    {
      onSaveStruct();
    }
  }

  QJsonObject root;
  m_structRootNode->writeToJson(root);
  QJsonDocument saveDoc(root);
  return saveDoc.toJson();
}

QMap<QString, StructDef*> StructEditorWidget::getStructMap()
{
  return m_structSelectModel->getStructMap();
}

StructTreeNode* StructEditorWidget::getStructDefs()
{
  return m_structRootNode;
}
