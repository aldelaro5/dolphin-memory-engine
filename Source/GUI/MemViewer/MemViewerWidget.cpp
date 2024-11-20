#include "MemViewerWidget.h"

#include <sstream>

#include <QLabel>
#include <QVBoxLayout>

#include "../../DolphinProcess/DolphinAccessor.h"

MemViewerWidget::MemViewerWidget(QWidget* const parent) : QWidget(parent)
{
  setWindowTitle("Memory Viewer");
  initialiseWidgets();
  makeLayouts();

  connect(m_memViewer, &MemViewer::addWatch, this,
          [this](MemWatchEntry* entry) { emit addWatchRequested(entry); });
}

MemViewerWidget::~MemViewerWidget()
{
  delete m_memViewer;
}

void MemViewerWidget::initialiseWidgets()
{
  m_txtJumpAddress = new AddressInputWidget(this);
  connect(m_txtJumpAddress, &QLineEdit::textChanged, this,
          &MemViewerWidget::onJumpToAddressTextChanged);
  m_btnGoToMEM1Start = new QPushButton(tr("Go to MEM1"));
  connect(m_btnGoToMEM1Start, &QPushButton::clicked, this, &MemViewerWidget::onGoToMEM1Start);
  m_btnGoToSecondaryRAMStart = new QPushButton(tr("Go to ARAM"));
  connect(m_btnGoToSecondaryRAMStart, &QPushButton::clicked, this,
          &MemViewerWidget::onGoToSecondaryRAMStart);
  m_memViewer = new MemViewer(this);
  connect(m_memViewer, &MemViewer::memErrorOccured, this, &MemViewerWidget::mustUnhook);
  m_updateMemoryTimer = new QTimer(this);
  connect(m_updateMemoryTimer, &QTimer::timeout, m_memViewer, &MemViewer::updateViewer);
}

void MemViewerWidget::makeLayouts()
{
  QLabel* lblJumpToAddress = new QLabel(tr("Jump to an address: "));

  QHBoxLayout* controls_layout = new QHBoxLayout();
  controls_layout->addWidget(lblJumpToAddress);
  controls_layout->addWidget(m_txtJumpAddress);
  controls_layout->addWidget(m_btnGoToMEM1Start);
  controls_layout->addWidget(m_btnGoToSecondaryRAMStart);

  QVBoxLayout* main_layout = new QVBoxLayout();
  main_layout->addLayout(controls_layout);
  main_layout->addWidget(m_memViewer);
  setLayout(main_layout);
  layout()->setSizeConstraint(QLayout::SetFixedSize);
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
    m_memViewer->jumpToAddress(jumpAddress);
}

void MemViewerWidget::onGoToMEM1Start()
{
  m_memViewer->jumpToAddress(Common::MEM1_START);
}

void MemViewerWidget::onGoToSecondaryRAMStart()
{
  if (DolphinComm::DolphinAccessor::isARAMAccessible())
    m_memViewer->jumpToAddress(Common::ARAM_START);
  else
    m_memViewer->jumpToAddress(Common::MEM2_START);
}

void MemViewerWidget::hookStatusChanged(bool hook)
{
  m_txtJumpAddress->setEnabled(hook);
  m_btnGoToMEM1Start->setEnabled(hook);
  m_btnGoToSecondaryRAMStart->setEnabled(hook);
  m_memViewer->memoryValidityChanged(hook);
}

void MemViewerWidget::onMEM2StatusChanged(bool enabled)
{
  m_btnGoToSecondaryRAMStart->setText(enabled ? tr("Go to MEM2") : tr("Go to ARAM"));
  if ((!enabled && m_memViewer->getCurrentFirstAddress() >= Common::MEM2_START) ||
      (enabled && m_memViewer->getCurrentFirstAddress() < Common::MEM1_START))
    m_memViewer->jumpToAddress(Common::MEM1_START);

  if (!enabled)
    m_btnGoToSecondaryRAMStart->setEnabled(DolphinComm::DolphinAccessor::isARAMAccessible());
}

void MemViewerWidget::goToAddress(u32 address)
{
  m_memViewer->jumpToAddress(address);
}

void MemViewerWidget::setStructDefs(StructTreeNode* baseNode)
{
  m_memViewer->setStructDefs(baseNode);
}
