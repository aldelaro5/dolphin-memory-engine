#include "MemViewer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

#include "../../Common/CommonUtils.h"
#include "../../DolphinProcess/DolphinAccessor.h"

MemViewer::MemViewer(QWidget* parent) : QAbstractScrollArea(parent)
{
#ifdef __linux__
  setFont(QFont("Monospace", 15));
#elif _WIN32
  setFont(QFont("Courier New", 15));
#endif
  m_hexKeyList = QList<int>({Qt::Key_0, Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5,
                             Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_A, Qt::Key_B,
                             Qt::Key_C, Qt::Key_D, Qt::Key_E, Qt::Key_F});
  m_charWidthEm = fontMetrics().width(QLatin1Char('M'));
  m_charHeight = fontMetrics().height();
  m_hexAreaWidth = 16 * (m_charWidthEm * 2 + m_charWidthEm / 2);
  m_hexAreaHeight = 16 * m_charHeight;
  m_rowHeaderWidth = m_charWidthEm * (sizeof(u32) * 2 + 1) + m_charWidthEm / 2;
  m_hexAsciiSeparatorPosX = m_rowHeaderWidth + m_hexAreaWidth;
  m_columnHeaderHeight = m_charHeight + m_charWidthEm / 2;
  m_curosrRect = new QRect();
  m_updatedRawMemoryData = new char[16 * 16];
  m_lastRawMemoryData = new char[16 * 16];
  m_memoryMsElapsedLastChange = new int[16 * 16];
  m_memViewStart = Common::MEM1_START;
  m_memViewEnd = Common::MEM1_END;
  m_currentFirstAddress = m_memViewStart;
  std::fill(m_memoryMsElapsedLastChange, m_memoryMsElapsedLastChange + 16 * 16, 0);
  updateMemoryData();
  std::memcpy(m_lastRawMemoryData, m_updatedRawMemoryData, 16 * 16);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  verticalScrollBar()->setRange(0, ((m_memViewEnd - m_memViewStart) / 16) - 1);
  verticalScrollBar()->setPageStep(16);

  m_elapsedTimer.start();

  // The viewport is implicitly updated at the constructor's end
}

MemViewer::~MemViewer()
{
  delete[] m_updatedRawMemoryData;
  delete[] m_lastRawMemoryData;
  delete[] m_memoryMsElapsedLastChange;
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
                                                         m_currentFirstAddress, 16 * 16);
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

    if (m_currentFirstAddress > m_memViewEnd - 16 * 16)
      m_currentFirstAddress = m_memViewEnd - 16 * 16;
    std::fill(m_memoryMsElapsedLastChange, m_memoryMsElapsedLastChange + 16 * 16, 0);
    updateMemoryData();
    std::memcpy(m_lastRawMemoryData, m_updatedRawMemoryData, 16 * 16);
    m_carretBetweenHex = false;

    m_disableScrollContentEvent = true;
    verticalScrollBar()->setValue(((address & 0xFFFFFFF0) - m_memViewStart) / 16);
    m_disableScrollContentEvent = false;

    viewport()->update();
  }
}

void MemViewer::changeMemoryRegion(const bool isMEM2)
{
  m_memViewStart = isMEM2 ? Common::MEM2_START : Common::MEM1_START;
  m_memViewEnd = isMEM2 ? Common::MEM2_END : Common::MEM1_END;
  verticalScrollBar()->setRange(0, ((m_memViewEnd - m_memViewStart) / 16) - 1);
}

void MemViewer::mousePressEvent(QMouseEvent* event)
{
  int x = event->pos().x();
  int y = event->pos().y();
  if (x >= m_rowHeaderWidth && x <= m_rowHeaderWidth + m_hexAreaWidth && y >= m_columnHeaderHeight)
  {
    int oldSelectedPosX = m_byteSelectedPosX;
    int oldSelectedPosY = m_byteSelectedPosY;
    m_byteSelectedPosX = (x - m_rowHeaderWidth) / (m_charWidthEm / 2 + m_charWidthEm * 2);
    m_byteSelectedPosY = (y - m_columnHeaderHeight) / m_charHeight;
    if (oldSelectedPosX == m_byteSelectedPosX && oldSelectedPosY == m_byteSelectedPosY)
      m_carretBetweenHex = !m_carretBetweenHex;
    else
      m_carretBetweenHex = false;
    viewport()->update();
  }
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
    return true;
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
    return true;
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
    else
    {
      m_byteSelectedPosX = 15;
      m_byteSelectedPosY = 15;
      scrollContentsBy(0, 1);
    }
    return true;
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
    else
    {
      m_byteSelectedPosX = 0;
      m_byteSelectedPosY = 0;
      scrollContentsBy(0, -1);
    }
    return true;
  case Qt::Key_PageUp:
    scrollContentsBy(0, verticalScrollBar()->pageStep());
    return true;
  case Qt::Key_PageDown:
    scrollContentsBy(0, -verticalScrollBar()->pageStep());
    return true;
  case Qt::Key_Home:
    verticalScrollBar()->setValue(0);
    return true;
  case Qt::Key_End:
    verticalScrollBar()->setValue((m_memViewEnd - m_memViewStart) / 16 - 1);
    return true;
  default:
    return false;
  }
}

