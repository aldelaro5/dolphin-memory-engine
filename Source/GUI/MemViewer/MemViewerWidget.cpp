#include "MemViewerWidget.h"

#include <sstream>

#include <QLabel>
#include <QVBoxLayout>

#include "../../DolphinProcess/DolphinAccessor.h"

MemViewerWidget::MemViewerWidget(QWidget* parent, u32 consoleAddress) : QWidget(parent)
{
  QLabel* lblJumpToAddress = new QLabel("Jump to an address: ");
  m_txtJumpAddress = new QLineEdit(this);
  m_btnGoToMEM1Start = new QPushButton("Go to the start of MEM1");
  connect(m_btnGoToMEM1Start, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemViewerWidget::onGoToMEM1Start);
  m_btnGoToMEM2Start = new QPushButton("Go to the start of MEM2");
  connect(m_btnGoToMEM2Start, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), this,
          &MemViewerWidget::onGoToMEM2Start);
  QHBoxLayout* controls_layout = new QHBoxLayout();
  controls_layout->addWidget(lblJumpToAddress);
  controls_layout->addWidget(m_txtJumpAddress);
  controls_layout->addWidget(m_btnGoToMEM1Start);
  controls_layout->addWidget(m_btnGoToMEM2Start);

  QVBoxLayout* main_layout = new QVBoxLayout();
  connect(m_txtJumpAddress, &QLineEdit::textChanged, this,
          &MemViewerWidget::onJumpToAddressTextChanged);
  m_memViewer = new MemViewer(this);
  main_layout->addLayout(controls_layout);
  main_layout->addWidget(m_memViewer);
  setLayout(main_layout);
  layout()->setSizeConstraint(QLayout::SetFixedSize);

  connect(m_memViewer, &MemViewer::memErrorOccured, this, &MemViewerWidget::mustUnhook);

  m_updateMemoryTimer = new QTimer(this);
  connect(m_updateMemoryTimer, &QTimer::timeout, m_memViewer, &MemViewer::updateViewer);
}

QTimer* MemViewerWidget::getUpdateTimer() const
{
  return m_updateMemoryTimer;
}

void MemViewerWidget::onJumpToAddressTextChanged()
{
  std::stringstream ss(m_txtJumpAddress->text().toStdString());
  u32 jumpAddress = 0;
  ss >> std::hex >> std::uppercase >> jumpAddress;
  if (!ss.fail())
  {
    m_memViewer->jumpToAddress(jumpAddress);
  }
}

void MemViewerWidget::onGoToMEM1Start()
{
  m_memViewer->jumpToAddress(Common::MEM1_START);
}

void MemViewerWidget::onGoToMEM2Start()
{
  m_memViewer->jumpToAddress(Common::MEM2_START);
}

void MemViewerWidget::hookStatusChanged(bool hook)
{
  m_txtJumpAddress->setEnabled(hook);
  m_btnGoToMEM1Start->setEnabled(hook);
  m_btnGoToMEM2Start->setEnabled(hook);
  m_memViewer->memoryValidityChanged(hook);
}

void MemViewerWidget::goToAddress(u32 address)
{
  m_memViewer->jumpToAddress(address);
}