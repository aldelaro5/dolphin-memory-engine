#pragma once

#include <QAbstractScrollArea>
#include <QColor>
#include <QElapsedTimer>
#include <QList>
#include <QRect>
#include <QTimer>
#include <QWidget>

#include "../../Common/CommonTypes.h"

class MemViewer : public QAbstractScrollArea
{
  Q_OBJECT

public:
  MemViewer(QWidget* parent);
  ~MemViewer();
  QSize sizeHint() const override;
  void mousePressEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void scrollContentsBy(int dx, int dy) override;
  u32 getCurrentFirstAddress() const;
  void jumpToAddress(const u32 address);
  void updateViewer();
  void memoryValidityChanged(const bool valid);

signals:
  void memErrorOccured();

private:
  bool handleNaviguationKey(const int key);
  bool writeHexCharacterToSelectedMemory(const std::string hexCharToWrite);
  void updateMemoryData();
  void changeMemoryRegion(const bool isMEM2);
  void renderColumnsHeaderText(QPainter& painter);
  void renderRowHeaderText(QPainter& painter, const int rowIndex);
  void renderSeparatorLines(QPainter& painter);
  void renderMemory(QPainter& painter, const int rowIndex, const int columnIndex);
  void renderHexByte(QPainter& painter, const int rowIndex, const int columnIndex, QColor& bgColor,
                     QColor& fgColor, bool drawCarret);
  void renderASCIIText(QPainter& painter, const int rowIndex, const int columnIndex,
                       QColor& bgColor, QColor& fgColor);
  void renderCarret(QPainter& painter, const int rowIndex, const int columnIndex);
  void determineMemoryTextRenderProperties(const int rowIndex, const int columnIndex,
                                           bool& drawCarret, QColor& bgColor, QColor& fgColor);

  int m_byteSelectedPosX = -1;
  int m_byteSelectedPosY = -1;
  int m_charWidthEm = 0;
  int m_charHeight = 0;
  int m_hexAreaWidth = 0;
  int m_hexAreaHeight = 0;
  int m_rowHeaderWidth = 0;
  int m_columnHeaderHeight = 0;
  int m_hexAsciiSeparatorPosX = 0;
  char* m_updatedRawMemoryData = nullptr;
  char* m_lastRawMemoryData = nullptr;
  int* m_memoryMsElapsedLastChange = nullptr;
  bool m_carretBetweenHex = false;
  bool m_disableScrollContentEvent = false;
  bool m_validMemory = false;
  u32 m_currentFirstAddress = 0;
  u32 m_memViewStart = 0;
  u32 m_memViewEnd = 0;
  QRect* m_curosrRect;
  QList<int> m_hexKeyList;
  QElapsedTimer m_elapsedTimer;
};
