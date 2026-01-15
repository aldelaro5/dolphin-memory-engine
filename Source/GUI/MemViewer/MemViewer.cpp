#include "MemViewer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>

#include "../../Common/CommonUtils.h"
#include "../../DolphinProcess/DolphinAccessor.h"
#include "../MemWatcher/Dialogs/DlgAddWatchEntry.h"
#include "../Settings/SConfig.h"

MemViewer::MemViewer(QWidget* parent) : QAbstractScrollArea(parent)
{
  initialise();

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  changeMemoryRegion(MemoryRegion::MEM1);
  verticalScrollBar()->setPageStep(m_numRows);

  m_elapsedTimer.start();

  m_copyShortcut = new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_C), parent);
  connect(m_copyShortcut, &QShortcut::activated, this,
          [this]() { copySelection(Common::MemType::type_byteArray); });

  // The viewport is implicitly updated at the constructor's end
}

MemViewer::~MemViewer()
{
  delete[] m_updatedRawMemoryData;
  delete[] m_lastRawMemoryData;
  delete[] m_memoryMsElapsedLastChange;
  delete m_curosrRect;
}

void MemViewer::initialise()
{
  updateFontSize();
  m_curosrRect = new QRect();
  m_updatedRawMemoryData = new char[m_numCells];
  m_lastRawMemoryData = new char[m_numCells];
  m_memoryMsElapsedLastChange = new int[m_numCells];
  m_memViewStart = Common::MEM1_START;
  m_memViewEnd = Common::GetMEM1End();
  m_currentFirstAddress = m_memViewStart;

  std::fill(m_memoryMsElapsedLastChange, m_memoryMsElapsedLastChange + m_numCells, 0);
  updateMemoryData();
  std::memcpy(m_lastRawMemoryData, m_updatedRawMemoryData, m_numCells);
}

QSize MemViewer::sizeHint() const
{
  return {m_rowHeaderWidth + m_hexAreaWidth + 17 * m_charWidthEm + verticalScrollBar()->width(),
          m_columnHeaderHeight + m_hexAreaHeight + m_charHeight / 2};
}

void MemViewer::memoryValidityChanged(const bool valid)
{
  m_validMemory = valid;
  if (valid)
  {
    if (m_memViewStart >= Common::MEM2_START)
    {
      m_memViewStart = Common::MEM2_START;
      m_memViewEnd = Common::GetMEM2End();
    }
    else if (m_memViewStart >= Common::MEM1_START)
    {
      m_memViewStart = Common::MEM1_START;
      m_memViewEnd = Common::GetMEM1End();
    }
    else if (m_memViewStart >= Common::ARAM_START)
    {
      m_memViewStart = Common::ARAM_START;
      m_memViewEnd = Common::ARAM_END;
    }
    verticalScrollBar()->setRange(
        0, (static_cast<int>(m_memViewEnd - m_memViewStart) / m_numColumns) - m_numRows);
  }
  viewport()->update();
}

void MemViewer::setStructDefs(StructTreeNode* baseNode)
{
  m_structDefs = baseNode;
}

void MemViewer::updateMemoryData()
{
  std::swap(m_updatedRawMemoryData, m_lastRawMemoryData);
  m_validMemory = false;
  if (DolphinComm::DolphinAccessor::isValidConsoleAddress(m_currentFirstAddress))
    m_validMemory = DolphinComm::DolphinAccessor::readFromRAM(
        Common::dolphinAddrToOffset(m_currentFirstAddress,
                                    DolphinComm::DolphinAccessor::isARAMAccessible()),
        m_updatedRawMemoryData, m_numCells, false);
  if (!m_validMemory)
    emit memErrorOccured();
}

void MemViewer::updateViewer()
{
  updateMemoryData();
  viewport()->update();
  if (!m_validMemory)
    emit memErrorOccured();
}

u32 MemViewer::getCurrentFirstAddress() const
{
  return m_currentFirstAddress;
}

void MemViewer::jumpToAddress(const u32 address)
{
  if (DolphinComm::DolphinAccessor::isValidConsoleAddress(address))
  {
    m_currentFirstAddress = address;
    if (address >= Common::ARAM_START && address < Common::ARAM_END &&
        m_memViewStart != Common::ARAM_START)
      changeMemoryRegion(MemoryRegion::ARAM);
    else if (address >= Common::MEM1_START && address < Common::GetMEM1End() &&
             m_memViewStart != Common::MEM1_START)
      changeMemoryRegion(MemoryRegion::MEM1);
    else if (address >= Common::MEM2_START && address < Common::GetMEM2End() &&
             m_memViewStart != Common::MEM2_START)
      changeMemoryRegion(MemoryRegion::MEM2);

    if (m_currentFirstAddress > m_memViewEnd - m_numCells)
      m_currentFirstAddress = m_memViewEnd - m_numCells;
    std::fill(m_memoryMsElapsedLastChange, m_memoryMsElapsedLastChange + m_numCells, 0);
    updateMemoryData();
    std::memcpy(m_lastRawMemoryData, m_updatedRawMemoryData, m_numCells);
    m_carrotIndex = 0;

    m_disableScrollContentEvent = true;
    verticalScrollBar()->setValue(static_cast<int>((address & 0xFFFFFFF0) - m_memViewStart) /
                                  m_numColumns);
    m_disableScrollContentEvent = false;

    viewport()->update();
  }
}

void MemViewer::changeMemoryRegion(const MemoryRegion region)
{
  switch (region)
  {
  case MemoryRegion::ARAM:
    m_memViewStart = Common::ARAM_START;
    m_memViewEnd = Common::ARAM_END;
    break;
  case MemoryRegion::MEM1:
    m_memViewStart = Common::MEM1_START;
    m_memViewEnd = Common::GetMEM1End();
    break;
  case MemoryRegion::MEM2:
    m_memViewStart = Common::MEM2_START;
    m_memViewEnd = Common::GetMEM2End();
    break;
  }
  verticalScrollBar()->setRange(
      0, (static_cast<int>(m_memViewEnd - m_memViewStart) / m_numColumns) - m_numRows);
}

MemViewer::bytePosFromMouse MemViewer::mousePosToBytePos(QPoint pos)
{
  int x = pos.x();
  int y = pos.y();

  const int spacing = m_charWidthEm / 2;
  const int hexCellWidth = m_charWidthEm * m_digitsPerBox + spacing;
  const int hexAreaLeft = m_rowHeaderWidth - spacing / 2;
  const int asciiAreaLeft = m_hexAsciiSeparatorPosX + spacing;
  const int areaTop = m_columnHeaderHeight + m_charHeight - fontMetrics().overlinePos();
  QRect hexArea(hexAreaLeft, areaTop, m_hexAreaWidth, m_charHeight * m_numRows);
  QRect asciiArea(asciiAreaLeft, areaTop, m_charWidthEm * m_numColumns, m_charHeight * m_numRows);

  bytePosFromMouse bytePos;
  bytePos.carrotIndex = 0;

  // Transform x and y to indices for column and row
  if (hexArea.contains(x, y, false))
  {
    bytePos.x = ((x - hexAreaLeft) / hexCellWidth) * m_sizeOfType;
    m_editingHex = true;
    bytePos.carrotIndex = ((x - hexAreaLeft) % hexCellWidth) / m_charWidthEm;
    if (bytePos.carrotIndex >= m_digitsPerBox)
      bytePos.carrotIndex = m_digitsPerBox - 1;
  }
  else if (asciiArea.contains(x, y, false))
  {
    bytePos.x = (x - asciiAreaLeft) / m_charWidthEm;
    m_editingHex = false;
  }
  else
  {
    bytePos.isInViewer = false;
    return bytePos;
  }
  bytePos.y = (y - areaTop) / m_charHeight;
  bytePos.isInViewer = true;
  return bytePos;
}

