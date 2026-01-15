#include "MemWatchWidget.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIODevice>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QSignalMapper>
#include <QString>
#include <QTextStream>
#include <QVBoxLayout>
#include <string>
#include <unordered_map>

#include "../../Common/MemoryCommon.h"
#include "../../MemoryWatch/MemWatchEntry.h"
#include "../GUICommon.h"
#include "Dialogs/DlgAddWatchEntry.h"
#include "Dialogs/DlgChangeType.h"
#include "Dialogs/DlgImportCTFile.h"

MemWatchWidget::MemWatchWidget(QWidget* parent) : QWidget(parent)
{
  initialiseWidgets();
  makeLayouts();
}

MemWatchWidget::~MemWatchWidget()
{
  delete m_watchDelegate;
  delete m_watchModel;
}

void MemWatchWidget::initialiseWidgets()
{
  m_watchModel = new MemWatchModel(this);
  connect(m_watchModel, &MemWatchModel::dataEdited, this, &MemWatchWidget::onDataEdited);
  connect(m_watchModel, &MemWatchModel::writeFailed, this, &MemWatchWidget::onValueWriteError);
  connect(m_watchModel, &MemWatchModel::dropSucceeded, this, &MemWatchWidget::onDropSucceeded);
  connect(m_watchModel, &MemWatchModel::readFailed, this, &MemWatchWidget::mustUnhook);
  connect(m_watchModel, &MemWatchModel::rowsInserted, this, &MemWatchWidget::onRowsInserted);

  m_watchDelegate = new MemWatchDelegate();

  m_watchView = new QTreeView;
  m_watchView->setDragEnabled(true);
  m_watchView->setAcceptDrops(true);
  m_watchView->setDragDropMode(QAbstractItemView::InternalMove);
  m_watchView->setDropIndicatorShown(true);
  m_watchView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_watchView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_watchView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_watchView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_watchView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_watchView, &QWidget::customContextMenuRequested, this,
          &MemWatchWidget::onMemWatchContextMenuRequested);
  connect(m_watchView, &QAbstractItemView::doubleClicked, this,
          &MemWatchWidget::onWatchDoubleClicked);
  connect(m_watchView, &QTreeView::collapsed, this, &MemWatchWidget::onCollapsed);
  connect(m_watchView, &QTreeView::expanded, this, &MemWatchWidget::onExpanded);
  m_watchView->setItemDelegate(m_watchDelegate);
  m_watchView->setModel(m_watchModel);

  const int charWidth{m_watchView->fontMetrics().averageCharWidth()};
  m_watchView->header()->setMinimumSectionSize(charWidth);
  m_watchView->header()->resizeSection(MemWatchModel::WATCH_COL_LABEL, charWidth * 35);

  m_watchView->header()->setSectionResizeMode(MemWatchModel::WATCH_COL_TYPE,
                                              QHeaderView::ResizeToContents);
  m_watchView->header()->setSectionResizeMode(MemWatchModel::WATCH_COL_ADDRESS,
                                              QHeaderView::ResizeToContents);
  m_watchView->header()->setSectionResizeMode(MemWatchModel::WATCH_COL_LOCK,
                                              QHeaderView::ResizeToContents);

  QShortcut* deleteWatchShortcut = new QShortcut(QKeySequence::Delete, m_watchView);
  connect(deleteWatchShortcut, &QShortcut::activated, this, &MemWatchWidget::onDeleteSelection);

  QShortcut* copyWatchShortcut =
      new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_C), m_watchView);
  connect(copyWatchShortcut, &QShortcut::activated, this,
          &MemWatchWidget::copySelectedWatchesToClipBoard);

  QShortcut* cutWatchShortcut =
      new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_X), m_watchView);
  connect(cutWatchShortcut, &QShortcut::activated, this,
          &MemWatchWidget::cutSelectedWatchesToClipBoard);

  QShortcut* pasteWatchShortcut =
      new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_V), m_watchView);
  connect(pasteWatchShortcut, &QShortcut::activated, this, [this] {
    const QModelIndexList selectedIndexes{m_watchView->selectionModel()->selectedIndexes()};
    pasteWatchFromClipBoard(selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back());
  });

  m_btnAddGroup = new QPushButton(tr("Add group"), this);
  connect(m_btnAddGroup, &QPushButton::clicked, this, &MemWatchWidget::onAddGroup);

  m_btnAddWatchEntry = new QPushButton(tr("Add watch"), this);
  connect(m_btnAddWatchEntry, &QPushButton::clicked, this, &MemWatchWidget::onAddWatchEntry);

  m_updateTimer = new QTimer(this);
  connect(m_updateTimer, &QTimer::timeout, m_watchModel, &MemWatchModel::onUpdateTimer);

  m_freezeTimer = new QTimer(this);
  connect(m_freezeTimer, &QTimer::timeout, m_watchModel, &MemWatchModel::onFreezeTimer);
}

void MemWatchWidget::makeLayouts()
{
  QWidget* buttons = new QWidget;
  QHBoxLayout* buttons_layout = new QHBoxLayout;

  buttons_layout->addWidget(m_btnAddGroup);
  buttons_layout->addWidget(m_btnAddWatchEntry);
  buttons_layout->setContentsMargins(0, 0, 0, 0);
  buttons->setLayout(buttons_layout);

  QVBoxLayout* layout = new QVBoxLayout;

  layout->addWidget(buttons);
  layout->addWidget(m_watchView);
  layout->setContentsMargins(3, 0, 3, 0);
  setLayout(layout);
}

