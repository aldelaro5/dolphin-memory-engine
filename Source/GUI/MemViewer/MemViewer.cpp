#include "MemViewer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

#include "../../Common/CommonUtils.h"
#include "../../DolphinProcess/DolphinAccessor.h"

#define VISIBLE_COLS 16 // Should be a multiple of 16, or the header doesn't make sense
#define VISIBLE_ROWS 16
#define NUM_BYTES (VISIBLE_COLS * VISIBLE_ROWS)

MemViewer::MemViewer(QWidget* parent) : QAbstractScrollArea(parent)
{
  initialise();

  std::fill(m_memoryMsElapsedLastChange, m_memoryMsElapsedLastChange + NUM_BYTES, 0);
  updateMemoryData();
  std::memcpy(m_lastRawMemoryData, m_updatedRawMemoryData, NUM_BYTES);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  changeMemoryRegion(false);
  verticalScrollBar()->setPageStep(VISIBLE_ROWS);

  m_elapsedTimer.start();

  // The viewport is implicitly updated at the constructor's end
}

MemViewer::~MemViewer()
{
  delete[] m_updatedRawMemoryData;
  delete[] m_lastRawMemoryData;
  delete[] m_memoryMsElapsedLastChange;
}

void MemViewer::initialise()
{
  updateFontSize(m_memoryFontSize);
  m_curosrRect = new QRect();
  m_updatedRawMemoryData = new char[NUM_BYTES];
  m_lastRawMemoryData = new char[NUM_BYTES];
  m_memoryMsElapsedLastChange = new int[NUM_BYTES];
  m_memViewStart = Common::MEM1_START;
  m_memViewEnd = Common::MEM1_END;
  m_currentFirstAddress = m_memViewStart;
}

QSize MemViewer::sizeHint() const
{
  return QSize(m_rowHeaderWidth + m_hexAreaWidth + 17 * m_charWidthEm +
                   verticalScrollBar()->width(),
               m_columnHeaderHeight + m_hexAreaHeight + m_charHeight / 2);
}

void MemViewer::memoryValidityChanged(const bool valid)
{
  m_validMemory = valid;
  viewport()->update();
}

void MemViewer::updateMemoryData()
{
  std::swap(m_updatedRawMemoryData, m_lastRawMemoryData);
  m_validMemory =
      (DolphinComm::DolphinAccessor::updateRAMCache() == Common::MemOperationReturnCode::OK);
  if (DolphinComm::DolphinAccessor::isValidConsoleAddress(m_currentFirstAddress))
    DolphinComm::DolphinAccessor::copyRawMemoryFromCache(m_updatedRawMemoryData,
                                                         m_currentFirstAddress, NUM_BYTES);
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
    if (address < Common::MEM1_END && m_memViewStart != Common::MEM1_START)
      changeMemoryRegion(false);
    else if (address >= Common::MEM2_START && m_memViewStart != Common::MEM2_START)
      changeMemoryRegion(true);

    if (m_currentFirstAddress > m_memViewEnd - NUM_BYTES)
      m_currentFirstAddress = m_memViewEnd - NUM_BYTES;
    std::fill(m_memoryMsElapsedLastChange, m_memoryMsElapsedLastChange + NUM_BYTES, 0);
    updateMemoryData();
    std::memcpy(m_lastRawMemoryData, m_updatedRawMemoryData, NUM_BYTES);
    m_carretBetweenHex = false;

    m_disableScrollContentEvent = true;
    verticalScrollBar()->setValue(((address & 0xFFFFFFF0) - m_memViewStart) / VISIBLE_COLS);
    m_disableScrollContentEvent = false;

    viewport()->update();
  }
}

void MemViewer::changeMemoryRegion(const bool isMEM2)
{
  m_memViewStart = isMEM2 ? Common::MEM2_START : Common::MEM1_START;
  m_memViewEnd = isMEM2 ? Common::MEM2_END : Common::MEM1_END;
  verticalScrollBar()->setRange(0, ((m_memViewEnd - m_memViewStart) / VISIBLE_COLS) - VISIBLE_ROWS);
}