void MemViewer::mousePressEvent(QMouseEvent* event)
{
  // Only handle left-click events
  if (event->button() != Qt::MouseButton::LeftButton)
    return;

  bytePosFromMouse bytePos = mousePosToBytePos(event->pos());

  if (!bytePos.isInViewer)
    return;

  const bool wasEditingHex = m_editingHex;

  // Toggle carrot-index when the same byte is clicked twice from the hex table
  m_carrotIndex =
      (m_editingHex && wasEditingHex && m_carrotIndex != bytePos.carrotIndex &&
       m_StartBytesSelectionPosX == bytePos.x && m_StartBytesSelectionPosY == bytePos.y &&
       m_EndBytesSelectionPosX == bytePos.x && m_EndBytesSelectionPosY == bytePos.y) ?
          bytePos.carrotIndex :
          0;

  m_StartBytesSelectionPosX = bytePos.x;
  m_StartBytesSelectionPosY = bytePos.y;
  m_EndBytesSelectionPosX = bytePos.x;
  m_EndBytesSelectionPosY = bytePos.y;
  m_selectionType = SelectionType::single;

  viewport()->update();
}

void MemViewer::mouseMoveEvent(QMouseEvent* event)
{
  if (!(event->buttons() & Qt::LeftButton))
    return;

  bytePosFromMouse bytePos = mousePosToBytePos(event->pos());

  if (!bytePos.isInViewer)
    return;

  int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
  int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
  int indexDrag = bytePos.y * m_numColumns + bytePos.x;

  // The selection is getting retracted, but still goes the same direction as before
  if (indexDrag > indexStart && indexDrag < indexEnd)
  {
    if (m_selectionType == SelectionType::upward)
    {
      m_StartBytesSelectionPosX = bytePos.x;
      m_StartBytesSelectionPosY = bytePos.y;
    }
    else if (m_selectionType == SelectionType::downward)
    {
      m_EndBytesSelectionPosX = bytePos.x;
      m_EndBytesSelectionPosY = bytePos.y;
    }
  }
  // The selection either expands upwards OR it switches to upward
  else if (indexDrag < indexStart && indexDrag < indexEnd)
  {
    if (m_selectionType == SelectionType::downward)
    {
      m_EndBytesSelectionPosX = m_StartBytesSelectionPosX;
      m_EndBytesSelectionPosY = m_StartBytesSelectionPosY;
    }
    m_StartBytesSelectionPosX = bytePos.x;
    m_StartBytesSelectionPosY = bytePos.y;
    m_selectionType = SelectionType::upward;
  }
  // The selection either expands downward OR it switches to downward
  else if (indexDrag > indexStart && indexDrag > indexEnd)
  {
    if (m_selectionType == SelectionType::upward)
    {
      m_StartBytesSelectionPosX = m_EndBytesSelectionPosX;
      m_StartBytesSelectionPosY = m_EndBytesSelectionPosY;
    }
    m_EndBytesSelectionPosX = bytePos.x;
    m_EndBytesSelectionPosY = bytePos.y;
    m_selectionType = SelectionType::downward;
  }
  // The selection is just one byte
  else if ((indexDrag == indexStart && m_selectionType == SelectionType::downward) ||
           (indexDrag == indexEnd && m_selectionType == SelectionType::upward))
  {
    m_StartBytesSelectionPosX = bytePos.x;
    m_StartBytesSelectionPosY = bytePos.y;
    m_EndBytesSelectionPosX = bytePos.x;
    m_EndBytesSelectionPosY = bytePos.y;
    m_selectionType = SelectionType::single;
  }

  viewport()->update();
}

