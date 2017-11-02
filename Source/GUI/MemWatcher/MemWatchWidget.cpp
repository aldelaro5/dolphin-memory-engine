#include "MemWatchWidget.h"

#include <QAction>
#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIODevice>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QSignalMapper>
#include <QString>
#include <QVBoxLayout>
#include <string>

#include "../../Common/MemoryCommon.h"
#include "../../MemoryWatch/MemoryWatch.h"
#include "../GUICommon.h"
#include "DlgAddWatchEntry.h"
#include "DlgChangeType.h"

MemWatchWidget::MemWatchWidget(QWidget* parent) : QWidget(parent)
{
  m_watchModel = new MemWatchModel(this);
  m_watchDelegate = new MemWatchDelegate();

  m_watchView = new QTreeView;
  m_watchView->setDragEnabled(true);
  m_watchView->setAcceptDrops(true);
  m_watchView->setDragDropMode(QAbstractItemView::InternalMove);
  m_watchView->setDropIndicatorShown(true);
  m_watchView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_watchView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_watchView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_watchView,
          static_cast<void (QWidget::*)(const QPoint&)>(&QWidget::customContextMenuRequested), this,
          static_cast<void (MemWatchWidget::*)(const QPoint&)>(
              &MemWatchWidget::onMemWatchContextMenuRequested));
  connect(m_watchModel,
          static_cast<void (MemWatchModel::*)(const QModelIndex&, Common::MemOperationReturnCode)>(
              &MemWatchModel::writeFailed),
          this,
          static_cast<void (MemWatchWidget::*)(const QModelIndex&, Common::MemOperationReturnCode)>(
              &MemWatchWidget::onValueWriteError));
  connect(m_watchModel, &MemWatchModel::dropSucceeded, this, &MemWatchWidget::onDropSucceeded);
  connect(m_watchView,
          static_cast<void (QAbstractItemView::*)(const QModelIndex&)>(
              &QAbstractItemView::doubleClicked),
          this,
          static_cast<void (MemWatchWidget::*)(const QModelIndex&)>(
              &MemWatchWidget::onWatchDoubleClicked));
  m_watchView->setItemDelegate(m_watchDelegate);
  m_watchView->setModel(m_watchModel);
  QShortcut* shortcut = new QShortcut(QKeySequence::Delete, m_watchView);
  connect(shortcut, &QShortcut::activated, this, &MemWatchWidget::onDeleteNode);

  QWidget* buttons = new QWidget;
  QHBoxLayout* buttons_layout = new QHBoxLayout;
  m_btnAddGroup = new QPushButton("Add group", this);
  m_btnAddWatchEntry = new QPushButton("Add watch", this);

  buttons_layout->addWidget(m_btnAddGroup);
  buttons_layout->addWidget(m_btnAddWatchEntry);
  buttons->setLayout(buttons_layout);
  connect(m_btnAddGroup, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemWatchWidget::onAddGroup);
  connect(m_btnAddWatchEntry, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemWatchWidget::onAddWatchEntry);

  QVBoxLayout* layout = new QVBoxLayout;

  layout->addWidget(buttons);
  layout->addWidget(m_watchView);

  setLayout(layout);

  connect(m_watchModel, &MemWatchModel::readFailed, this, &MemWatchWidget::mustUnhook);

  m_updateTimer = new QTimer(this);
  connect(m_updateTimer, &QTimer::timeout, m_watchModel, &MemWatchModel::onUpdateTimer);

  m_freezeTimer = new QTimer(this);
  connect(m_freezeTimer, &QTimer::timeout, m_watchModel, &MemWatchModel::onFreezeTimer);
}