void MemWatchWidget::onMemWatchContextMenuRequested(const QPoint& pos)
{
  QModelIndex index = m_watchView->indexAt(pos);
  QMenu* contextMenu = new QMenu(this);
  MemWatchTreeNode* node = nullptr;
  if (index != QModelIndex())
  {
    node = MemWatchModel::getTreeNodeFromIndex(index);
    if (!node->isGroup())
    {
      MemWatchEntry* const entry{MemWatchModel::getEntryFromIndex(index)};
      int typeIndex = static_cast<int>(entry->getType());
      Common::MemType theType = static_cast<Common::MemType>(typeIndex);

      if (entry->isBoundToPointer())
      {
        QMenu* memViewerSubMenu = contextMenu->addMenu(tr("Browse &memory at"));
        QAction* showPointerInViewer = new QAction(tr("The &pointer address..."), this);
        connect(showPointerInViewer, &QAction::triggered, this,
                [this, entry] { emit goToAddressInViewer(entry->getConsoleAddress()); });
        memViewerSubMenu->addAction(showPointerInViewer);
        for (int i{0}; i < static_cast<int>(entry->getPointerLevel()); ++i)
        {
          std::string strAddressOfPath = entry->getAddressStringForPointerLevel(i + 1);
          if (strAddressOfPath == "???")
            break;
          QAction* showAddressOfPathInViewer = new QAction(
              tr("The pointed address at &level %1...").arg(QString::number(i + 1)), this);
          connect(showAddressOfPathInViewer, &QAction::triggered, this, [this, entry, i] {
            emit goToAddressInViewer(entry->getAddressForPointerLevel(i + 1));
          });
          memViewerSubMenu->addAction(showAddressOfPathInViewer);
        }

        QAction* showInViewer = new QAction(tr("Browse memory at this &address..."), this);
        connect(showInViewer, &QAction::triggered, this,
                [this, entry] { emit goToAddressInViewer(entry->getConsoleAddress()); });
      }
      else
      {
        QAction* showInViewer = new QAction(tr("Browse memory at this &address..."), this);
        connect(showInViewer, &QAction::triggered, this,
                [this, entry] { emit goToAddressInViewer(entry->getConsoleAddress()); });

        contextMenu->addAction(showInViewer);
      }
      if (theType != Common::MemType::type_string && theType != Common::MemType::type_ppc)
      {
        contextMenu->addSeparator();

        QAction* viewDec = new QAction(tr("View as &Decimal"), this);
        QAction* viewHex = new QAction(tr("View as &Hexadecimal"), this);
        QAction* viewOct = new QAction(tr("View as &Octal"), this);
        QAction* viewBin = new QAction(tr("View as &Binary"), this);

        connect(viewDec, &QAction::triggered, m_watchModel,
                [this, entry] { setSelectedWatchesBase(entry, Common::MemBase::base_decimal); });
        connect(viewHex, &QAction::triggered, m_watchModel, [this, entry] {
          setSelectedWatchesBase(entry, Common::MemBase::base_hexadecimal);
        });
        connect(viewOct, &QAction::triggered, m_watchModel,
                [this, entry] { setSelectedWatchesBase(entry, Common::MemBase::base_octal); });
        connect(viewBin, &QAction::triggered, m_watchModel,
                [this, entry] { setSelectedWatchesBase(entry, Common::MemBase::base_binary); });

        contextMenu->addAction(viewDec);
        contextMenu->addAction(viewHex);
        contextMenu->addAction(viewOct);
        contextMenu->addAction(viewBin);
        contextMenu->addSeparator();

        int baseIndex = static_cast<int>(entry->getBase());
        Common::MemBase theBase = static_cast<Common::MemBase>(baseIndex);
        contextMenu->actions().at(baseIndex + 2)->setEnabled(false);

        if (theBase == Common::MemBase::base_decimal)
        {
          QAction* viewSigned = new QAction(tr("View as &Signed"), this);
          QAction* viewUnsigned = new QAction(tr("View as &Unsigned"), this);

          connect(viewSigned, &QAction::triggered, m_watchModel, [this, entry] {
            entry->setSignedUnsigned(false);
            m_hasUnsavedChanges = true;
          });
          connect(viewUnsigned, &QAction::triggered, m_watchModel, [this, entry] {
            entry->setSignedUnsigned(true);
            m_hasUnsavedChanges = true;
          });

          contextMenu->addAction(viewSigned);
          contextMenu->addAction(viewUnsigned);

          if (entry->isUnsigned())
          {
            contextMenu->actions()
                .at(static_cast<int>(Common::MemBase::base_none) + 4)
                ->setEnabled(false);
          }
          else
          {
            contextMenu->actions()
                .at(static_cast<int>(Common::MemBase::base_none) + 3)
                ->setEnabled(false);
          }
        }
      }
      if (theType == Common::MemType::type_ppc)
      {
        contextMenu->addSeparator();

        QAction* b_absolute = new QAction(tr("Branch Type &Absolute"), this);
        QAction* b_relative = new QAction(tr("Branch Type &Relative"), this);

        connect(b_absolute, &QAction::triggered, m_watchModel, [this, entry] {
          entry->setBranchType(true);
          m_hasUnsavedChanges = true;
        });
        connect(b_relative, &QAction::triggered, m_watchModel, [this, entry] {
          entry->setBranchType(false);
          m_hasUnsavedChanges = true;
        });

        contextMenu->addAction(b_absolute);
        contextMenu->addAction(b_relative);

        contextMenu->actions().at((entry->isAbsoluteBranch() ? 0 : 1) + 2)->setEnabled(false);
      }
      contextMenu->addSeparator();
      QAction* lockSelection = new QAction(tr("Lock"), this);
      connect(lockSelection, &QAction::triggered, this, [this] { onLockSelection(true); });
      contextMenu->addAction(lockSelection);
      QAction* unlockSelection = new QAction(tr("Unlock"), this);
      connect(unlockSelection, &QAction::triggered, this, [this] { onLockSelection(false); });
      contextMenu->addAction(unlockSelection);
      contextMenu->addSeparator();

      if (!GUICommon::isContainerType(entry->getType()))
      {
        QAction* const editValue{new QAction(tr("Edit Value"), this)};
        connect(editValue, &QAction::triggered, this,
                [this, index]() { m_watchView->edit(index); });
        contextMenu->addAction(editValue);
        contextMenu->addSeparator();
      }
    }
  }
  else
  {
    node = m_watchModel->getRootNode();
  }

  const QModelIndexList simplifiedSelection{simplifySelection()};
  if (!simplifiedSelection.empty())
  {
    bool canGroupSelection = true;
    for (const QModelIndex& selectedIndex : simplifiedSelection)
    {
      const MemWatchTreeNode* selectedNode = m_watchModel->getTreeNodeFromIndex(selectedIndex);
      if (selectedNode != nullptr && selectedNode->getParent() != nullptr &&
          (!selectedNode->getParent()->isGroup() &&
           selectedNode->getParent() != m_watchModel->getRootNode() &&
           GUICommon::isContainerType(selectedNode->getParent()->getEntry()->getType())))
        canGroupSelection = false;
    }
    if (canGroupSelection)
    {
      QAction* const groupAction{new QAction(tr("&Group"), this)};
      connect(groupAction, &QAction::triggered, this, &MemWatchWidget::groupCurrentSelection);
      contextMenu->addAction(groupAction);
      contextMenu->addSeparator();
    }
  }

  if (node == m_watchModel->getRootNode() || node->isGroup())
  {
    QAction* const addGroup{new QAction(tr("Add gro&up"), this)};
    connect(addGroup, &QAction::triggered, this, &MemWatchWidget::onAddGroup);
    contextMenu->addAction(addGroup);
    QAction* const addWatch{new QAction(tr("Add &watch"), this)};
    connect(addWatch, &QAction::triggered, this, &MemWatchWidget::onAddWatchEntry);
    contextMenu->addAction(addWatch);
    contextMenu->addSeparator();
  }

  QAction* cut = new QAction(tr("Cu&t"), this);
  connect(cut, &QAction::triggered, this, [this] { cutSelectedWatchesToClipBoard(); });
  contextMenu->addAction(cut);
  QAction* copy = new QAction(tr("&Copy"), this);
  connect(copy, &QAction::triggered, this, [this] { copySelectedWatchesToClipBoard(); });
  contextMenu->addAction(copy);

  MemWatchEntry* const entry{MemWatchModel::getEntryFromIndex(index)};
  if (entry)
  {
    if (entry->isBoundToPointer())
    {
      QMenu* const copyAddrSubmenu = contextMenu->addMenu(tr("Copy add&ress..."));
      QAction* const copyPointer = new QAction(tr("Copy &base address..."), this);
      const QString addrString{QString::number(entry->getConsoleAddress(), 16).toUpper()};
      connect(copyPointer, &QAction::triggered, this,
              [addrString] { QApplication::clipboard()->setText(addrString); });
      copyAddrSubmenu->addAction(copyPointer);
      for (int i = 0; i < static_cast<int>(entry->getPointerLevel()); ++i)
      {
        if (!entry->getAddressForPointerLevel(i + 1))
          break;
        QAction* const copyAddrOfPointer =
            new QAction(tr("Copy pointed address at &level %1...").arg(i + 1), this);
        const QString addrString{
            QString::number(entry->getAddressForPointerLevel(i + 1), 16).toUpper()};
        connect(copyAddrOfPointer, &QAction::triggered, this,
                [addrString] { QApplication::clipboard()->setText(addrString); });
        copyAddrSubmenu->addAction(copyAddrOfPointer);
      }
    }
    else
    {
      QAction* const copyPointer = new QAction(tr("Copy add&ress"), this);
      const QString addrString{QString::number(entry->getConsoleAddress(), 16).toUpper()};
      connect(copyPointer, &QAction::triggered, this,
              [addrString] { QApplication::clipboard()->setText(addrString); });
      contextMenu->addAction(copyPointer);
    }
  }

  QAction* paste = new QAction(tr("&Paste"), this);
  connect(paste, &QAction::triggered, this, [this, index] { pasteWatchFromClipBoard(index); });
  contextMenu->addAction(paste);

  QAction* deleteSelection = new QAction(tr("&Delete"), this);
  if (node != m_watchModel->getRootNode() &&
      !(node->getParent() != nullptr && node->getParent()->getEntry() &&
        GUICommon::isContainerType(node->getParent()->getEntry()->getType())))
  {
    contextMenu->addSeparator();
    connect(deleteSelection, &QAction::triggered, this, [this] { onDeleteSelection(); });
    contextMenu->addAction(deleteSelection);
  }

  QModelIndexList selection = m_watchView->selectionModel()->selectedRows();
  if (selection.count() == 0)
  {
    copy->setEnabled(false);
    cut->setEnabled(false);
    deleteSelection->setEnabled(false);
  }

  contextMenu->popup(m_watchView->viewport()->mapToGlobal(pos));
}