void MemViewer::contextMenuEvent(QContextMenuEvent* event)
{
  if (event->reason() == QContextMenuEvent::Reason::Mouse)
  {
    bytePosFromMouse bytePos = mousePosToBytePos(event->pos());
    if (!bytePos.isInViewer)
    {
      event->ignore();
      return;
    }

    int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
    int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
    int indexMouse = bytePos.y * m_numColumns + bytePos.x;

    QMenu* contextMenu = new QMenu(this);

    if (indexMouse >= indexStart && indexMouse <= indexEnd)
    {
      QAction* copyBytesAction = new QAction(tr("&Copy selection as bytes"));
      connect(copyBytesAction, &QAction::triggered, this,
              [this]() { copySelection(Common::MemType::type_byteArray); });
      contextMenu->addAction(copyBytesAction);

      QAction* copyStringAction = new QAction(tr("&Copy selection as ASCII string"));
      connect(copyStringAction, &QAction::triggered, this,
              [this]() { copySelection(Common::MemType::type_string); });
      contextMenu->addAction(copyStringAction);

      QAction* editAction = new QAction(tr("&Edit all selection..."));
      connect(editAction, &QAction::triggered, this, &MemViewer::editSelection);
      contextMenu->addAction(editAction);

      QAction* addBytesArrayAction = new QAction(tr("&Add selection as array of bytes..."));
      connect(addBytesArrayAction, &QAction::triggered, this,
              &MemViewer::addSelectionAsArrayOfBytes);
      contextMenu->addAction(addBytesArrayAction);
    }

    QAction* addSingleWatchAction = new QAction(tr("&Add a watch to this address..."));
    connect(addSingleWatchAction, &QAction::triggered, this,
            [this, indexMouse]() { addByteIndexAsWatch(indexMouse); });
    contextMenu->addAction(addSingleWatchAction);

    // -----------------------------------------------
    // Everything below is for a
    // sub menu for changing how the memory is viewed
    // -----------------------------------------------

    QMenu* visualizeSubMenu = new QMenu("&View as", this);
    contextMenu->addMenu(visualizeSubMenu);

    QAction* visualize_oneByte = new QAction(tr("&One byte"));
    connect(visualize_oneByte, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_byte); });
    visualizeSubMenu->addAction(visualize_oneByte);
    if (m_type == Common::MemType::type_byte)
      visualize_oneByte->setDisabled(true);

    QAction* visualize_twoBytes = new QAction(tr("&Two bytes (Halfword)"));
    connect(visualize_twoBytes, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_halfword); });
    visualizeSubMenu->addAction(visualize_twoBytes);
    if (m_type == Common::MemType::type_halfword)
      visualize_twoBytes->setDisabled(true);

    QAction* visualize_fourBytes = new QAction(tr("&Four bytes (Word)"));
    connect(visualize_fourBytes, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_word); });
    visualizeSubMenu->addAction(visualize_fourBytes);
    if (m_type == Common::MemType::type_word)
      visualize_fourBytes->setDisabled(true);

    QAction* visualize_eightBytes = new QAction(tr("&Eight bytes (Doubleword)"));
    connect(visualize_eightBytes, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_doubleword); });
    visualizeSubMenu->addAction(visualize_eightBytes);
    if (m_type == Common::MemType::type_doubleword)
      visualize_eightBytes->setDisabled(true);

    QAction* visualize_float = new QAction(tr("&Float"));
    connect(visualize_float, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_float); });
    visualizeSubMenu->addAction(visualize_float);
    if (m_type == Common::MemType::type_float)
      visualize_float->setDisabled(true);

    QAction* visualize_double = new QAction(tr("&Double"));
    connect(visualize_double, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_double); });
    visualizeSubMenu->addAction(visualize_double);
    if (m_type == Common::MemType::type_double)
      visualize_double->setDisabled(true);

    QAction* visualize_ppc = new QAction(tr("&Assembly (PowerPC)"));
    connect(visualize_ppc, &QAction::triggered, this,
            [this]() { setMemType(Common::MemType::type_ppc); });
    visualizeSubMenu->addAction(visualize_ppc);
    if (m_type == Common::MemType::type_ppc)
      visualize_ppc->setDisabled(true);

    visualizeSubMenu->addSeparator();

    QAction* visualize_signed = new QAction(tr("View as &Signed"));
    connect(visualize_signed, &QAction::triggered, this, [this]() { setSigned(false); });
    visualizeSubMenu->addAction(visualize_signed);
    if (!m_isUnsigned)
      visualize_signed->setDisabled(true);

    QAction* visualize_unsigned = new QAction(tr("View as &Unsigned"));
    connect(visualize_unsigned, &QAction::triggered, this, [this]() { setSigned(true); });
    visualizeSubMenu->addAction(visualize_unsigned);
    if (m_isUnsigned)
      visualize_unsigned->setDisabled(true);

    visualizeSubMenu->addSeparator();

    QAction* visualize_decimal = new QAction(tr("View as &Decimal"));
    connect(visualize_decimal, &QAction::triggered, this,
            [this]() { setBase(Common::MemBase::base_decimal); });
    visualizeSubMenu->addAction(visualize_decimal);
    if (m_base == Common::MemBase::base_decimal)
      visualize_decimal->setDisabled(true);

    QAction* visualize_hexadecimal = new QAction(tr("View as &Hexadecimal"));
    connect(visualize_hexadecimal, &QAction::triggered, this,
            [this]() { setBase(Common::MemBase::base_hexadecimal); });
    visualizeSubMenu->addAction(visualize_hexadecimal);
    if (m_base == Common::MemBase::base_hexadecimal)
      visualize_hexadecimal->setDisabled(true);

    QAction* visualize_binary = new QAction(tr("View as &Binary"));
    connect(visualize_binary, &QAction::triggered, this,
            [this]() { setBase(Common::MemBase::base_binary); });
    visualizeSubMenu->addAction(visualize_binary);
    if (m_base == Common::MemBase::base_binary)
      visualize_binary->setDisabled(true);

    visualizeSubMenu->addSeparator();

    QAction* visualize_absolute = new QAction(tr("Branch type &Absolute"));
    connect(visualize_absolute, &QAction::triggered, this, [this]() { setBranchType(true); });
    visualizeSubMenu->addAction(visualize_absolute);
    if (m_absoluteBranch)
      visualize_absolute->setDisabled(true);

    QAction* visualize_relative = new QAction(tr("Branch type &Relative"));
    connect(visualize_relative, &QAction::triggered, this, [this]() { setBranchType(false); });
    visualizeSubMenu->addAction(visualize_relative);
    if (!m_absoluteBranch)
      visualize_relative->setDisabled(true);

    contextMenu->popup(viewport()->mapToGlobal(event->pos()));
  }
}

void MemViewer::wheelEvent(QWheelEvent* event)
{
  if (event->modifiers().testFlag(Qt::ControlModifier))
  {
    if (event->angleDelta().y() < 0 && m_memoryFontSize > 5)
    {
      m_memoryFontSize -= 1;
    }
    else if (event->angleDelta().y() > 0)
    {
      m_memoryFontSize += 1;
    }
    updateFontSize();

    viewport()->update();
  }
  else
  {
    QAbstractScrollArea::wheelEvent(event);
  }
}

void MemViewer::updateFontSize()
{
  if (m_memoryFontSize == -1)
  {
    m_memoryFontSize = static_cast<int>(font().pointSize() * 1.5);
  }

  QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  fixedFont.setPointSize(m_memoryFontSize);
  setFont(fixedFont);

  m_charWidthEm = fontMetrics().horizontalAdvance(QLatin1Char('M'));
  m_charHeight = fontMetrics().height();
  m_hexAreaWidth =
      (m_numColumns / m_sizeOfType) * (m_charWidthEm * m_digitsPerBox + m_charWidthEm / 2);
  m_hexAreaHeight = m_numRows * m_charHeight;
  m_rowHeaderWidth = m_charWidthEm * (static_cast<int>(sizeof(u32)) * 2 + 1) + m_charWidthEm / 2;
  m_hexAsciiSeparatorPosX = m_rowHeaderWidth + m_hexAreaWidth;
  m_columnHeaderHeight = m_charHeight + m_charWidthEm / 2;
}

void MemViewer::setMemType(Common::MemType type)
{
  m_type = type;
  m_sizeOfType = static_cast<int>(Common::getSizeForType(type, 1));
  updateDigitsPerBox();
}

void MemViewer::setBase(Common::MemBase base)
{
  m_base = base;
  updateDigitsPerBox();
}

void MemViewer::setSigned(bool isUnsigned)
{
  m_isUnsigned = isUnsigned;
  updateDigitsPerBox();
}

void MemViewer::setBranchType(bool absoluteBranch)
{
  m_absoluteBranch = absoluteBranch;
  updateDigitsPerBox();
}

