#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "MemViewer.h"

class MemViewerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit MemViewerWidget(QWidget* parent);
  ~MemViewerWidget() override;

  MemViewerWidget(const MemViewerWidget&) = delete;
  MemViewerWidget(MemViewerWidget&&) = delete;
  MemViewerWidget& operator=(const MemViewerWidget&) = delete;
  MemViewerWidget& operator=(MemViewerWidget&&) = delete;

  void onJumpToAddressTextChanged();
  void onGoToMEM1Start();
  void onGoToSecondaryRAMStart();
  QTimer* getUpdateTimer() const;
  void hookStatusChanged(bool hook);
  void onMEM2StatusChanged(bool enabled);
  void goToAddress(u32 address);

signals:
  void mustUnhook();
  void addWatchRequested(MemWatchEntry* entry);

private:
  void initialiseWidgets();
  void makeLayouts();

  QLineEdit* m_txtJumpAddress{};
  QPushButton* m_btnGoToMEM1Start{};
  QPushButton* m_btnGoToSecondaryRAMStart{};
  QTimer* m_updateMemoryTimer{};
  MemViewer* m_memViewer{};
};