void MemWatchWidget::onMemWatchContextMenuRequested(const QPoint& pos)
{
  QModelIndex index = m_watchView->indexAt(pos);
  if (index != QModelIndex())
  {
    MemWatchTreeNode* node = m_watchModel->getTreeNodeFromIndex(index);
    if (!node->isGroup())
    {
      MemWatchEntry* entry = m_watchModel->getEntryFromIndex(index);
      int typeIndex = static_cast<int>(entry->getType());
      Common::MemType theType = static_cast<Common::MemType>(typeIndex);

      QMenu* contextMenu = new QMenu(this);

      if (entry->isBoundToPointer())
      {
        QMenu* memViewerSubMenu = contextMenu->addMenu("Browse memory at");
        QAction* showPointerInViewer = new QAction("The pointer address...", this);
        connect(showPointerInViewer, &QAction::triggered, this,
                [=] { emit goToAddressInViewer(entry->getConsoleAddress()); });
        memViewerSubMenu->addAction(showPointerInViewer);
        for (int i = 0; i < entry->getPointerLevel(); ++i)
        {
          std::string strAddressOfPath = entry->getAddressStringForPointerLevel(i + 1);
          if (strAddressOfPath == "???")
            break;
          QAction* showAddressOfPathInViewer =
              new QAction("The pointed address at level " + QString::number(i + 1) + "...", this);
          connect(showAddressOfPathInViewer, &QAction::triggered, this,
                  [=] { emit goToAddressInViewer(entry->getAddressForPointerLevel(i + 1)); });
          memViewerSubMenu->addAction(showAddressOfPathInViewer);
        }

        QAction* showInViewer = new QAction("Browse memory at this address...", this);
        connect(showInViewer, &QAction::triggered, this,
                [=] { emit goToAddressInViewer(entry->getConsoleAddress()); });
      }
      else
      {
        QAction* showInViewer = new QAction("Browse memory at this address...", this);
        connect(showInViewer, &QAction::triggered, this,
                [=] { emit goToAddressInViewer(entry->getConsoleAddress()); });

        contextMenu->addAction(showInViewer);
      }
      if (theType != Common::MemType::type_string)
      {
        contextMenu->addSeparator();

        QAction* viewDec = new QAction("View as Decimal", this);
        QAction* viewHex = new QAction("View as Hexadecimal", this);
        QAction* viewOct = new QAction("View as Octal", this);
        QAction* viewBin = new QAction("View as Binary", this);

        connect(viewDec, &QAction::triggered, m_watchModel, [=] {
          entry->setBase(Common::MemBase::base_decimal);
          m_hasUnsavedChanges = true;
        });
        connect(viewHex, &QAction::triggered, m_watchModel, [=] {
          entry->setBase(Common::MemBase::base_hexadecimal);
          m_hasUnsavedChanges = true;
        });
        connect(viewOct, &QAction::triggered, m_watchModel, [=] {
          entry->setBase(Common::MemBase::base_octal);
          m_hasUnsavedChanges = true;
        });
        connect(viewBin, &QAction::triggered, m_watchModel, [=] {
          entry->setBase(Common::MemBase::base_binary);
          m_hasUnsavedChanges = true;
        });

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
          QAction* viewSigned = new QAction("View as Signed", this);
          QAction* viewUnsigned = new QAction("View as Unsigned", this);

          connect(viewSigned, &QAction::triggered, m_watchModel, [=] {
            entry->setSignedUnsigned(false);
            m_hasUnsavedChanges = true;
          });
          connect(viewUnsigned, &QAction::triggered, m_watchModel, [=] {
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
      contextMenu->popup(m_watchView->viewport()->mapToGlobal(pos));
    }
  }
}

void MemWatchWidget::onWatchDoubleClicked(const QModelIndex& index)
{
  if (index != QVariant())
  {
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
    if (index.column() == MemWatchModel::WATCH_COL_TYPE && !node->isGroup())
    {
      MemWatchEntry* entry = node->getEntry();
      int typeIndex = static_cast<int>(entry->getType());
      DlgChangeType* dlg = new DlgChangeType(this, typeIndex, entry->getLength());
      if (dlg->exec() == QDialog::Accepted)
      {
        Common::MemType theType = static_cast<Common::MemType>(dlg->getTypeIndex());
        m_watchModel->changeType(index, theType, dlg->getLength());
        m_hasUnsavedChanges = true;
      }
    }
    else if (index.column() == MemWatchModel::WATCH_COL_ADDRESS && !node->isGroup())
    {
      MemWatchEntry* entryCopy = new MemWatchEntry(node->getEntry());
      DlgAddWatchEntry* dlg = new DlgAddWatchEntry(entryCopy);
      if (dlg->exec() == QDialog::Accepted)
      {
        m_watchModel->editEntry(entryCopy, index);
        m_hasUnsavedChanges = true;
      }
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
    MemWatchTreeNode* node = m_watchModel->getTreeNodeFromIndex(index);
    MemWatchEntry* entry = node->getEntry();
    int typeIndex = static_cast<int>(entry->getType());
    int baseIndex = static_cast<int>(entry->getBase());

    QString errorMsg("The value you entered for the type " +
                     GUICommon::g_memTypeNames.at(typeIndex));
    if (entry->getType() == Common::MemType::type_byteArray)
      errorMsg +=
          (" is invalid, you must enter the bytes in hexadecimal with one space between each byte");
    else
      errorMsg += (" in the base " + GUICommon::g_memBaseNames.at(baseIndex) + " is invalid");
    QMessageBox* errorBox = new QMessageBox(QMessageBox::Critical, QString("Invalid value"),
                                            errorMsg, QMessageBox::Ok, this);
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
  }
}

void MemWatchWidget::onAddGroup()
{
  bool ok = false;
  QString text = QInputDialog::getText(this, "Add a group",
                                       "Enter the group name:", QLineEdit::Normal, "", &ok);
  if (ok && !text.isEmpty())
  {
    m_watchModel->addGroup(text);
    m_hasUnsavedChanges = true;
  }
}

void MemWatchWidget::onAddWatchEntry()
{
  DlgAddWatchEntry* dlg = new DlgAddWatchEntry(nullptr);
  if (dlg->exec() == QDialog::Accepted)
  {
    addWatchEntry(dlg->getEntry());
  }
}

void MemWatchWidget::addWatchEntry(MemWatchEntry* entry)
{
  m_watchModel->addEntry(entry);
  m_hasUnsavedChanges = true;
}

void MemWatchWidget::onDeleteNode()
{
  QModelIndexList selection = m_watchView->selectionModel()->selectedRows();
  bool hasGroupWithChild = false;
  for (int i = 0; i < selection.count(); i++)
  {
    const QModelIndex index = selection.at(i);
    MemWatchTreeNode* node = static_cast<MemWatchTreeNode*>(index.internalPointer());
    if (node->isGroup() && node->hasChildren())
    {
      hasGroupWithChild = true;
    }
  }

  QString confirmationMsg = "Are you sure you want to delete these watches and/or groups?";
  if (hasGroupWithChild)
    confirmationMsg +=
        "\n\nThe current selection contains one or more groups with watches in them, deleting the "
        "groups will also delete their watches, if you want to avoid this, move the watches out of "
        "the groups.";
  QMessageBox* confirmationBox =
      new QMessageBox(QMessageBox::Question, QString("Deleting confirmation"), confirmationMsg,
                      QMessageBox::Yes | QMessageBox::No, this);
  if (confirmationBox->exec() == QMessageBox::Yes)
  {
    // First, discard all indexes whose parent is selected already
    QModelIndexList* toDeleteList = new QModelIndexList();
    for (int i = 0; i < selection.count(); ++i)
    {
      const QModelIndex index = selection.at(i);
      if (!m_watchView->selectionModel()->isSelected(index.parent()))
      {
        toDeleteList->append(index);
      }
    }

    // Then, delete the rest (the children will be deleted too)
    for (auto i : *toDeleteList)
    {
      m_watchModel->removeNode(i);
    }
    m_hasUnsavedChanges = true;
  }
}

void MemWatchWidget::onDropSucceeded()
{
  m_hasUnsavedChanges = true;
}

QTimer* MemWatchWidget::getUpdateTimer() const
{
  return m_updateTimer;
}

QTimer* MemWatchWidget::getFreezeTimer() const
{
  return m_freezeTimer;
}

void MemWatchWidget::openWatchFile()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Open watch list", m_watchListFile,
                                                  "Dolphin memory watches file (*.dmw)");
  if (fileName != "")
  {
    QFile watchFile(fileName);
    if (!watchFile.exists())
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, QString("Error while opening file"),
          QString("The watch list file " + fileName + " does not exist"), QMessageBox::Ok, this);
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
    m_watchModel->loadRootFromJsonRecursive(loadDoc.object());
    m_watchListFile = fileName;
    m_hasUnsavedChanges = false;
  }
}

void MemWatchWidget::saveWatchFile()
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
      return;
    }
    QJsonObject root;
    m_watchModel->writeRootToJsonRecursive(root);
    QJsonDocument saveDoc(root);
    watchFile.write(saveDoc.toJson());
    watchFile.close();
    m_hasUnsavedChanges = false;
  }
  else
  {
    saveAsWatchFile();
  }
}

void MemWatchWidget::saveAsWatchFile()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Save watch list", m_watchListFile,
                                                  "Dolphin memory watches file (*.dmw)");
  if (fileName != "")
  {
    QFile watchFile(fileName);
    if (!watchFile.open(QIODevice::WriteOnly))
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Error while creating file",
          "An error occured while creating and opening the watch list file for writting",
          QMessageBox::Ok, this);
      errorBox->exec();
      return;
    }
    QJsonObject root;
    m_watchModel->writeRootToJsonRecursive(root);
    QJsonDocument saveDoc(root);
    watchFile.write(saveDoc.toJson());
    watchFile.close();
    m_watchListFile = fileName;
    m_hasUnsavedChanges = false;
  }
}

bool MemWatchWidget::warnIfUnsavedChanges()
{
  if (m_hasUnsavedChanges)
  {
    QMessageBox* questionBox = new QMessageBox(
        QMessageBox::Question, "Unsaved changes",
        "You have unsaved changes in the current watch list, do you want to save them?",
        QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);
    switch (questionBox->exec())
    {
    case QMessageBox::Cancel:
      return false;
    case QMessageBox::No:
      return true;
    case QMessageBox::Yes:
      saveWatchFile();
      return true;
    }
  }
  return true;
}