void MemViewer::updateDigitsPerBox()
{
  switch (m_type)
  {
  case Common::MemType::type_byte:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      m_digitsPerBox = m_isUnsigned ? 3 : 4;
      break;
    case Common::MemBase::base_hexadecimal:
      m_digitsPerBox = 2;
      break;
    case Common::MemBase::base_binary:
      m_digitsPerBox = 8;
      break;
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      m_digitsPerBox = 2;  // Shouldn't ever reach here
    }
    break;
  case Common::MemType::type_halfword:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      m_digitsPerBox = m_isUnsigned ? 5 : 6;
      break;
    case Common::MemBase::base_hexadecimal:
      m_digitsPerBox = 4;
      break;
    case Common::MemBase::base_binary:
      m_digitsPerBox = 16;
      break;
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      m_digitsPerBox = 2;  // Shouldn't ever reach here
    }
    break;
  case Common::MemType::type_word:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      m_digitsPerBox = m_isUnsigned ? 10 : 11;
      break;
    case Common::MemBase::base_hexadecimal:
      m_digitsPerBox = 8;
      break;
    case Common::MemBase::base_binary:
      m_digitsPerBox = 32;
      break;
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      m_digitsPerBox = 2;  // Shouldn't ever reach here
    }
    break;
  case Common::MemType::type_doubleword:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      m_digitsPerBox = 20;
      break;
    case Common::MemBase::base_hexadecimal:
      m_digitsPerBox = 16;
      break;
    case Common::MemBase::base_binary:
      m_digitsPerBox = 64;
      break;
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      m_digitsPerBox = 2;  // Shouldn't ever reach here
    }
    break;
  case Common::MemType::type_float:
    m_digitsPerBox = 18;
    break;
  case Common::MemType::type_double:
    m_digitsPerBox = 28;
    break;
  case Common::MemType::type_ppc:
    m_digitsPerBox = 32;
    break;
  case Common::MemType::type_string:
  case Common::MemType::type_byteArray:
  case Common::MemType::type_struct:
  case Common::MemType::type_array:
  case Common::MemType::type_none:
    m_digitsPerBox = 2;  // Shouldn't ever reach here
  }
  m_carrotIndex = 0;
  updateFontSize();
  updateGeometry();
  viewport()->update();
}

void MemViewer::scrollToSelection()
{
  if (m_selectionType != SelectionType::downward)
  {
    if (m_StartBytesSelectionPosY < 0)
      scrollContentsBy(0, -m_StartBytesSelectionPosY);
    else if (m_StartBytesSelectionPosY >= m_numRows)
      scrollContentsBy(0, m_numRows - m_StartBytesSelectionPosY - 1);
  }
  else
  {
    if (m_EndBytesSelectionPosY < 0)
      scrollContentsBy(0, -m_EndBytesSelectionPosY);
    else if (m_EndBytesSelectionPosY >= m_numRows)
      scrollContentsBy(0, m_numRows - m_EndBytesSelectionPosY - 1);
  }

  viewport()->update();
}

void MemViewer::copySelection(const Common::MemType type) const
{
  int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
  int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
  if (m_editingHex)
    indexEnd += (m_sizeOfType - 1);
  size_t selectionLength = static_cast<size_t>(indexEnd - indexStart + 1);
  if (m_editingHex)
    selectionLength -= selectionLength % static_cast<size_t>(m_sizeOfType);

  char* selectedMem = new char[selectionLength];
  if (DolphinComm::DolphinAccessor::isValidConsoleAddress(m_currentFirstAddress))
  {
    bool valid = DolphinComm::DolphinAccessor::readFromRAM(
        Common::dolphinAddrToOffset(m_currentFirstAddress + indexStart,
                                    DolphinComm::DolphinAccessor::isARAMAccessible()),
        selectedMem, selectionLength, false);
    if (valid)
    {
      std::string bytes = Common::formatMemoryToString(selectedMem, type, selectionLength,
                                                       Common::MemBase::base_none, true);
      QClipboard* clipboard = QGuiApplication::clipboard();
      clipboard->setText(QString::fromStdString(bytes));
    }
  }
  delete[] selectedMem;
}

QString MemViewer::getEditAllText() const
{
  switch (m_type)
  {
  case Common::MemType::type_byte:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      return "Byte (in decimal)";
    case Common::MemBase::base_hexadecimal:
      return "Byte (in hex)";
    case Common::MemBase::base_binary:
      return "Byte (in binary)";
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      break;
    }
    break;
  case Common::MemType::type_halfword:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      return "Halfword (in decimal)";
    case Common::MemBase::base_hexadecimal:
      return "Halfword (in hex)";
    case Common::MemBase::base_binary:
      return "Halfword (in binary)";
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      break;
    }
    break;
  case Common::MemType::type_word:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      return "Word (in decimal)";
    case Common::MemBase::base_hexadecimal:
      return "Word (in hex)";
    case Common::MemBase::base_binary:
      return "Word (in binary)";
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      break;
    }
    break;
  case Common::MemType::type_doubleword:
    switch (m_base)
    {
    case Common::MemBase::base_decimal:
      return "Doubleword (in decimal)";
    case Common::MemBase::base_hexadecimal:
      return "Doubleword (in hex)";
    case Common::MemBase::base_binary:
      return "Doubleword (in binary)";
    case Common::MemBase::base_octal:
    case Common::MemBase::base_none:
      break;
    }
    break;
  case Common::MemType::type_float:
    return "Float";
  case Common::MemType::type_double:
    return "Double";
  case Common::MemType::type_ppc:
    return "Power PC Assembly (Absolute branch not supported)";
  case Common::MemType::type_string:
  case Common::MemType::type_byteArray:
  case Common::MemType::type_struct:
  case Common::MemType::type_array:
  case Common::MemType::type_none:
    break;
  }
  return "ERROR!!!";
}
void MemViewer::editSelection()
{
  int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
  int indexEnd =
      m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX + (m_sizeOfType - 1);
  size_t selectionLength = static_cast<size_t>(indexEnd - indexStart + 1);
  selectionLength -= selectionLength % static_cast<size_t>(m_sizeOfType);

  QString strByte = QInputDialog::getText(this, "Enter the value", getEditAllText());
  if (!strByte.isEmpty())
  {
    Common::MemOperationReturnCode return_code = Common::MemOperationReturnCode::OK;
    size_t len_actual = 0;
    const Common::MemBase base =
        (m_type == Common::MemType::type_double || m_type == Common::MemType::type_float) ?
            Common::MemBase::base_decimal :
            m_base;
    char* one_set_mem = Common::formatStringToMemory(return_code, len_actual, strByte.toStdString(),
                                                     base, m_type, m_sizeOfType, 0U);

    if (return_code != Common::MemOperationReturnCode::OK || len_actual == 0)
    {
      QMessageBox* errorBox = new QMessageBox(
          QMessageBox::Critical, "Invalid input",
          QString::fromStdString("The input you gave is invalid"), QMessageBox::Ok, this);
      errorBox->exec();
      delete[] one_set_mem;
      return;
    }

    if (Common::shouldBeBSwappedForType(m_type))
    {
      for (size_t i = 0; i < len_actual / 2; i++)
      {
        char temp = one_set_mem[i];
        one_set_mem[i] = one_set_mem[len_actual - i - 1];
        one_set_mem[len_actual - i - 1] = temp;
      }
    }

    char* newMem = new char[selectionLength];
    for (size_t i = 0; i < selectionLength; i += len_actual)
      std::memcpy(newMem + i, one_set_mem, len_actual);
    if (DolphinComm::DolphinAccessor::isValidConsoleAddress(m_currentFirstAddress + indexStart))
    {
      if (!DolphinComm::DolphinAccessor::writeToRAM(
              Common::dolphinAddrToOffset(m_currentFirstAddress + indexStart,
                                          DolphinComm::DolphinAccessor::isARAMAccessible()),
              newMem, selectionLength, false))
      {
        emit memErrorOccured();
      }
    }
    delete[] one_set_mem;
    delete[] newMem;
  }
}