void MemViewer::mousePressEvent(QMouseEvent* event)
{
  // Only handle left-click events
  if (event->button() != Qt::MouseButton::LeftButton)
    return;

  int x = event->pos().x();
  int y = event->pos().y();

  const bool wasEditingHex = m_editingHex;
  const int spacing = m_charWidthEm / 2;
  int clickedHexPosX = (x - m_rowHeaderWidth) / (spacing + m_charWidthEm * 2);
  int clickedAsciiPosX = (x - m_hexAsciiSeparatorPosX - spacing) / m_charWidthEm;
  int clickedPosY = (y - m_columnHeaderHeight) / m_charHeight;

  // Ignore clicking the header
  if (clickedPosY < 0)
    return;

  if (clickedHexPosX >= 0 && clickedHexPosX < VISIBLE_COLS)
  {
    x = clickedHexPosX;
    m_editingHex = true;
  }
  else if (clickedAsciiPosX >= 0 && clickedAsciiPosX < VISIBLE_COLS)
  {
    x = clickedAsciiPosX;
    m_editingHex = false;
  }
  else
    return;

  // Toggle carrot-between-hex when the same byte is clicked twice from the hex table
  m_carretBetweenHex = (m_editingHex && wasEditingHex && !m_carretBetweenHex &&
                        m_byteSelectedPosX == x && m_byteSelectedPosY == clickedPosY);

  m_byteSelectedPosX = x;
  m_byteSelectedPosY = clickedPosY;

  viewport()->update();
}

void MemViewer::wheelEvent(QWheelEvent* event)
{
  if (event->modifiers().testFlag(Qt::ControlModifier))
  {
    if (event->delta() < 0 && m_memoryFontSize > 5)
      updateFontSize(m_memoryFontSize - 1);
    else if (event->delta() > 0)
      updateFontSize(m_memoryFontSize + 1);

    viewport()->update();
  }
  else
  {
    QAbstractScrollArea::wheelEvent(event);
  }
}

void MemViewer::updateFontSize(int newSize)
{
  m_memoryFontSize = newSize;

#ifdef __linux__
  setFont(QFont("Monospace", m_memoryFontSize));
#elif _WIN32
  setFont(QFont("Courier New", m_memoryFontSize));
#endif

  m_charWidthEm = fontMetrics().width(QLatin1Char('M'));
  m_charHeight = fontMetrics().height();
  m_hexAreaWidth = VISIBLE_COLS * (m_charWidthEm * 2 + m_charWidthEm / 2);
  m_hexAreaHeight = VISIBLE_ROWS * m_charHeight;
  m_rowHeaderWidth = m_charWidthEm * (sizeof(u32) * 2 + 1) + m_charWidthEm / 2;
  m_hexAsciiSeparatorPosX = m_rowHeaderWidth + m_hexAreaWidth;
  m_columnHeaderHeight = m_charHeight + m_charWidthEm / 2;
}