void MemWatchWidget::setSelectedWatchesBase(MemWatchEntry* entry, Common::MemBase base)
{
  QModelIndexList selectedItems = m_watchView->selectionModel()->selectedRows();
  if (selectedItems.count() == 1)
  {
    entry->setBase(base);
  }
  else
  {
    for (int x = 0; x < selectedItems.count(); x++)
    {
      MemWatchEntry* const selectedEntry{MemWatchModel::getEntryFromIndex(selectedItems.at(x))};
      if (selectedEntry != nullptr && selectedEntry->getType() != Common::MemType::type_string)
        selectedEntry->setBase(base);
    }
  }
  m_hasUnsavedChanges = true;
}

void MemWatchWidget::groupCurrentSelection()
{
  const QModelIndexList indexes{simplifySelection()};
  if (indexes.isEmpty())
    return;

  m_watchModel->groupSelection(indexes);

  m_hasUnsavedChanges = true;
}

void MemWatchWidget::cutSelectedWatchesToClipBoard()
{
  copySelectedWatchesToClipBoard();

  const QModelIndexList cutList{simplifySelection()};
  if (!cutList.empty())
  {
    for (const auto& index : cutList)
    {
      m_watchModel->deleteNode(index);
    }

    m_hasUnsavedChanges = true;
  }
}