void MemViewer::addSelectionAsArrayOfBytes()
{
  int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
  int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
  if (m_editingHex)
    indexEnd += (m_sizeOfType - 1);
  size_t selectionLength = static_cast<size_t>(indexEnd - indexStart + 1);
  if (m_editingHex)
    selectionLength -= selectionLength % static_cast<size_t>(m_sizeOfType);

  QString strLabel{QInputDialog::getText(this, "Enter the label of the new watch", "label")};
  if (!strLabel.isEmpty())
  {
    MemWatchEntry* newEntry = new MemWatchEntry(strLabel, m_currentFirstAddress + indexStart,
                                                Common::MemType::type_byteArray,
                                                Common::MemBase::base_none, true, selectionLength);
    emit addWatch(newEntry);
  }
}

void MemViewer::addByteIndexAsWatch(int index)
{
  MemWatchEntry* entry = new MemWatchEntry();
  entry->setConsoleAddress(m_currentFirstAddress + index);
  DlgAddWatchEntry dlg(true, entry, m_structDefs->getStructNames(), this);

  if (dlg.exec() == QDialog::Accepted)
  {
    emit addWatch(dlg.stealEntry());
  }
}

bool MemViewer::handleNaviguationKey(const int key, bool shiftIsHeld)
{
  switch (key)
  {
  case Qt::Key_Up:
    if (verticalScrollBar()->value() + m_StartBytesSelectionPosY <= verticalScrollBar()->minimum())
    {
      QApplication::beep();
    }
    else
    {
      if (m_selectionType == SelectionType::downward && shiftIsHeld)
      {
        m_EndBytesSelectionPosY--;
        int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
        int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
        if (indexEnd <= indexStart)
        {
          std::swap(m_StartBytesSelectionPosX, m_EndBytesSelectionPosX);
          std::swap(m_StartBytesSelectionPosY, m_EndBytesSelectionPosY);
          if (indexEnd == indexStart)
            m_selectionType = SelectionType::single;
          else
            m_selectionType = SelectionType::upward;
        }
      }
      else
      {
        m_StartBytesSelectionPosY--;
        if (!shiftIsHeld)
        {
          m_EndBytesSelectionPosX = m_StartBytesSelectionPosX;
          m_EndBytesSelectionPosY = m_StartBytesSelectionPosY;
          m_selectionType = SelectionType::single;
        }
        else
        {
          m_selectionType = SelectionType::upward;
        }
      }
    }

    scrollToSelection();
    break;
  case Qt::Key_Down:
    if (verticalScrollBar()->value() + m_StartBytesSelectionPosY >= verticalScrollBar()->maximum())
    {
      QApplication::beep();
    }
    else
    {
      if (m_selectionType == SelectionType::upward && shiftIsHeld)
      {
        m_StartBytesSelectionPosY++;
        int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
        int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
        if (indexEnd <= indexStart)
        {
          std::swap(m_StartBytesSelectionPosX, m_EndBytesSelectionPosX);
          std::swap(m_StartBytesSelectionPosY, m_EndBytesSelectionPosY);
          if (indexEnd == indexStart)
            m_selectionType = SelectionType::single;
          else
            m_selectionType = SelectionType::downward;
        }
      }
      else
      {
        m_EndBytesSelectionPosY++;
        if (!shiftIsHeld)
        {
          m_StartBytesSelectionPosX = m_EndBytesSelectionPosX;
          m_StartBytesSelectionPosY = m_EndBytesSelectionPosY;
          m_selectionType = SelectionType::single;
        }
        else
        {
          m_selectionType = SelectionType::downward;
        }
      }
    }

    scrollToSelection();
    break;
  case Qt::Key_Left:
    if (m_StartBytesSelectionPosX <= 0 && m_StartBytesSelectionPosY <= 0 &&
        verticalScrollBar()->value() == verticalScrollBar()->minimum())
    {
      QApplication::beep();
    }
    else
    {
      if (m_selectionType == SelectionType::downward && shiftIsHeld)
      {
        if (!m_editingHex)
          m_EndBytesSelectionPosX--;
        else
          m_EndBytesSelectionPosX -= m_sizeOfType;
        if (m_EndBytesSelectionPosX < 0)
        {
          m_EndBytesSelectionPosX += m_numColumns;
          m_EndBytesSelectionPosY--;
        }
        int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
        int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
        if (indexEnd == indexStart)
          m_selectionType = SelectionType::single;
      }
      else
      {
        if (!m_editingHex)
          m_StartBytesSelectionPosX--;
        else
          m_StartBytesSelectionPosX -= m_sizeOfType;
        if (m_StartBytesSelectionPosX < 0)
        {
          m_StartBytesSelectionPosX += m_numColumns;
          m_StartBytesSelectionPosY--;
        }
        if (!shiftIsHeld)
        {
          m_EndBytesSelectionPosX = m_StartBytesSelectionPosX;
          m_EndBytesSelectionPosY = m_StartBytesSelectionPosY;
          m_selectionType = SelectionType::single;
        }
        else
        {
          m_selectionType = SelectionType::upward;
        }
      }

      scrollToSelection();
    }
    break;
  case Qt::Key_Right:
    if (m_StartBytesSelectionPosX >= m_numColumns - 1 &&
        m_StartBytesSelectionPosY >= m_numRows - 1 &&
        verticalScrollBar()->value() == verticalScrollBar()->maximum())
    {
      QApplication::beep();
    }
    else
    {
      if (m_selectionType == SelectionType::upward && shiftIsHeld)
      {
        if (!m_editingHex)
          m_StartBytesSelectionPosX++;
        else
          m_StartBytesSelectionPosX += m_sizeOfType;
        if (m_StartBytesSelectionPosX >= m_numColumns)
        {
          m_StartBytesSelectionPosX -= m_numColumns;
          m_StartBytesSelectionPosY++;
        }
        int indexStart = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
        int indexEnd = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;
        if (indexEnd == indexStart)
          m_selectionType = SelectionType::single;
      }
      else
      {
        if (!m_editingHex)
          m_EndBytesSelectionPosX++;
        else
          m_EndBytesSelectionPosX += m_sizeOfType;
        if (m_EndBytesSelectionPosX >= m_numColumns)
        {
          m_EndBytesSelectionPosX -= m_numColumns;
          m_EndBytesSelectionPosY++;
        }
        if (!shiftIsHeld)
        {
          m_StartBytesSelectionPosX = m_EndBytesSelectionPosX;
          m_StartBytesSelectionPosY = m_EndBytesSelectionPosY;
          m_selectionType = SelectionType::single;
        }
        else
        {
          m_selectionType = SelectionType::downward;
        }
      }

      scrollToSelection();
    }
    break;
  case Qt::Key_PageUp:
    scrollContentsBy(0, std::min(verticalScrollBar()->pageStep(), verticalScrollBar()->value()));
    break;
  case Qt::Key_PageDown:
    scrollContentsBy(0, std::max(-verticalScrollBar()->pageStep(),
                                 verticalScrollBar()->value() - verticalScrollBar()->maximum()));
    break;
  case Qt::Key_Home:
    verticalScrollBar()->setValue(0);
    m_StartBytesSelectionPosX = 0;
    m_StartBytesSelectionPosY = 0;
    m_EndBytesSelectionPosX = 0;
    m_EndBytesSelectionPosY = 0;
    m_selectionType = SelectionType::single;
    viewport()->update();
    break;
  case Qt::Key_End:
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    m_StartBytesSelectionPosX = m_numColumns - 1;
    m_StartBytesSelectionPosY = m_numRows - 1;
    m_EndBytesSelectionPosX = m_numColumns - 1;
    m_EndBytesSelectionPosY = m_numRows - 1;
    m_selectionType = SelectionType::single;
    viewport()->update();
    break;
  default:
    return false;
  }

  // Always set the carrot at the start of a byte after navigating
  m_carrotIndex = 0;
  return true;
}