bool MemViewer::handleNaviguationKey(const int key)
{
  switch (key)
  {
  case Qt::Key_Up:
    if (m_byteSelectedPosY != 0)
    {
      m_byteSelectedPosY--;
      viewport()->update();
    }
    else
    {
      scrollContentsBy(0, 1);
    }
    break;
  case Qt::Key_Down:
    if (m_byteSelectedPosY != 15)
    {
      m_byteSelectedPosY++;
      viewport()->update();
    }
    else
    {
      scrollContentsBy(0, -1);
    }
    break;
  case Qt::Key_Left:
    if (m_byteSelectedPosX != 0)
    {
      m_byteSelectedPosX--;
      viewport()->update();
    }
    else if (m_byteSelectedPosY != 0)
    {
      m_byteSelectedPosX = 15;
      m_byteSelectedPosY--;
      viewport()->update();
    }
    else if (verticalScrollBar()->value() > verticalScrollBar()->minimum())
    {
      m_byteSelectedPosX = 15;
      m_byteSelectedPosY = 0;
      scrollContentsBy(0, 1);
    }
    break;
  case Qt::Key_Right:
    if (m_byteSelectedPosX != 15)
    {
      m_byteSelectedPosX++;
      viewport()->update();
    }
    else if (m_byteSelectedPosY != 15)
    {
      m_byteSelectedPosX = 0;
      m_byteSelectedPosY++;
      viewport()->update();
    }
    else if (verticalScrollBar()->value() < verticalScrollBar()->maximum())
    {
      scrollContentsBy(0, -1);
      m_byteSelectedPosX = 0;
      m_byteSelectedPosY = 15;
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
    m_byteSelectedPosX = 0;
    m_byteSelectedPosY = 0;
    break;
  case Qt::Key_End:
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    m_byteSelectedPosX = 15;
    m_byteSelectedPosY = 15;
    break;
  default:
    return false;
  }

  // Always set the carrot at the start of a byte after navigating
  m_carretBetweenHex = false;
  return true;
}

bool MemViewer::writeCharacterToSelectedMemory(char byteToWrite)
{
  const size_t memoryOffset = ((m_byteSelectedPosY * VISIBLE_COLS) + m_byteSelectedPosX);
  if (m_editingHex)
  {
    // Convert ascii to actual value
    if (byteToWrite >= 'A')
      byteToWrite -= 'A' - 10;
    else if (byteToWrite >= '0')
      byteToWrite -= '0';

    const char selectedMemoryValue = *(m_updatedRawMemoryData + memoryOffset);
    if (m_carretBetweenHex)
      byteToWrite = selectedMemoryValue & 0xF0 | byteToWrite;
    else
      byteToWrite = selectedMemoryValue & 0x0F | (byteToWrite << 4);
  }

  u32 offsetToWrite = Common::dolphinAddrToOffset(m_currentFirstAddress + (u32)memoryOffset);
  return DolphinComm::DolphinAccessor::writeToRAM(offsetToWrite, &byteToWrite, 1, false);
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
    if (!handleNaviguationKey(event->key()))
      event->ignore();
    return;
  }

  bool success = false;
  if (m_editingHex)
  {
    // Beep when entering a non-valid value
    const std::string hexChar = event->text().toUpper().toStdString();
    const char value = hexChar[0];
    if (!(value >= '0' && value <= '9') && !(value >= 'A' && value <= 'F'))
    {
      QApplication::beep();
      return;
    }

    success = writeCharacterToSelectedMemory(value);
    m_carretBetweenHex = !m_carretBetweenHex;
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

  if (!m_carretBetweenHex || !m_editingHex)
    handleNaviguationKey(Qt::Key::Key_Right);

  updateMemoryData();
  viewport()->update();
}

void MemViewer::scrollContentsBy(int dx, int dy)
{
  if (!m_disableScrollContentEvent && m_validMemory)
  {
    u32 newAddress = m_currentFirstAddress + VISIBLE_COLS * (-dy);

    // Move selection
    m_byteSelectedPosY += dy;

    if (newAddress < m_memViewStart)
      newAddress = m_memViewStart;
    else if (newAddress > m_memViewEnd - NUM_BYTES)
      newAddress = m_memViewEnd - NUM_BYTES;

    if (newAddress != m_currentFirstAddress)
    {
      jumpToAddress(newAddress);
    }
  }
}

void MemViewer::renderSeparatorLines(QPainter& painter)
{
  painter.drawLine(m_hexAsciiSeparatorPosX, 0, m_hexAsciiSeparatorPosX,
                   m_columnHeaderHeight + m_hexAreaHeight);
  painter.drawLine(m_rowHeaderWidth - m_charWidthEm / 2, 0, m_rowHeaderWidth - m_charWidthEm / 2,
                   m_columnHeaderHeight + m_hexAreaHeight);
  painter.drawLine(0, m_columnHeaderHeight, m_hexAsciiSeparatorPosX + 17 * m_charWidthEm,
                   m_columnHeaderHeight);
}

void MemViewer::renderColumnsHeaderText(QPainter& painter)
{
  painter.drawText(m_charWidthEm / 2, m_charHeight, " Address");
  int posXHeaderText = m_rowHeaderWidth;
  for (int i = 0; i < VISIBLE_COLS; i++)
  {
    std::stringstream ss;
    int byte = (m_currentFirstAddress + i) & 0xF;
    ss << std::hex << std::uppercase << byte;
    std::string headerText = "." + ss.str();
    painter.drawText(posXHeaderText, m_charHeight, QString::fromStdString(headerText));
    posXHeaderText += m_charWidthEm * 2 + m_charWidthEm / 2;
  }

  painter.drawText(m_hexAsciiSeparatorPosX + m_charWidthEm / 2, m_charHeight, "  Text (ASCII)  ");
}

void MemViewer::renderRowHeaderText(QPainter& painter, const int rowIndex)
{
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(sizeof(u32) * 2) << std::hex << std::uppercase
     << m_currentFirstAddress + VISIBLE_COLS * rowIndex;
  int x = m_charWidthEm / 2;
  int y = (rowIndex + 1) * m_charHeight + m_columnHeaderHeight;
  painter.drawText(x, y, QString::fromStdString(ss.str()));
}

void MemViewer::renderCarret(QPainter& painter, const int rowIndex, const int columnIndex)
{
  int posXHex = m_rowHeaderWidth + (m_charWidthEm * 2 + m_charWidthEm / 2) * columnIndex;
  QColor oldPenColor = painter.pen().color();
  int carretPosX = posXHex + (m_carretBetweenHex ? m_charWidthEm : 0);
  painter.setPen(QColor(Qt::red));
  painter.drawLine(carretPosX,
                   rowIndex * m_charHeight + (m_charHeight - fontMetrics().overlinePos()) +
                       m_columnHeaderHeight,
                   carretPosX,
                   rowIndex * m_charHeight + (m_charHeight - fontMetrics().overlinePos()) +
                       m_columnHeaderHeight + m_charHeight);
  painter.setPen(oldPenColor);
}

void MemViewer::determineMemoryTextRenderProperties(const int rowIndex, const int columnIndex,
                                                    bool& drawCarret, QColor& bgColor,
                                                    QColor& fgColor)
{
  if (rowIndex == m_byteSelectedPosY && columnIndex == m_byteSelectedPosX)
  {
    bgColor = QColor(Qt::darkBlue);
    fgColor = QColor(Qt::white);
    drawCarret = true;
  }
  // If the byte changed since the last data update
  else if (m_lastRawMemoryData[rowIndex * VISIBLE_COLS + columnIndex] !=
           m_updatedRawMemoryData[rowIndex * VISIBLE_COLS + columnIndex])
  {
    m_memoryMsElapsedLastChange[rowIndex * VISIBLE_COLS + columnIndex] = m_elapsedTimer.elapsed();
    bgColor = QColor(Qt::red);
  }
  // If the last changes is less than a second old
  else if (m_memoryMsElapsedLastChange[rowIndex * VISIBLE_COLS + columnIndex] != 0 &&
           m_elapsedTimer.elapsed() -
                   m_memoryMsElapsedLastChange[rowIndex * VISIBLE_COLS + columnIndex] <
               1000)
  {
    QColor baseColor = QColor(Qt::red);
    float alphaPercentage =
        (1000 - (m_elapsedTimer.elapsed() -
                 m_memoryMsElapsedLastChange[rowIndex * VISIBLE_COLS + columnIndex])) /
        (1000 / 100);
    int newAlpha = std::trunc(baseColor.alpha() * (alphaPercentage / 100));
    bgColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), newAlpha);
  }
}