void MemWatchWidget::copySelectedWatchesToClipBoard()
{
  QModelIndexList selection = m_watchView->selectionModel()->selectedRows();
  if (selection.count() == 0)
    return;

  QJsonObject jsonNode;
  {
    const QModelIndexList toCopyList{simplifySelection()};

    std::unordered_map<MemWatchTreeNode*, MemWatchTreeNode*> parentMap;
    MemWatchTreeNode rootNodeCopy(nullptr, nullptr, false, QString{});
    for (const auto& index : toCopyList)
    {
      MemWatchTreeNode* const childNode{MemWatchModel::getTreeNodeFromIndex(index)};
      parentMap[childNode] = childNode->getParent();

      rootNodeCopy.appendChild(childNode);  // Borrow node temporarily.
    }
    rootNodeCopy.writeToJson(jsonNode, true);

    // Remove borrowed children before going out of scope.
    rootNodeCopy.removeChildren();
    for (auto& [childNode, parentNode] : parentMap)
    {
      childNode->setParent(parentNode);
    }
  }

  QJsonDocument doc(jsonNode);
  QString nodeJsonStr(doc.toJson());

  QClipboard* clipboard = QApplication::clipboard();
  clipboard->setText(nodeJsonStr);
}

void MemWatchWidget::pasteWatchFromClipBoard(const QModelIndex& referenceIndex)
{
  MemWatchTreeNode copiedRootNode(nullptr);
  {
    const QString nodeStr{QApplication::clipboard()->text()};
    const QJsonDocument loadDoc{QJsonDocument::fromJson(nodeStr.toUtf8())};
    copiedRootNode.readFromJson(loadDoc.object(), QMap<QString, QString>());
  }

  const QVector<MemWatchTreeNode*> children{copiedRootNode.getChildren()};
  copiedRootNode.removeChildren();

  std::vector<MemWatchTreeNode*> childrenVec(children.constBegin(), children.constEnd());
  m_watchModel->addNodes(childrenVec, referenceIndex);

  m_hasUnsavedChanges = true;
}