bool MemViewer::writeCharacterToSelectedMemory(char byteToWrite)
{
  const size_t memoryOffset =
      ((m_StartBytesSelectionPosY * m_numColumns) + m_StartBytesSelectionPosX);
  const u32 offsetToWrite{
      Common::dolphinAddrToOffset(m_currentFirstAddress + static_cast<u32>(memoryOffset),
                                  DolphinComm::DolphinAccessor::isARAMAccessible())};
  if (m_editingHex)
  {
    std::string mem_str = memToStrFormatted(m_StartBytesSelectionPosY, m_StartBytesSelectionPosX);
    if (m_type != Common::MemType::type_double && m_type != Common::MemType::type_float)
    {
      std::replace(mem_str.begin(), mem_str.end(), ' ', '0');
    }
    else
    {
      std::string::iterator letter_e = std::find(mem_str.begin(), mem_str.end(), 'e');
      std::replace(mem_str.begin(), letter_e, ' ', '0');
      if (byteToWrite == '.')
      {
        size_t index = mem_str.find('.');
        if (index != std::string::npos)
        {
          mem_str.erase(index, 1);
        }
      }
    }
    if (static_cast<size_t>(m_carrotIndex) > mem_str.size())
      mem_str.insert(mem_str.end(), m_carrotIndex - mem_str.size() + 1, '0');
    mem_str[m_carrotIndex] = byteToWrite;

    Common::MemOperationReturnCode return_code = Common::MemOperationReturnCode::OK;
    size_t len_actual = 0;
    const Common::MemBase base =
        (m_type == Common::MemType::type_double || m_type == Common::MemType::type_float) ?
            Common::MemBase::base_decimal :
            m_base;
    char* new_mem =
        Common::formatStringToMemory(return_code, len_actual, mem_str, base, m_type, m_sizeOfType,
                                     m_absoluteBranch ? static_cast<u32>(memoryOffset) : 0U);
    bool toReturn = DolphinComm::DolphinAccessor::writeToRAM(
        offsetToWrite, new_mem, len_actual, Common::shouldBeBSwappedForType(m_type));
    delete[] new_mem;
    return toReturn;
  }
  else
  {
    // Editing ASCII section
    return DolphinComm::DolphinAccessor::writeToRAM(offsetToWrite, &byteToWrite, 1, false);
  }
}

void MemViewer::keyPressEvent(QKeyEvent* event)
{
  if (!m_validMemory)
  {
    event->ignore();
    return;
  }

  QString text = event->text();

  if (text.isEmpty())
  {
    if (!handleNaviguationKey(event->key(), event->modifiers() & Qt::ShiftModifier))
      event->ignore();
    return;
  }

  bool success = false;
  if (m_editingHex)
  {
    // Beep when entering a non-valid value
    const std::string hexChar = event->text().toUpper().toStdString();
    const char value = hexChar[0];
    const bool ishex{('0' <= value && value <= '9') || ('A' <= value && value <= 'F')};
    const bool isFloatChars{('0' <= value && value <= '9') || ('-' == value) || ('.' == value)};
    const bool isdecUnsigned{('0' <= value && value <= '9') || ('-' == value)};
    const bool isdecSigned{('0' <= value && value <= '9')};
    const bool isbin{('0' <= value && value <= '1')};

    const bool isFloatOrDouble =
        m_type == Common::MemType::type_float || m_type == Common::MemType::type_double;
    const bool invalid_hex =
        !isFloatOrDouble && (m_base == Common::MemBase::base_hexadecimal && !ishex);
    const bool invalid_floatChars = isFloatOrDouble && !isFloatChars;
    const bool invalid_decUnsigned = !isFloatOrDouble && (m_base == Common::MemBase::base_decimal &&
                                                          !m_isUnsigned && !isdecUnsigned);
    const bool invalid_decSigned = !isFloatOrDouble && (m_base == Common::MemBase::base_decimal &&
                                                        m_isUnsigned && !isdecSigned);
    const bool invalid_binary =
        !isFloatOrDouble && (m_base == Common::MemBase::base_binary && !isbin);

    if (m_type == Common::MemType::type_ppc || invalid_hex || invalid_floatChars ||
        invalid_decUnsigned || invalid_decSigned || invalid_binary)
    {
      QApplication::beep();
      return;
    }

    success = writeCharacterToSelectedMemory(value);
    m_carrotIndex = (m_carrotIndex + 1) % m_digitsPerBox;
  }
  else
  {
    std::string str = text.toStdString();
    success = writeCharacterToSelectedMemory(str[0]);
  }

  if (!success)
  {
    m_validMemory = false;
    viewport()->update();
    emit memErrorOccured();
    return;
  }

  if (!m_carrotIndex || !m_editingHex)
    handleNaviguationKey(Qt::Key::Key_Right, false);

  updateMemoryData();
  viewport()->update();
}

void MemViewer::scrollContentsBy(int dx, int dy)
{
  (void)dx;

  if (!m_disableScrollContentEvent && m_validMemory)
  {
    u32 newAddress = m_currentFirstAddress + m_numColumns * (-dy);

    // Move selection
    m_StartBytesSelectionPosY += dy;
    m_EndBytesSelectionPosY += dy;

    if (newAddress < m_memViewStart)
      newAddress = m_memViewStart;
    else if (newAddress > m_memViewEnd - m_numCells)
      newAddress = m_memViewEnd - m_numCells;

    if (newAddress != m_currentFirstAddress)
      jumpToAddress(newAddress);
  }
}