void MemViewer::renderHexByte(QPainter& painter, const int rowIndex, const int columnIndex,
                              QColor& bgColor, QColor& fgColor, bool drawCarret)
{
  int posXHex = m_rowHeaderWidth + (m_charWidthEm * 2 + m_charWidthEm / 2) * columnIndex;
  std::string hexByte = Common::formatMemoryToString(
      m_updatedRawMemoryData + ((rowIndex * VISIBLE_COLS) + columnIndex),
      Common::MemType::type_byteArray, 1, Common::MemBase::base_none, true);
  QRect* currentByteRect = new QRect(posXHex,
                                     m_columnHeaderHeight + rowIndex * m_charHeight +
                                         (m_charHeight - fontMetrics().overlinePos()),
                                     m_charWidthEm * 2, m_charHeight);

  painter.fillRect(*currentByteRect, bgColor);
  if (drawCarret)
  {
    renderCarret(painter, rowIndex, columnIndex);
  }
  painter.setPen(fgColor);
  painter.drawText(posXHex, (rowIndex + 1) * m_charHeight + m_columnHeaderHeight,
                   QString::fromStdString(hexByte));
}

void MemViewer::renderASCIIText(QPainter& painter, const int rowIndex, const int columnIndex,
                                QColor& bgColor, QColor& fgColor)
{
  std::string asciiStr = Common::formatMemoryToString(
      m_updatedRawMemoryData + ((rowIndex * VISIBLE_COLS) + columnIndex),
      Common::MemType::type_string, 1, Common::MemBase::base_none, true);
  int asciiByte = (int)asciiStr[0];
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
}