void MemWatchWidget::onWatchDoubleClicked(const QModelIndex& index)
{
  if (index != QVariant())
  {
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());

    if (node->isGroup() || (node->getParent() && node->getParent()->getEntry() &&
                            GUICommon::isContainerType(node->getParent()->getEntry()->getType())))
      return;

    if (index.column() == MemWatchModel::WATCH_COL_TYPE)
    {
      MemWatchEntry* entry = node->getEntry();
      int typeIndex = static_cast<int>(entry->getType());
      DlgChangeType* dlg = new DlgChangeType(
          this, typeIndex, entry->getLength(), m_structDefs->getStructNames(),
          entry->getStructName(), entry->getContainerCount(), entry->getContainerEntry());
      if (dlg->exec() == QDialog::Accepted)
      {
        Common::MemType theType = static_cast<Common::MemType>(dlg->getTypeIndex());
        if (theType == Common::MemType::type_struct && dlg->getStructName() != QString())
          entry->setStructName(dlg->getStructName());
        if (theType == Common::MemType::type_array)
        {
          entry->setContainerCount(dlg->getContainerCount());
          entry->setContainerEntry(dlg->getContainerEntry());
        }

        m_watchModel->changeType(index, theType, dlg->getLength());
        m_hasUnsavedChanges = true;
      }
    }
    else if (index.column() == MemWatchModel::WATCH_COL_ADDRESS)
    {
      MemWatchEntry* entryCopy = new MemWatchEntry(node->getEntry());
      DlgAddWatchEntry dlg(false, entryCopy, m_structDefs->getStructNames(), this);
      if (dlg.exec() == QDialog::Accepted)
      {
        m_watchModel->editEntry(dlg.stealEntry(), index);
        m_hasUnsavedChanges = true;
      }
    }

    if (index.column() == MemWatchModel::WATCH_COL_VALUE &&
        node->getEntry()->getType() == Common::MemType::type_array)
    {
      bool ok{};
      size_t newCount = QInputDialog::getInt(
          this, tr("Set Array Count"), tr("Array Count:"),
          static_cast<int>(node->getEntry()->getContainerCount()), 1, 9999, 1, &ok, Qt::Dialog);
      if (ok)
        m_watchModel->setContainerCount(node, newCount);
    }
  }
}

void MemWatchWidget::onDataEdited(const QModelIndex& index, const QVariant& value, const int role)
{
  if (role != Qt::EditRole)
    return;

  const int column{index.column()};
  if (column == MemWatchModel::WATCH_COL_LABEL || column == MemWatchModel::WATCH_COL_TYPE ||
      column == MemWatchModel::WATCH_COL_ADDRESS)
  {
    m_hasUnsavedChanges = true;
  }
  else if (column == MemWatchModel::WATCH_COL_VALUE)
  {
    MemWatchTreeNode* const node{static_cast<MemWatchTreeNode*>(index.internalPointer())};
    MemWatchEntry* const entry{node->getEntry()};
    const Common::MemType entryType{entry->getType()};

    for (const auto& selectedIndex : m_watchView->selectionModel()->selectedIndexes())
    {
      if (selectedIndex == index)
        continue;
      if (selectedIndex.column() != MemWatchModel::WATCH_COL_VALUE)
        continue;

      MemWatchTreeNode* const selectedNode{
          static_cast<MemWatchTreeNode*>(selectedIndex.internalPointer())};
      if (selectedNode->isGroup())
        continue;
      MemWatchEntry* const selectedEntry{selectedNode->getEntry()};
      if (selectedEntry->getType() != entryType)
        continue;

      m_watchModel->editData(selectedIndex, value, role);
    }
  }
}

void MemWatchWidget::onValueWriteError(const QModelIndex& index,
                                       Common::MemOperationReturnCode writeReturn)
{
  switch (writeReturn)
  {
  case Common::MemOperationReturnCode::invalidInput:
  {
    MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
    MemWatchEntry* entry = node->getEntry();
    int typeIndex = static_cast<int>(entry->getType());
    int baseIndex = static_cast<int>(entry->getBase());

    QString errorMsg;
    if (entry->getType() == Common::MemType::type_byteArray)
      errorMsg = tr("The value you entered for the type %1 is invalid, you must enter the bytes in "
                    "hexadecimal with one space between each byte")
                     .arg(GUICommon::g_memTypeNames.at(typeIndex));
    else
      errorMsg = tr("The value you entered for the type %1 in the base %2 is invalid")
                     .arg(GUICommon::g_memTypeNames.at(typeIndex))
                     .arg(GUICommon::g_memBaseNames.at(baseIndex));
    QMessageBox* errorBox = new QMessageBox(QMessageBox::Critical, tr("Invalid value"), errorMsg,
                                            QMessageBox::Ok, this);
    errorBox->exec();
    break;
  }
  case Common::MemOperationReturnCode::inputTooLong:
  {
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
    MemWatchEntry* entry = node->getEntry();
    int typeIndex = static_cast<int>(entry->getType());

    QString errorMsg("The value you entered for the type " +
                     GUICommon::g_memTypeNames.at(typeIndex) + " for the length " +
                     QString::fromStdString(std::to_string(entry->getLength())) +
                     " is too long for the given length");
    if (entry->getType() == Common::MemType::type_byteArray)
      errorMsg += (", you must enter the bytes in hexadecimal with one space between each byte");
    QMessageBox* errorBox = new QMessageBox(QMessageBox::Critical, QString("Invalid value"),
                                            errorMsg, QMessageBox::Ok, this);
    errorBox->exec();
    break;
  }
  case Common::MemOperationReturnCode::invalidPointer:
  {
    QMessageBox* errorBox = new QMessageBox(QMessageBox::Critical, QString("Invalid pointer"),
                                            "This pointer points to invalid memory, therefore, you "
                                            "cannot write to the value of this watch",
                                            QMessageBox::Ok, this);
    errorBox->exec();
    break;
  }
  case Common::MemOperationReturnCode::operationFailed:
  {
    emit mustUnhook();
    break;
  }
  case Common::MemOperationReturnCode::OK:
    break;
  case Common::MemOperationReturnCode::noAbsoluteBranchForPPC:
    break;
  }
}