bool MemViewer::writeHexCharacterToSelectedMemory(const std::string hexCharToWrite)
{
  std::string editedByteStr = Common::formatMemoryToString(
      m_updatedRawMemoryData + ((m_byteSelectedPosY * 16) + m_byteSelectedPosX),
      Common::MemType::type_byteArray, 1, Common::MemBase::base_none, true);
  if (m_carretBetweenHex)
    editedByteStr[1] = hexCharToWrite[0];
  else
    editedByteStr[0] = hexCharToWrite[0];

  std::stringstream ss(editedByteStr);
  unsigned int editedByteData = 0;
  ss >> std::hex >> editedByteData;
  char byteToWrite = static_cast<char>(editedByteData);
  u32 offsetToWrite = Common::dolphinAddrToOffset(m_currentFirstAddress +
                                                  (m_byteSelectedPosY * 16 + m_byteSelectedPosX));
  return DolphinComm::DolphinAccessor::writeToRAM(offsetToWrite, &byteToWrite, 1, false);
}

void MemViewer::keyPressEvent(QKeyEvent* event)
{
  if (m_validMemory)
  {
    if (!m_hexKeyList.contains(event->key()))
    {
      if (!handleNaviguationKey(event->key()))
        event->ignore();
    }
    else
    {
      if (!writeHexCharacterToSelectedMemory(event->text().toUpper().toStdString()))
      {
        m_validMemory = false;
        viewport()->update();
        emit memErrorOccured();
      }
      else
      {
        if (!m_carretBetweenHex)
        {
          m_carretBetweenHex = true;
        }
        else
        {
          m_carretBetweenHex = false;
          if (m_byteSelectedPosX == 15)
          {
            if (m_byteSelectedPosY != 15)
            {
              m_byteSelectedPosX = 0;
              m_byteSelectedPosY++;
            }
          }
          else
          {
            m_byteSelectedPosX++;
          }
        }
        updateMemoryData();
        viewport()->update();
      }
    }
  }
  else
  {
    event->ignore();
  }
}

void MemViewer::scrollContentsBy(int dx, int dy)
{
  if (!m_disableScrollContentEvent && m_validMemory)
  {
    u32 newAddress = m_currentFirstAddress + 16 * (-dy);
    if (newAddress < m_memViewStart)
      newAddress = m_memViewStart;
    else if (newAddress > m_memViewEnd - 16 * 16)
      newAddress = m_memViewEnd - 16 * 16;

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
  int i = m_currentFirstAddress & 0xF;
  do
  {
    if (i >= 16)
    {
      i = 0;
    }
    else
    {
      std::stringstream ss;
      ss << std::hex << std::uppercase << i;
      std::string headerText = "." + ss.str();
      painter.drawText(posXHeaderText, m_charHeight, QString::fromStdString(headerText));
      posXHeaderText += m_charWidthEm * 2 + m_charWidthEm / 2;
      i++;
    }
  } while (i != (m_currentFirstAddress & 0xF));

  painter.drawText(m_hexAsciiSeparatorPosX + m_charWidthEm / 2, m_charHeight, "  Text (ASCII)  ");
}

void MemViewer::renderRowHeaderText(QPainter& painter, const int rowIndex)
{
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(sizeof(u32) * 2) << std::hex << std::uppercase
     << m_currentFirstAddress + 16 * rowIndex;
  painter.drawText(m_charWidthEm / 2, (rowIndex + 1) * m_charHeight + m_columnHeaderHeight,
                   QString::fromStdString(ss.str()));
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
  else if (m_lastRawMemoryData[rowIndex * 16 + columnIndex] !=
           m_updatedRawMemoryData[rowIndex * 16 + columnIndex])
  {
    m_memoryMsElapsedLastChange[rowIndex * 16 + columnIndex] = m_elapsedTimer.elapsed();
    bgColor = QColor(Qt::red);
  }
  // If the last changes is less than a second old
  else if (m_memoryMsElapsedLastChange[rowIndex * 16 + columnIndex] != 0 &&
           m_elapsedTimer.elapsed() - m_memoryMsElapsedLastChange[rowIndex * 16 + columnIndex] <
               1000)
  {
    QColor baseColor = QColor(Qt::red);
    float alphaPercentage = (1000 - (m_elapsedTimer.elapsed() -
                                     m_memoryMsElapsedLastChange[rowIndex * 16 + columnIndex])) /
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
      m_updatedRawMemoryData + ((rowIndex * 16) + columnIndex), Common::MemType::type_byteArray, 1,
      Common::MemBase::base_none, true);
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
      m_updatedRawMemoryData + ((rowIndex * 16) + columnIndex), Common::MemType::type_string, 1,
      Common::MemBase::base_none, true);
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
  if (!(m_currentFirstAddress + (16 * rowIndex + columnIndex) >= m_memViewStart &&
        m_currentFirstAddress + (16 * rowIndex + columnIndex) < m_memViewEnd) ||
      !DolphinComm::DolphinAccessor::isValidConsoleAddress(m_currentFirstAddress +
                                                           (16 * rowIndex + columnIndex)) ||
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

  for (int i = 0; i < 16; ++i)
  {
    renderRowHeaderText(painter, i);
    for (int j = 0; j < 16; ++j)
    {
      renderMemory(painter, i, j);
    }
  }
}