void MemViewer::renderSeparatorLines(QPainter& painter) const
{
  QColor oldPenColor = painter.pen().color();
  painter.setPen(QGuiApplication::palette().color(QPalette::WindowText));

  painter.drawLine(m_hexAsciiSeparatorPosX, 0, m_hexAsciiSeparatorPosX,
                   m_columnHeaderHeight + m_hexAreaHeight);
  painter.drawLine(m_rowHeaderWidth - m_charWidthEm / 2, 0, m_rowHeaderWidth - m_charWidthEm / 2,
                   m_columnHeaderHeight + m_hexAreaHeight);
  painter.drawLine(0, m_columnHeaderHeight, m_hexAsciiSeparatorPosX + 17 * m_charWidthEm,
                   m_columnHeaderHeight);

  int divide_amt = std::max(SConfig::getInstance().getViewerNbrBytesSeparator(), m_sizeOfType);

  if (SConfig::getInstance().getViewerNbrBytesSeparator() != 0)
  {
    int bytesSeparatorXPos = m_rowHeaderWidth - m_charWidthEm / 2 + m_charWidthEm / 4;
    for (int i = 0; i < m_numColumns / divide_amt - 1; i++)
    {
      bytesSeparatorXPos += (m_charWidthEm * m_digitsPerBox) * (divide_amt / m_sizeOfType) +
                            (m_charWidthEm / 2) * (divide_amt / m_sizeOfType);
      painter.drawLine(bytesSeparatorXPos, 0, bytesSeparatorXPos,
                       m_columnHeaderHeight + m_hexAreaHeight);
    }
  }
  painter.setPen(oldPenColor);
}

void MemViewer::renderColumnsHeaderText(QPainter& painter) const
{
  QColor oldPenColor = painter.pen().color();
  painter.setPen(QGuiApplication::palette().color(QPalette::WindowText));
  painter.drawText(static_cast<int>(static_cast<double>(m_charWidthEm) * 1.5), m_charHeight,
                   tr("Address"));
  int posXHeaderText = m_rowHeaderWidth;
  for (int i = 0; i < m_numColumns; i += m_sizeOfType)
  {
    std::stringstream ss;
    const u32 byte{(m_currentFirstAddress + static_cast<u32>(i)) & 0xF};
    ss << std::hex << std::uppercase << byte;
    std::string headerText = "." + ss.str();
    painter.drawText(posXHeaderText, m_charHeight, QString::fromStdString(headerText));
    posXHeaderText += m_charWidthEm * m_digitsPerBox + m_charWidthEm / 2;
  }

  painter.drawText(m_hexAsciiSeparatorPosX +
                       static_cast<int>(static_cast<double>(m_charWidthEm) * 2.5),
                   m_charHeight, tr("Text (ASCII)"));
  painter.drawText(0, 0, 0, 0, 0, QString());
  painter.setPen(oldPenColor);
}

void MemViewer::renderRowHeaderText(QPainter& painter, const int rowIndex) const
{
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(sizeof(u32) * 2) << std::hex << std::uppercase
     << m_currentFirstAddress + m_numColumns * rowIndex;
  int x = m_charWidthEm / 2;
  int y = (rowIndex + 1) * m_charHeight + m_columnHeaderHeight;
  QColor oldPenColor = painter.pen().color();
  painter.setPen(QGuiApplication::palette().color(QPalette::WindowText));
  painter.drawText(x, y, QString::fromStdString(ss.str()));
  painter.setPen(oldPenColor);
}

void MemViewer::renderCarret(QPainter& painter, const int rowIndex, const int columnIndex)
{
  int posXHex = m_rowHeaderWidth +
                (m_charWidthEm * m_digitsPerBox + m_charWidthEm / 2) * (columnIndex / m_sizeOfType);
  QColor oldPenColor = painter.pen().color();
  int carretPosX = posXHex + (m_carrotIndex * m_charWidthEm);
  painter.setPen(QColor(Qt::red));
  painter.drawLine(carretPosX,
                   rowIndex * m_charHeight + (m_charHeight - fontMetrics().overlinePos()) +
                       m_columnHeaderHeight,
                   carretPosX,
                   rowIndex * m_charHeight + (m_charHeight - fontMetrics().overlinePos()) +
                       m_columnHeaderHeight + m_charHeight - 1);
  painter.setPen(oldPenColor);
}

void MemViewer::determineMemoryTextRenderProperties(const int rowIndex, const int columnIndex,
                                                    bool& drawCarret, QColor& bgColor,
                                                    QColor& fgColor)
{
  int currentIndex = rowIndex * m_numColumns + columnIndex;
  int selectionStartIndex = m_StartBytesSelectionPosY * m_numColumns + m_StartBytesSelectionPosX;
  int selectionEndIndex = m_EndBytesSelectionPosY * m_numColumns + m_EndBytesSelectionPosX;

  if (currentIndex >= selectionStartIndex && currentIndex <= selectionEndIndex)
  {
    bgColor = QGuiApplication::palette().color(QPalette::Highlight);
    fgColor = QGuiApplication::palette().color(QPalette::HighlightedText);
    if (selectionStartIndex == selectionEndIndex)
      drawCarret = true;
  }
  // If the byte changed since the last data update
  else if (m_lastRawMemoryData[rowIndex * m_numColumns + columnIndex] !=
           m_updatedRawMemoryData[rowIndex * m_numColumns + columnIndex])
  {
    m_memoryMsElapsedLastChange[rowIndex * m_numColumns + columnIndex] =
        static_cast<int>(m_elapsedTimer.elapsed());
    bgColor = QColor(Qt::red);
  }
  // If the last changes is less than a second old
  else if (m_memoryMsElapsedLastChange[rowIndex * m_numColumns + columnIndex] != 0 &&
           m_elapsedTimer.elapsed() -
                   m_memoryMsElapsedLastChange[rowIndex * m_numColumns + columnIndex] <
               1000)
  {
    QColor baseColor = QColor(Qt::red);
    float alphaPercentage =
        (1000.0f -
         static_cast<float>(m_elapsedTimer.elapsed() -
                            m_memoryMsElapsedLastChange[rowIndex * m_numColumns + columnIndex])) /
        (1000.0f / 100.0f);
    const int newAlpha{
        static_cast<int>(static_cast<float>(baseColor.alpha()) * alphaPercentage / 100.0f)};
    bgColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), newAlpha);
  }
}