void MemWatchWidget::onAddGroup()
{
  bool ok = false;
  QString text = QInputDialog::getText(this, "Add a group",
                                       "Enter the group name:", QLineEdit::Normal, "", &ok);
  if (ok && !text.isEmpty())
  {
    const QModelIndexList selectedIndexes{m_watchView->selectionModel()->selectedIndexes()};
    m_watchModel->addGroup(text, selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back());
    m_hasUnsavedChanges = true;
  }
}

void MemWatchWidget::onAddWatchEntry()
{
  DlgAddWatchEntry dlg(true, nullptr, m_structDefs->getStructNames(), this);
  if (dlg.exec() == QDialog::Accepted)
  {
    addWatchEntry(dlg.stealEntry());
  }
}

void MemWatchWidget::addWatchEntry(MemWatchEntry* entry)
{
  const QModelIndexList selectedIndexes{m_watchView->selectionModel()->selectedIndexes()};
  m_watchModel->addEntry(entry, selectedIndexes.empty() ? QModelIndex{} : selectedIndexes.back());
  m_hasUnsavedChanges = true;
}

QModelIndexList MemWatchWidget::simplifySelection() const
{
  QModelIndexList simplifiedSelection;
  QModelIndexList selection = m_watchView->selectionModel()->selectedRows();

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

bool MemWatchWidget::isAnyAncestorSelected(const QModelIndex& index) const
{
  if (m_watchModel->parent(index) == QModelIndex())
    return false;
  if (m_watchView->selectionModel()->isSelected(index.parent()))
    return true;
  return isAnyAncestorSelected(index.parent());
}

void MemWatchWidget::onLockSelection(bool lockStatus)
{
  QModelIndexList selection = m_watchView->selectionModel()->selectedRows();
  if (selection.count() == 0)
    return;

  for (int i = 0; i < selection.count(); i++)
  {
    MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(selection.at(i))};
    if (!node->isGroup())
    {
      MemWatchEntry* entry = node->getEntry();
      entry->setLock(lockStatus);
    }
  }
}

void MemWatchWidget::onDeleteSelection()
{
  QModelIndexList selection = m_watchView->selectionModel()->selectedRows();
  if (selection.count() == 0)
    return;

  bool hasGroupWithChild = false;
  for (int i = 0; i < selection.count(); i++)
  {
    const QModelIndex index = selection.at(i);
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
    if (node->isGroup() && node->hasChildren())
    {
      hasGroupWithChild = true;
      break;
    }
  }

  QString confirmationMsg = "Are you sure you want to delete these watches and/or groups?";
  if (hasGroupWithChild)
    confirmationMsg +=
        "\n\nThe current selection contains one or more groups with watches in them, deleting "
        "the "
        "groups will also delete their watches, if you want to avoid this, move the watches out "
        "of "
        "the groups.";

  QMessageBox* confirmationBox =
      new QMessageBox(QMessageBox::Question, QString("Deleting confirmation"), confirmationMsg,
                      QMessageBox::Yes | QMessageBox::Cancel, this);
  confirmationBox->setDefaultButton(QMessageBox::Yes);
  if (confirmationBox->exec() == QMessageBox::Yes)
  {
    const QModelIndexList toDeleteList{simplifySelection()};
    for (const auto& index : toDeleteList)
    {
      m_watchModel->deleteNode(index);
    }

    m_hasUnsavedChanges = true;
  }
}

void MemWatchWidget::onDropSucceeded()
{
  m_hasUnsavedChanges = true;
}

void MemWatchWidget::onRowsInserted(const QModelIndex& parent, const int first, const int last)
{
  const QModelIndex firstIndex{m_watchModel->index(first, 0, parent)};
  const QModelIndex lastIndex{m_watchModel->index(last, 0, parent)};
  const QItemSelection selection{firstIndex,
                                 lastIndex.siblingAtColumn(MemWatchModel::WATCH_COL_NUM - 1)};

  QItemSelectionModel* const selectionModel{m_watchView->selectionModel()};
  // If the parent node is a container and it is not expanded, do not select the child node or
  // expand it.
  const MemWatchTreeNode* parentNode = MemWatchModel::getTreeNodeFromIndex(parent);
  if (parentNode != nullptr && !parentNode->isGroup() &&
      parentNode != m_watchModel->getRootNode() &&
      GUICommon::isContainerType(parentNode->getEntry()->getType()))
  {
    selectionModel->clearSelection();
    return;
  }

  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
  selectionModel->setCurrentIndex(lastIndex, QItemSelectionModel::Current);

  QTimer::singleShot(0, [this, parent, first, last, lastIndex] {
    for (int i{first}; i <= last; ++i)
    {
      const MemWatchTreeNode* const node{
          MemWatchModel::getTreeNodeFromIndex(m_watchModel->index(i, 0, parent))};
      updateExpansionState(node);
    }

    m_watchView->scrollTo(lastIndex);
  });
}

void MemWatchWidget::onCollapsed(const QModelIndex& index)
{
  MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
  if (!node)
    return;
  node->setExpanded(false);
  m_hasUnsavedChanges = true;

  if (!node->isGroup() && GUICommon::isContainerType(node->getEntry()->getType()))
    m_watchModel->collapseContainerNode(node);
}

