#pragma once

#include <QAbstractScrollArea>
#include <QColor>
#include <QElapsedTimer>
#include <QList>
#include <QRect>
#include <QShortcut>
#include <QTimer>
#include <QWidget>

#include "../../Common/CommonTypes.h"
#include "../../Common/MemoryCommon.h"
#include "../../MemoryWatch/MemWatchEntry.h"
#include "../../Structs/StructTreeNode.h"

class MemViewer : public QAbstractScrollArea
{
  Q_OBJECT

public:
  explicit MemViewer(QWidget* parent);
  ~MemViewer() override;

  MemViewer(const MemViewer&) = delete;
  MemViewer(MemViewer&&) = delete;
  MemViewer& operator=(const MemViewer&) = delete;
  MemViewer& operator=(MemViewer&&) = delete;

  QSize sizeHint() const override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void scrollContentsBy(int dx, int dy) override;
  u32 getCurrentFirstAddress() const;
  void jumpToAddress(u32 address);
  void updateViewer();
  void memoryValidityChanged(bool valid);
  void setStructDefs(StructTreeNode* baseNode);

signals:
  void memErrorOccured();
  void addWatch(MemWatchEntry* entry);

private:
  enum class SelectionType
  {
    upward,
    downward,
    single
  };

  enum class MemoryRegion
  {
    MEM1,
    MEM2,
    ARAM
  };

  struct bytePosFromMouse
  {
    int x = 0;
    int y = 0;
    bool isInViewer = false;
  };

  void initialise();

  void updateFontSize();
  bytePosFromMouse mousePosToBytePos(QPoint pos);
  void scrollToSelection();
  void copySelection(Common::MemType type) const;
  void editSelection();
  void addSelectionAsArrayOfBytes();
  void addByteIndexAsWatch(int index);
  bool handleNaviguationKey(int key, bool shiftIsHeld);
  bool writeCharacterToSelectedMemory(char byteToWrite);
  void updateMemoryData();
  void changeMemoryRegion(MemoryRegion region);
  void renderColumnsHeaderText(QPainter& painter) const;
  void renderRowHeaderText(QPainter& painter, int rowIndex) const;
  void renderSeparatorLines(QPainter& painter) const;
  void renderMemory(QPainter& painter, int rowIndex, int columnIndex);
  void renderHexByte(QPainter& painter, int rowIndex, int columnIndex, QColor& bgColor,
                     QColor& fgColor, bool drawCarret);
  void renderASCIIText(QPainter& painter, int rowIndex, int columnIndex, QColor& bgColor,
                       QColor& fgColor);
  void renderCarret(QPainter& painter, int rowIndex, int columnIndex);
  void determineMemoryTextRenderProperties(int rowIndex, int columnIndex, bool& drawCarret,
                                           QColor& bgColor, QColor& fgColor);

  const int m_numRows = 16;
  const int m_numColumns = 16;  // Should be a multiple of 16, or the header doesn't make much sense
  const int m_numCells = m_numRows * m_numColumns;
  int m_memoryFontSize = -1;
  int m_StartBytesSelectionPosX = 0;
  int m_StartBytesSelectionPosY = 0;
  int m_EndBytesSelectionPosX = 0;
  int m_EndBytesSelectionPosY = 0;
  SelectionType m_selectionType = SelectionType::single;
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
  bool m_editingHex = false;
  bool m_carretBetweenHex = false;
  bool m_disableScrollContentEvent = false;
  bool m_validMemory = false;
  u32 m_currentFirstAddress = 0;
  u32 m_memViewStart = 0;
  u32 m_memViewEnd = 0;
  QRect* m_curosrRect{};
  QShortcut* m_copyShortcut{};
  QElapsedTimer m_elapsedTimer;

  StructTreeNode* m_structDefs;
};