std::string MemViewer::memToStrFormatted(const int rowIndex, const int columnIndex) const
{
  const bool is_double = m_type == Common::MemType::type_double;
  const bool is_float = m_type == Common::MemType::type_float;
  const bool is_ppc = m_type == Common::MemType::type_ppc;
  const bool isUnsigned = m_base == Common::MemBase::base_decimal ? m_isUnsigned : true;
  const Common::MemBase base =
      (is_ppc || is_float || is_double) ? Common::MemBase::base_decimal : m_base;
  const u32 address = m_currentFirstAddress + ((rowIndex * m_numColumns) + columnIndex);

  std::string toReturn = Common::formatMemoryToString(
      m_updatedRawMemoryData + ((rowIndex * m_numColumns) + columnIndex), m_type,
      Common::getSizeForType(m_type, 1), base, isUnsigned, Common::shouldBeBSwappedForType(m_type),
      m_absoluteBranch ? address : 0U);

  if (base != Common::MemBase::base_decimal &&
      toReturn.size() < static_cast<size_t>(m_digitsPerBox))
    toReturn.insert(0, m_digitsPerBox - toReturn.size(), '0');

  if (is_float || is_double)
  {
    if (toReturn.find(".") == std::string::npos)
      toReturn += ".00";

    // give space to type a new value 10 times larger
    if (toReturn[0] == '-')
      toReturn = "- " + toReturn.substr(1);
    else
      toReturn = " " + toReturn;
  }
  else if (m_type != Common::MemType::type_ppc && base == Common::MemBase::base_decimal &&
           toReturn.size() < static_cast<size_t>(m_digitsPerBox))
  {
    // pad decimal values to be right aligned (don't do if float or double)
    bool starting_index = (toReturn.find('-') != (size_t)-1);
    toReturn.insert(starting_index, m_digitsPerBox - toReturn.size(), ' ');
  }

  if (toReturn.length() > static_cast<size_t>(m_digitsPerBox))
    toReturn = toReturn.substr(0, m_digitsPerBox);

  return toReturn;
}

void MemViewer::renderHexByte(QPainter& painter, const int rowIndex, const int columnIndex,
                              QColor& bgColor, QColor& fgColor, bool drawCarret)
{
  int posXHex = m_rowHeaderWidth +
                (m_charWidthEm * m_digitsPerBox + m_charWidthEm / 2) * (columnIndex / m_sizeOfType);
  std::string hexByte = memToStrFormatted(rowIndex, columnIndex);
  QRect* currentByteRect = new QRect(posXHex,
                                     m_columnHeaderHeight + rowIndex * m_charHeight +
                                         (m_charHeight - fontMetrics().overlinePos()),
                                     m_charWidthEm * m_digitsPerBox, m_charHeight);
  painter.fillRect(*currentByteRect, bgColor);
  if (drawCarret)
    renderCarret(painter, rowIndex, columnIndex);

  painter.setPen(fgColor);
  painter.drawText(posXHex, (rowIndex + 1) * m_charHeight + m_columnHeaderHeight,
                   QString::fromStdString(hexByte));
  delete currentByteRect;
}

void MemViewer::renderASCIIText(QPainter& painter, const int rowIndex, const int columnIndex,
                                QColor& bgColor, QColor& fgColor)
{
  std::string asciiStr = Common::formatMemoryToString(
      m_updatedRawMemoryData + ((rowIndex * m_numColumns) + columnIndex),
      Common::MemType::type_string, 1, Common::MemBase::base_none, true);
  const int asciiByte{static_cast<int>(static_cast<unsigned char>(asciiStr[0]))};
  if (asciiByte > 0x7E || asciiByte < 0x20)
    asciiStr = ".";
  QRect* currentCharRect = new QRect(
      (columnIndex * m_charWidthEm) + m_hexAsciiSeparatorPosX + m_charWidthEm / 2,
      rowIndex * m_charHeight + (m_charHeight - fontMetrics().overlinePos()) + m_columnHeaderHeight,
      m_charWidthEm, m_charHeight);
  painter.fillRect(*currentCharRect, bgColor);
  painter.setPen(fgColor);
  painter.drawText((columnIndex * m_charWidthEm) + m_hexAsciiSeparatorPosX + m_charWidthEm / 2,
                   (rowIndex + 1) * m_charHeight + m_columnHeaderHeight,
                   QString::fromStdString(asciiStr));
  delete currentCharRect;
}

void MemViewer::renderMemory(QPainter& painter, const int rowIndex, const int columnIndex)
{
  QColor oldPenColor = painter.pen().color();
  QColor fgColor = QGuiApplication::palette().color(QPalette::WindowText);
  int posXHex =
      m_rowHeaderWidth + (m_charWidthEm * m_digitsPerBox + m_charWidthEm / 2) * columnIndex;
  const bool validRange{
      m_currentFirstAddress + (m_numColumns * rowIndex + columnIndex) >= m_memViewStart &&
      m_currentFirstAddress + (m_numColumns * rowIndex + columnIndex) < m_memViewEnd};
  if (!validRange ||
      !DolphinComm::DolphinAccessor::isValidConsoleAddress(
          m_currentFirstAddress + (m_numColumns * rowIndex + columnIndex)) ||
      !m_validMemory)
  {
    painter.setPen(fgColor);
    painter.drawText(posXHex, (rowIndex + 1) * m_charHeight + m_columnHeaderHeight, "??");
    painter.drawText((columnIndex * m_charWidthEm) + m_hexAsciiSeparatorPosX + m_charWidthEm / 2,
                     (rowIndex + 1) * m_charHeight + m_columnHeaderHeight, "?");
  }
  else
  {
    QColor bgColor = QColor(Qt::transparent);
    bool drawCarret = false;

    determineMemoryTextRenderProperties(rowIndex, columnIndex, drawCarret, bgColor, fgColor);

    renderASCIIText(painter, rowIndex, columnIndex, bgColor, fgColor);
    if (columnIndex % m_sizeOfType == 0)
    {
      QColor bgColor_cur = QColor(Qt::transparent);
      QColor fgColor_cur = QGuiApplication::palette().color(QPalette::WindowText);
      bool drawCarrot_cur = false;

      // Check all bytes in larger data types to see what colors it should be
      for (int i = 0; i < m_sizeOfType; i++)
      {
        determineMemoryTextRenderProperties(rowIndex, columnIndex + i, drawCarrot_cur, bgColor_cur,
                                            fgColor_cur);
        drawCarret = drawCarrot_cur | drawCarret;
        if (bgColor_cur.rgb() ==
            QColor(QGuiApplication::palette().color(QPalette::Highlight)).rgb())
        {
          bgColor = bgColor_cur;
          fgColor = fgColor_cur;
          break;
        }
        if (bgColor_cur.rgb() == QColor(Qt::red).rgb())
        {
          bgColor = bgColor_cur;
          fgColor = fgColor_cur;
        }
      }
      renderHexByte(painter, rowIndex, columnIndex, bgColor, fgColor, drawCarret);
    }
  }
  painter.setPen(oldPenColor);
}

void MemViewer::paintEvent(QPaintEvent* event)
{
  (void)event;

  QPainter painter(viewport());
  painter.setFont(font());
  painter.setPen(QColor(Qt::black));

  renderSeparatorLines(painter);
  renderColumnsHeaderText(painter);

  for (int i = 0; i < m_numRows; ++i)
  {
    renderRowHeaderText(painter, i);
    for (int j = 0; j < m_numColumns; ++j)
      renderMemory(painter, i, j);
  }
}