void MemViewer::renderMemory(QPainter& painter, const int rowIndex, const int columnIndex)
{
  QColor oldPenColor = painter.pen().color();
  int posXHex = m_rowHeaderWidth + (m_charWidthEm * 2 + m_charWidthEm / 2) * columnIndex;
  if (!(m_currentFirstAddress + (VISIBLE_COLS * rowIndex + columnIndex) >= m_memViewStart &&
        m_currentFirstAddress + (VISIBLE_COLS * rowIndex + columnIndex) < m_memViewEnd) ||
      !DolphinComm::DolphinAccessor::isValidConsoleAddress(
          m_currentFirstAddress + (VISIBLE_COLS * rowIndex + columnIndex)) ||
      !m_validMemory)
  {
    painter.drawText(posXHex, (rowIndex + 1) * m_charHeight + m_columnHeaderHeight, "??");
    painter.drawText((columnIndex * m_charWidthEm) + m_hexAsciiSeparatorPosX + m_charWidthEm / 2,
                     (rowIndex + 1) * m_charHeight + m_columnHeaderHeight, "?");
  }
  else
  {
    QColor bgColor = QColor(Qt::transparent);
    QColor fgColor = QColor(Qt::black);
    bool drawCarret = false;

    determineMemoryTextRenderProperties(rowIndex, columnIndex, drawCarret, bgColor, fgColor);

    renderHexByte(painter, rowIndex, columnIndex, bgColor, fgColor, drawCarret);
    renderASCIIText(painter, rowIndex, columnIndex, bgColor, fgColor);
    painter.setPen(oldPenColor);
  }
}

void MemViewer::paintEvent(QPaintEvent* event)
{
  QPainter painter(viewport());
  painter.setPen(QColor(Qt::black));

  renderSeparatorLines(painter);
  renderColumnsHeaderText(painter);

  for (int i = 0; i < VISIBLE_ROWS; ++i)
  {
    renderRowHeaderText(painter, i);
    for (int j = 0; j < VISIBLE_COLS; ++j)
    {
      renderMemory(painter, i, j);
    }
  }
}
