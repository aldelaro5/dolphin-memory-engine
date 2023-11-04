#include "SConfig.h"

SConfig::SConfig()
{
  const QString organization{"dolphin-memory-engine"};
  const QString application{"dolphin-memory-engine"};
  m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, organization, application);
}

SConfig::~SConfig()
{
  delete m_settings;
}

SConfig& SConfig::getInstance()
{
  static SConfig instance;
  return instance;
}

QString SConfig::getWatchModel() const
{
  return m_settings->value("watchModel", QString{}).toString();
}

bool SConfig::getAutoHook() const
{
  return m_settings->value("autoHook", true).toBool();
}

QByteArray SConfig::getMainWindowGeometry() const
{
  return m_settings->value("mainWindowSettings/mainWindowGeometry", QByteArray{}).toByteArray();
}

QByteArray SConfig::getMainWindowState() const
{
  return m_settings->value("mainWindowSettings/mainWindowState", QByteArray{}).toByteArray();
}

QByteArray SConfig::getSplitterState() const
{
  return m_settings->value("mainWindowSettings/splitterState", QByteArray{}).toByteArray();
}

int SConfig::getTheme() const
{
  return m_settings->value("coreSettings/theme", 0).toInt();
}

int SConfig::getWatcherUpdateTimerMs() const
{
  return m_settings->value("timerSettings/watcherUpdateTimerMs", 100).toInt();
}

int SConfig::getFreezeTimerMs() const
{
  return m_settings->value("timerSettings/freezeTimerMs", 10).toInt();
}

int SConfig::getScannerUpdateTimerMs() const
{
  return m_settings->value("timerSettings/scannerUpdateTimerMs", 10).toInt();
}

int SConfig::getViewerUpdateTimerMs() const
{
  return m_settings->value("timerSettings/viewerUpdateTimerMs", 100).toInt();
}

int SConfig::getViewerNbrBytesSeparator() const
{
  return m_settings->value("viewerSettings/nbrBytesSeparator", 1).toInt();
}

u32 SConfig::getMEM1Size() const
{
  return m_settings->value("memorySettings/MEM1Size", 24u * 1024 * 1024).toUInt();
}

u32 SConfig::getMEM2Size() const
{
  return m_settings->value("memorySettings/MEM2Size", 64u * 1024 * 1024).toUInt();
}

void SConfig::setWatchModel(const QString& json)
{
  m_settings->setValue("watchModel", json);
}

void SConfig::setAutoHook(const bool enabled)
{
  m_settings->setValue("autoHook", enabled);
}

void SConfig::setMainWindowGeometry(QByteArray const& geometry)
{
  m_settings->setValue("mainWindowSettings/mainWindowGeometry", geometry);
}

void SConfig::setMainWindowState(QByteArray const& state)
{
  m_settings->setValue("mainWindowSettings/mainWindowState", state);
}

void SConfig::setSplitterState(QByteArray const& state)
{
  m_settings->setValue("mainWindowSettings/splitterState", state);
}

void SConfig::setTheme(const int theme)
{
  m_settings->setValue("coreSettings/theme", theme);
}

void SConfig::setWatcherUpdateTimerMs(const int updateTimerMs)
{
  m_settings->setValue("timerSettings/watcherUpdateTimerMs", updateTimerMs);
}

void SConfig::setFreezeTimerMs(const int freezeTimerMs)
{
  m_settings->setValue("timerSettings/freezeTimerMs", freezeTimerMs);
}

void SConfig::setScannerUpdateTimerMs(const int scannerUpdateTimerMs)
{
  m_settings->setValue("timerSettings/scannerUpdateTimerMs", scannerUpdateTimerMs);
}

void SConfig::setViewerUpdateTimerMs(const int viewerUpdateTimerMs)
{
  m_settings->setValue("timerSettings/viewerUpdateTimerMs", viewerUpdateTimerMs);
}

void SConfig::setViewerNbrBytesSeparator(const int viewerNbrBytesSeparator)
{
  m_settings->setValue("viewerSettings/nbrBytesSeparator", viewerNbrBytesSeparator);
}

void SConfig::setMEM1Size(const u32 mem1SizeReal)
{
  m_settings->setValue("memorySettings/MEM1Size", mem1SizeReal);
}

void SConfig::setMEM2Size(const u32 mem2SizeReal)
{
  m_settings->setValue("memorySettings/MEM2Size", mem2SizeReal);
}