void MemWatchWidget::onExpanded(const QModelIndex& index)
{
  MemWatchTreeNode* const node{MemWatchModel::getTreeNodeFromIndex(index)};
  if (!node)
    return;
  node->setExpanded(true);
  m_hasUnsavedChanges = true;

  if (!node->isGroup() && GUICommon::isContainerType(node->getEntry()->getType()))
    m_watchModel->expandContainerNode(node);
}

QTimer* MemWatchWidget::getUpdateTimer() const
{
  return m_updateTimer;
}

QTimer* MemWatchWidget::getFreezeTimer() const
{
  return m_freezeTimer;
}

void MemWatchWidget::openWatchFile(const QString& fileName)
{
  QString srcFileName;
  if (fileName.isEmpty())
  {
    srcFileName = QFileDialog::getOpenFileName(this, "Open watch list", m_watchListFile,
                                               "Dolphin memory watches file (*.dmw)");
  }
  else
  {
    srcFileName = fileName;
  }
  if (!srcFileName.isEmpty())
  {
    bool structsOnly = false;
    bool clearStructTree = false;
    if (m_watchModel->hasAnyNodes() || m_structDefs->hasChildren())
    {
      QMessageBox* questionBox = new QMessageBox(
          QMessageBox::Question, "Asking to merge lists",
          "The current watch list has entries in it, do you want to merge it with this one?",
          QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, this);
      QPushButton* structOnly = questionBox->addButton(tr("Structs only"), QMessageBox::AcceptRole);
      int answer = questionBox->exec();
      if (answer == QMessageBox::Cancel)
        return;
      else if (answer == QMessageBox::No)
      {
        m_watchModel->clearRoot();
        clearStructTree = true;
      }
      else if (answer != QMessageBox::Yes && questionBox->clickedButton() == structOnly)
        structsOnly = true;
    }

    QFile watchFile(srcFileName);
    if (!watchFile.exists())
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, QString("Error while opening file"),
          QString("The watch list file " + srcFileName + " does not exist"), QMessageBox::Ok, this);
      errorBox->exec();
      return;
    }
    if (!watchFile.open(QIODevice::ReadOnly))
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Error while opening file",
          "An error occured while opening the watch list file for reading", QMessageBox::Ok, this);
      errorBox->exec();
      return;
    }
    QByteArray bytes = watchFile.readAll();
    watchFile.close();
    QJsonDocument loadDoc(QJsonDocument::fromJson(bytes));
    QMap<QString, QString> structReplacements = {};
    emit loadStructDefsFromJson(loadDoc.object(), structReplacements, clearStructTree);
    if (!structsOnly)
      m_watchModel->loadRootFromJsonRecursive(loadDoc.object(), structReplacements);
    updateExpansionState();
    m_watchListFile = srcFileName;
    m_hasUnsavedChanges = false;
  }
}

bool MemWatchWidget::saveWatchFile()
{
  if (QFile::exists(m_watchListFile))
  {
    QFile watchFile(m_watchListFile);
    if (!watchFile.open(QIODevice::WriteOnly))
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Error while creating file",
          "An error occured while creating and opening the watch list file for writting",
          QMessageBox::Ok, this);
      errorBox->exec();
      return false;
    }
    QJsonObject root;
    m_watchModel->writeRootToJsonRecursive(root);
    emit writeStructDefTreeToJson(root);
    QJsonDocument saveDoc(root);
    watchFile.write(saveDoc.toJson());
    watchFile.close();
    m_hasUnsavedChanges = false;
    return true;
  }
  return saveAsWatchFile();
}

bool MemWatchWidget::saveAsWatchFile()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Save watch list", m_watchListFile,
                                                  "Dolphin memory watches file (*.dmw)");
  if (fileName != "")
  {
    if (!fileName.endsWith(".dmw"))
      fileName.append(".dmw");
    QFile watchFile(fileName);
    if (!watchFile.open(QIODevice::WriteOnly))
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Error while creating file",
          "An error occured while creating and opening the watch list file for writting",
          QMessageBox::Ok, this);
      errorBox->exec();
      return false;
    }
    QJsonObject root;
    m_watchModel->writeRootToJsonRecursive(root);
    emit writeStructDefTreeToJson(root);
    QJsonDocument saveDoc(root);
    watchFile.write(saveDoc.toJson());
    watchFile.close();
    m_watchListFile = fileName;
    m_hasUnsavedChanges = false;
    return true;
  }
  return false;
}

void MemWatchWidget::clearWatchList()
{
  if (!m_watchModel->hasAnyNodes() && !m_structDefs->hasChildren())
  {
    m_watchListFile.clear();
    return;
  }

  const QString msg{tr("Are you sure you want to delete all watches and structs?")};
  QMessageBox box(QMessageBox::Question, tr("Clear confirmation"), msg,
                  QMessageBox::Yes | QMessageBox::Cancel, this);
  box.setDefaultButton(QMessageBox::Yes);
  if (box.exec() != QMessageBox::Yes)
    return;

  m_watchListFile.clear();
  m_structDefs->removeChildren();
  m_watchModel->clearRoot();
}

void MemWatchWidget::importFromCTFile()
{
  if (!warnIfUnsavedChanges())
    return;

  DlgImportCTFile* dlg = new DlgImportCTFile(this);
  if (dlg->exec() == QDialog::Accepted)
  {
    QFile* CTFile = new QFile(dlg->getFileName(), this);
    if (!CTFile->exists())
    {
      QMessageBox* errorBox =
          new QMessageBox(QMessageBox::Critical, QString("Error while opening file"),
                          QString("The cheat table file " + CTFile->fileName() + " does not exist"),
                          QMessageBox::Ok, this);
      errorBox->exec();
      return;
    }
    if (!CTFile->open(QIODevice::ReadOnly))
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Error while opening file",
          "An error occured while opening the cheat table file for reading", QMessageBox::Ok, this);
      errorBox->exec();
      return;
    }

    bool useDolphinPointers = dlg->willUseDolphinPointers();
    MemWatchModel::CTParsingErrors parsingErrors;
    if (useDolphinPointers)
      parsingErrors = m_watchModel->importRootFromCTFile(CTFile, useDolphinPointers);
    else
      parsingErrors =
          m_watchModel->importRootFromCTFile(CTFile, useDolphinPointers, dlg->getCommonBase());
    CTFile->close();

    if (parsingErrors.errorStr.isEmpty())
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Information, "Import sucessfull",
          "The Cheat Table was imported sucessfully without errors.", QMessageBox::Ok, this);
      errorBox->exec();
    }
    else if (parsingErrors.isCritical)
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Import failed",
          "The Cheat Table could not have been imported, here are the details of the error:\n\n" +
              parsingErrors.errorStr,
          QMessageBox::Ok, this);
      errorBox->exec();
    }
    else
    {
      QMessageBox* errorBox =
          new QMessageBox(QMessageBox::Warning, "Cheat table imported with errors",
                          "The Cheat Table was imported with error(s), click \"Show Details...\" "
                          "to see the error(s) detail(s) (you can right-click to select all the "
                          "details and copy it to the clipboard)",
                          QMessageBox::Ok, this);
      errorBox->setDetailedText(parsingErrors.errorStr);
      errorBox->exec();
    }
  }
}

void MemWatchWidget::exportWatchListAsCSV()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Export Watch List", m_watchListFile,
                                                  "Column separated values file (*.csv)");
  if (fileName != "")
  {
    if (!fileName.endsWith(".csv"))
      fileName.append(".csv");
    QFile csvFile(fileName);
    if (!csvFile.open(QIODevice::WriteOnly))
    {
      QMessageBox* errorBox =
          new QMessageBox(QMessageBox::Critical, "Error while creating file",
                          "An error occured while creating and opening the csv file for writting",
                          QMessageBox::Ok, this);
      errorBox->exec();
      return;
    }
    QTextStream outputStream(&csvFile);
    QString csvContents = m_watchModel->writeRootToCSVStringRecursive();
    outputStream << csvContents;
    csvFile.close();
  }
}

bool MemWatchWidget::warnIfUnsavedChanges()
{
  if (m_hasUnsavedChanges && m_watchModel->getRootNode()->hasChildren())
  {
    QMessageBox* questionBox = new QMessageBox(
        QMessageBox::Question, "Unsaved changes",
        "You have unsaved changes in the current watch list, do you want to save them?",
        QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, this);
    switch (questionBox->exec())
    {
    case QMessageBox::Cancel:
      return false;
    case QMessageBox::No:
      return true;
    case QMessageBox::Yes:
      return saveWatchFile();
    default:
      return false;
    }
  }
  return true;
}

void MemWatchWidget::restoreWatchModel(const QString& json)
{
  const QJsonDocument loadDoc(QJsonDocument::fromJson(json.toUtf8()));
  m_watchModel->loadRootFromJsonRecursive(loadDoc.object());
  updateExpansionState();
}

QString MemWatchWidget::saveWatchModel()
{
  QJsonObject root;
  m_watchModel->writeRootToJsonRecursive(root);
  QJsonDocument saveDoc(root);
  return saveDoc.toJson();
}

void MemWatchWidget::setStructDefs(StructTreeNode* structDefs, QMap<QString, StructDef*> structMap)
{
  m_structDefs = structDefs;
  m_watchModel->setStructMap(structMap);
}

void MemWatchWidget::onUpdateStructDetails(QString structName)
{
  m_watchModel->updateStructEntries(structName);
  m_hasUnsavedChanges = true;
}

void MemWatchWidget::onUpdateStructName(QString oldName, QString newName)
{
  m_watchModel->onStructNameChanged(oldName, newName);
  m_hasUnsavedChanges = true;
}

void MemWatchWidget::onStructDefAddRemove(QString structName, StructDef* structDef)
{
  m_watchModel->onStructDefAddRemove(structName, structDef);
  m_hasUnsavedChanges = true;
}

void MemWatchWidget::updateExpansionState(const MemWatchTreeNode* const node)
{
  QSignalBlocker signalBlocker(*m_watchView);

  std::vector<const MemWatchTreeNode*> nodes;
  nodes.push_back(node ? node : m_watchModel->getRootNode());

  while (!nodes.empty())
  {
    const MemWatchTreeNode* const node{nodes.back()};
    nodes.pop_back();

    if (!node)
      continue;

    if (node->isExpanded())
    {
      m_watchView->setExpanded(m_watchModel->getIndexFromTreeNode(node), true);
    }

    for (const MemWatchTreeNode* const child : node->getChildren())
    {
      nodes.push_back(child);
    }
  }
}
